/**
 * @file ImageConverterPanel.h
 * @brief 图片转换面板声明
 *
 * ImageConverterPanel 为图片格式转换功能的独立面板，包含：
 *   - 单文件转换页
 *   - 批量转换页
 *   - 格式选择
 *   - 输出路径设置
 *   - 进度显示
 *   - 日志输出
 */

#ifndef IMAGECONVERTERPANEL_H
#define IMAGECONVERTERPANEL_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QListWidget>
#include "ImageConverterEngine.h"

// 前置声明
class ImageConverterWorker;

/**
 * @brief 图片转换面板类
 *
 * 提供 BMP、JPEG、PNG、ICO、HEIC 五种格式之间的相互转换功能。
 * 支持单文件和批量转换两种模式。
 */
class ImageConverterPanel : public QMainWindow
{
    Q_OBJECT

public:
    explicit ImageConverterPanel(QWidget *parent = 0);
    ~ImageConverterPanel();

private slots:
    /** @brief 单文件模式：浏览选择源文件 */
    void onBrowseSingleFile();

    /** @brief 批量模式：添加文件 */
    void onAddFiles();

    /** @brief 批量模式：添加文件夹 */
    void onAddFolder();

    /** @brief 批量模式：移除选中文件 */
    void onRemoveSelected();

    /** @brief 批量模式：清空文件列表 */
    void onClearFiles();

    /** @brief 浏览选择输出目录 */
    void onBrowseOutputDir();

    /** @brief 目标格式变化时更新UI */
    void onFormatChanged(int index);

    /** @brief 开始转换 */
    void onStartConversion();

    /** @brief 取消转换 */
    void onCancelConversion();

    /** @brief 响应进度变化信号 */
    void onProgressChanged(int currentFile, int totalFiles, int percent);

    /** @brief 响应日志消息信号 */
    void onLogMessage(const QString& message);

    /** @brief 响应转换完成信号 */
    void onConversionFinished(int successCount, int totalFiles);

private:
    /** @brief 创建所有 UI 控件和布局 */
    void createUI();

    /** @brief 创建单文件转换页 */
    QWidget* createSingleFilePage();

    /** @brief 创建批量转换页 */
    QWidget* createBatchPage();

    /** @brief 创建设置区域 */
    QWidget* createSettingsGroup();

    /** @brief 创建日志区域 */
    QWidget* createLogGroup();

    /** @brief 连接信号与槽 */
    void createConnections();

    /** @brief 批量启用/禁用输入控件 */
    void setControlsEnabled(bool enabled);

    /** @brief 向日志区域追加带颜色的文本 */
    void appendLog(const QString& text, const QString& color = "black");

    /** @brief 获取当前选中的目标格式 */
    ImageFormat getCurrentFormat() const;

    /** @brief 更新转换按钮状态 */
    void updateConvertButton();

    // ---- 单文件页控件 ----
    QLineEdit* m_singleFileEdit;        ///< 单文件路径输入框
    QPushButton* m_browseSingleBtn;     ///< 单文件浏览按钮

    // ---- 批量页控件 ----
    QListWidget* m_fileListWidget;      ///< 文件列表
    QPushButton* m_addFilesBtn;         ///< 添加文件按钮
    QPushButton* m_addFolderBtn;        ///< 添加文件夹按钮
    QPushButton* m_removeBtn;           ///< 移除按钮
    QPushButton* m_clearBtn;            ///< 清空按钮
    QLabel* m_fileCountLabel;           ///< 文件数量标签

    // ---- 设置区控件 ----
    QComboBox* m_formatCombo;           ///< 目标格式下拉框
    QSpinBox* m_qualitySpin;            ///< JPEG质量输入框
    QLabel* m_qualityLabel;             ///< 质量标签
    QLineEdit* m_outputDirEdit;         ///< 输出目录输入框
    QPushButton* m_browseOutputBtn;     ///< 输出目录浏览按钮
    QCheckBox* m_overwriteCheck;        ///< 覆盖复选框

    // ---- 操作按钮 ----
    QPushButton* m_convertBtn;          ///< 转换按钮
    QPushButton* m_cancelBtn;           ///< 取消按钮

    // ---- 进度和日志 ----
    QProgressBar* m_progressBar;        ///< 进度条
    QLabel* m_statusLabel;              ///< 状态标签
    QTextEdit* m_logEdit;               ///< 日志显示区域

    // ---- 标签页 ----
    QTabWidget* m_tabWidget;            ///< 标签页容器

    // ---- 工作线程 ----
    ImageConverterWorker* m_pWorker;    ///< 转换工作线程指针
};

#endif // IMAGECONVERTERPANEL_H
