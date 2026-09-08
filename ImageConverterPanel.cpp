/**
 * @file ImageConverterPanel.cpp
 * @brief 图片转换面板实现
 *
 * 实现图片格式转换面板的界面构建、事件处理、工作线程管理等功能。
 */

#include "ImageConverterPanel.h"
#include "ImageConverterWorker.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

// ============================================================================
// 构造 / 析构
// ============================================================================

ImageConverterPanel::ImageConverterPanel(QWidget *parent)
    : QMainWindow(parent)
    , m_pWorker(NULL)
{
    createUI();
    createConnections();
    updateConvertButton();
}

ImageConverterPanel::~ImageConverterPanel()
{
    if (m_pWorker != NULL)
    {
        m_pWorker->cancel();
        m_pWorker->wait();
        delete m_pWorker;
        m_pWorker = NULL;
    }
}

// ============================================================================
// UI 构建
// ============================================================================

void ImageConverterPanel::createUI()
{
    setWindowTitle(tr("Image Format Converter"));
    setMinimumWidth(650);
    setMinimumHeight(550);

    // 中央容器
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 标签页：单文件 / 批量
    m_tabWidget = new QTabWidget(centralWidget);
    m_tabWidget->addTab(createSingleFilePage(), tr("Single File"));
    m_tabWidget->addTab(createBatchPage(), tr("Batch Conversion"));
    mainLayout->addWidget(m_tabWidget);

    // 设置区域
    mainLayout->addWidget(createSettingsGroup());

    // 进度和状态
    QHBoxLayout* progressLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Ready"));
    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(true);
    progressLayout->addWidget(m_statusLabel, 1);
    progressLayout->addWidget(m_progressBar, 3);
    mainLayout->addLayout(progressLayout);

    // 操作按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_convertBtn = new QPushButton(tr("Convert"));
    m_convertBtn->setFixedHeight(32);
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_cancelBtn->setFixedHeight(32);
    m_cancelBtn->setEnabled(false);

    btnLayout->addStretch();
    btnLayout->addWidget(m_convertBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    // 日志区域
    mainLayout->addWidget(createLogGroup());
}

/**
 * @brief 创建单文件转换页
 */
QWidget* ImageConverterPanel::createSingleFilePage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    // 源文件选择
    QGroupBox* sourceGroup = new QGroupBox(tr("Source File"));
    QHBoxLayout* sourceLayout = new QHBoxLayout(sourceGroup);

    m_singleFileEdit = new QLineEdit();
    m_singleFileEdit->setPlaceholderText(tr("Select an image file to convert..."));
    m_browseSingleBtn = new QPushButton(tr("Browse..."));
    m_browseSingleBtn->setFixedWidth(90);

    sourceLayout->addWidget(m_singleFileEdit);
    sourceLayout->addWidget(m_browseSingleBtn);

    layout->addWidget(sourceGroup);
    layout->addStretch();

    return page;
}

/**
 * @brief 创建批量转换页
 */
QWidget* ImageConverterPanel::createBatchPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    // 文件列表区域
    QGroupBox* listGroup = new QGroupBox(tr("File List"));
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    // 按钮行
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_addFilesBtn = new QPushButton(tr("Add Files..."));
    m_addFolderBtn = new QPushButton(tr("Add Folder..."));
    m_removeBtn = new QPushButton(tr("Remove"));
    m_clearBtn = new QPushButton(tr("Clear All"));

    btnLayout->addWidget(m_addFilesBtn);
    btnLayout->addWidget(m_addFolderBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_clearBtn);
    btnLayout->addStretch();

    // 文件数量标签
    m_fileCountLabel = new QLabel(tr("0 files selected"));
    btnLayout->addWidget(m_fileCountLabel);

    listLayout->addLayout(btnLayout);

    // 文件列表
    m_fileListWidget = new QListWidget();
    m_fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listLayout->addWidget(m_fileListWidget);

    layout->addWidget(listGroup);

    return page;
}

/**
 * @brief 创建设置区域
 */
QWidget* ImageConverterPanel::createSettingsGroup()
{
    QGroupBox* settingsGroup = new QGroupBox(tr("Conversion Settings"));
    QGridLayout* layout = new QGridLayout(settingsGroup);

    // 目标格式
    layout->addWidget(new QLabel(tr("Target Format:")), 0, 0);
    m_formatCombo = new QComboBox();
    m_formatCombo->addItem("BMP", (int)FORMAT_BMP);
    m_formatCombo->addItem("JPEG", (int)FORMAT_JPEG);
    m_formatCombo->addItem("PNG", (int)FORMAT_PNG);
    m_formatCombo->addItem("ICO", (int)FORMAT_ICO);
    m_formatCombo->addItem("HEIC", (int)FORMAT_HEIC);
    m_formatCombo->addItem("SVG", (int)FORMAT_SVG);
    m_formatCombo->setCurrentIndex(1);  // 默认 JPEG
    layout->addWidget(m_formatCombo, 0, 1);

    // JPEG/HEIC 质量
    m_qualityLabel = new QLabel(tr("Quality:"));
    layout->addWidget(m_qualityLabel, 0, 2);
    m_qualitySpin = new QSpinBox();
    m_qualitySpin->setRange(1, 100);
    m_qualitySpin->setValue(85);
    m_qualitySpin->setToolTip(tr("JPEG/HEIC quality (1-100). Higher = better quality, larger file."));
    layout->addWidget(m_qualitySpin, 0, 3);

    // 输出目录
    layout->addWidget(new QLabel(tr("Output Directory:")), 1, 0);
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText(tr("Leave empty to output to source file directory"));
    layout->addWidget(m_outputDirEdit, 1, 1, 1, 2);

    m_browseOutputBtn = new QPushButton(tr("Browse..."));
    m_browseOutputBtn->setFixedWidth(90);
    layout->addWidget(m_browseOutputBtn, 1, 3);

    m_svgHintLabel = new QLabel(tr("SVG preserves the original image. Zoom clarity is limited by the original resolution."));
    m_svgHintLabel->setWordWrap(true);
    m_svgHintLabel->setVisible(false);
    layout->addWidget(m_svgHintLabel, 2, 0, 1, 4);

    // 覆盖选项
    m_overwriteCheck = new QCheckBox(tr("Overwrite existing files"));
    layout->addWidget(m_overwriteCheck, 3, 0, 1, 4);

    return settingsGroup;
}

/**
 * @brief 创建日志区域
 */
QWidget* ImageConverterPanel::createLogGroup()
{
    QGroupBox* logGroup = new QGroupBox(tr("Log"));
    QVBoxLayout* layout = new QVBoxLayout(logGroup);

    m_logEdit = new QTextEdit();
    m_logEdit->setReadOnly(true);
    m_logEdit->setFont(QFont("Consolas", 9));
    m_logEdit->setMaximumHeight(120);

    layout->addWidget(m_logEdit);
    return logGroup;
}

// ============================================================================
// 信号槽连接
// ============================================================================

void ImageConverterPanel::createConnections()
{
    // 单文件页
    connect(m_browseSingleBtn, SIGNAL(clicked()), this, SLOT(onBrowseSingleFile()));

    // 批量页
    connect(m_addFilesBtn, SIGNAL(clicked()), this, SLOT(onAddFiles()));
    connect(m_addFolderBtn, SIGNAL(clicked()), this, SLOT(onAddFolder()));
    connect(m_removeBtn, SIGNAL(clicked()), this, SLOT(onRemoveSelected()));
    connect(m_clearBtn, SIGNAL(clicked()), this, SLOT(onClearFiles()));

    // 设置
    connect(m_browseOutputBtn, SIGNAL(clicked()), this, SLOT(onBrowseOutputDir()));
    connect(m_formatCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onFormatChanged(int)));

    // 操作按钮
    connect(m_convertBtn, SIGNAL(clicked()), this, SLOT(onStartConversion()));
    connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(onCancelConversion()));

    // 文件列表变化
    connect(m_fileListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(updateConvertButton()));
}

// ============================================================================
// 槽函数
// ============================================================================

/**
 * @brief 浏览选择单个源文件
 */
void ImageConverterPanel::onBrowseSingleFile()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filter = tr("Image Files (*.bmp *.jpg *.jpeg *.png *.ico *.heic *.heif);;All Files (*.*)");
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select Image File"), desktopPath, filter);

    if (!filePath.isEmpty())
    {
        m_singleFileEdit->setText(filePath);
        updateConvertButton();
    }
}

/**
 * @brief 添加多个文件
 */
void ImageConverterPanel::onAddFiles()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filter = tr("Image Files (*.bmp *.jpg *.jpeg *.png *.ico *.heic *.heif);;All Files (*.*)");
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Image Files"), desktopPath, filter);

    foreach (const QString& file, files)
    {
        if (m_fileListWidget->findItems(file, Qt::MatchExactly).isEmpty())
        {
            m_fileListWidget->addItem(file);
        }
    }

    m_fileCountLabel->setText(tr("%1 files selected").arg(m_fileListWidget->count()));
    updateConvertButton();
}

/**
 * @brief 添加文件夹中的所有图片
 */
void ImageConverterPanel::onAddFolder()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString folder = QFileDialog::getExistingDirectory(this, tr("Select Folder"), desktopPath);

    if (folder.isEmpty())
    {
        return;
    }

    QDir dir(folder);
    QStringList filters;
    filters << "*.bmp" << "*.jpg" << "*.jpeg" << "*.png" << "*.ico" << "*.heic" << "*.heif";
    dir.setNameFilters(filters);

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files);
    int addedCount = 0;

    foreach (const QFileInfo& fileInfo, fileInfoList)
    {
        QString filePath = fileInfo.absoluteFilePath();
        if (m_fileListWidget->findItems(filePath, Qt::MatchExactly).isEmpty())
        {
            m_fileListWidget->addItem(filePath);
            addedCount++;
        }
    }

    m_fileCountLabel->setText(tr("%1 files selected").arg(m_fileListWidget->count()));
    updateConvertButton();

    appendLog(tr("Added %1 files from folder: %2").arg(addedCount).arg(folder), "blue");
}

/**
 * @brief 移除选中的文件
 */
void ImageConverterPanel::onRemoveSelected()
{
    QList<QListWidgetItem*> selectedItems = m_fileListWidget->selectedItems();
    foreach (QListWidgetItem* item, selectedItems)
    {
        delete m_fileListWidget->takeItem(m_fileListWidget->row(item));
    }

    m_fileCountLabel->setText(tr("%1 files selected").arg(m_fileListWidget->count()));
    updateConvertButton();
}

/**
 * @brief 清空文件列表
 */
void ImageConverterPanel::onClearFiles()
{
    m_fileListWidget->clear();
    m_fileCountLabel->setText(tr("0 files selected"));
    updateConvertButton();
}

/**
 * @brief 浏览选择输出目录
 */
void ImageConverterPanel::onBrowseOutputDir()
{
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), desktopPath);

    if (!dir.isEmpty())
    {
        m_outputDirEdit->setText(dir);
    }
}

/**
 * @brief 目标格式变化时更新UI
 */
void ImageConverterPanel::onFormatChanged(int index)
{
    // JPEG 和 HEIC 格式显示质量设置
    ImageFormat format = (ImageFormat)m_formatCombo->currentData().toInt();
    bool showQuality = (format == FORMAT_JPEG || format == FORMAT_HEIC);
    
    m_qualityLabel->setVisible(showQuality);
    m_qualitySpin->setVisible(showQuality);
    m_svgHintLabel->setVisible(format == FORMAT_SVG);

    // HEIC 默认质量较低（50），JPEG 默认 85
    if (format == FORMAT_HEIC && m_qualitySpin->value() > 100)
    {
        m_qualitySpin->setValue(50);
    }
}

/**
 * @brief 开始转换
 */
void ImageConverterPanel::onStartConversion()
{
    // 收集源文件
    QStringList sourceFiles;
    int currentTab = m_tabWidget->currentIndex();

    if (currentTab == 0)  // 单文件模式
    {
        QString filePath = m_singleFileEdit->text().trimmed();
        if (filePath.isEmpty())
        {
            QMessageBox::warning(this, tr("Warning"), tr("Please select a source file."));
            return;
        }

        if (!QFileInfo::exists(filePath))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Source file does not exist."));
            return;
        }

        sourceFiles.append(filePath);
    }
    else  // 批量模式
    {
        for (int i = 0; i < m_fileListWidget->count(); ++i)
        {
            sourceFiles.append(m_fileListWidget->item(i)->text());
        }

        if (sourceFiles.isEmpty())
        {
            QMessageBox::warning(this, tr("Warning"), tr("Please add files to convert."));
            return;
        }
    }

    // 创建工作线程
    m_pWorker = new ImageConverterWorker();
    m_pWorker->setSourceFiles(sourceFiles);
    m_pWorker->setOutputDir(m_outputDirEdit->text().trimmed());
    m_pWorker->setTargetFormat(getCurrentFormat());
    m_pWorker->setQuality(m_qualitySpin->value());
    m_pWorker->setOverwrite(m_overwriteCheck->isChecked());

    // 连接信号
    connect(m_pWorker, SIGNAL(progressChanged(int, int, int)),
            this, SLOT(onProgressChanged(int, int, int)));
    connect(m_pWorker, SIGNAL(logMessage(QString)),
            this, SLOT(onLogMessage(QString)));
    connect(m_pWorker, SIGNAL(conversionFinished(int, int)),
            this, SLOT(onConversionFinished(int, int)));

    // 重置UI
    m_progressBar->setValue(0);
    m_logEdit->clear();
    m_statusLabel->setText(tr("Converting..."));
    appendLog(tr("Starting conversion: %1 files to %2 format")
        .arg(sourceFiles.size())
        .arg(ImageConverterEngine::getFormatName(getCurrentFormat())), "blue");

    // 禁用控件
    setControlsEnabled(false);

    // 启动线程
    m_pWorker->start();
}

/**
 * @brief 取消转换
 */
void ImageConverterPanel::onCancelConversion()
{
    if (m_pWorker != NULL && m_pWorker->isRunning())
    {
        m_pWorker->cancel();
        appendLog("Cancelling...", "orange");
        m_cancelBtn->setEnabled(false);
    }
}

/**
 * @brief 进度更新
 */
void ImageConverterPanel::onProgressChanged(int currentFile, int totalFiles, int percent)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(tr("Converting %1/%2...").arg(currentFile).arg(totalFiles));
}

/**
 * @brief 日志消息
 */
void ImageConverterPanel::onLogMessage(const QString& message)
{
    if (message.contains("[ERROR]"))
    {
        appendLog(message, "red");
    }
    else if (message.contains("[INFO]"))
    {
        appendLog(message, "blue");
    }
    else
    {
        appendLog(message, "black");
    }
}

/**
 * @brief 转换完成
 */
void ImageConverterPanel::onConversionFinished(int successCount, int totalFiles)
{
    setControlsEnabled(true);

    if (successCount == totalFiles)
    {
        m_progressBar->setValue(100);
        m_statusLabel->setText(tr("Conversion completed"));
        appendLog(tr("Conversion completed: %1/%2 files converted successfully!")
            .arg(successCount).arg(totalFiles), "green");
    }
    else if (successCount > 0)
    {
        m_statusLabel->setText(tr("Partial success"));
        appendLog(tr("Conversion finished: %1/%2 files converted")
            .arg(successCount).arg(totalFiles), "orange");
    }
    else
    {
        m_statusLabel->setText(tr("Conversion failed"));
        appendLog(tr("Conversion failed: no files were converted"), "red");
    }

    // 清理工作线程
    if (m_pWorker != NULL)
    {
        m_pWorker->deleteLater();
        m_pWorker = NULL;
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

void ImageConverterPanel::setControlsEnabled(bool enabled)
{
    m_singleFileEdit->setEnabled(enabled);
    m_browseSingleBtn->setEnabled(enabled);
    m_addFilesBtn->setEnabled(enabled);
    m_addFolderBtn->setEnabled(enabled);
    m_removeBtn->setEnabled(enabled);
    m_clearBtn->setEnabled(enabled);
    m_fileListWidget->setEnabled(enabled);
    m_formatCombo->setEnabled(enabled);
    m_qualitySpin->setEnabled(enabled);
    m_outputDirEdit->setEnabled(enabled);
    m_browseOutputBtn->setEnabled(enabled);
    m_overwriteCheck->setEnabled(enabled);
    m_convertBtn->setEnabled(enabled);
    m_cancelBtn->setEnabled(!enabled);
}

void ImageConverterPanel::appendLog(const QString& text, const QString& color)
{
    m_logEdit->append(QString("<span style=\"color:%1;\">%2</span>")
        .arg(color, text.toHtmlEscaped()));
}

ImageFormat ImageConverterPanel::getCurrentFormat() const
{
    return (ImageFormat)m_formatCombo->currentData().toInt();
}

void ImageConverterPanel::updateConvertButton()
{
    bool hasFiles = false;
    int currentTab = m_tabWidget->currentIndex();

    if (currentTab == 0)  // 单文件模式
    {
        hasFiles = !m_singleFileEdit->text().trimmed().isEmpty();
    }
    else  // 批量模式
    {
        hasFiles = m_fileListWidget->count() > 0;
    }

    m_convertBtn->setEnabled(hasFiles);
}
