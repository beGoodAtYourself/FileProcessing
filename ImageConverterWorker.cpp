/**
 * @file ImageConverterWorker.cpp
 * @brief 图片转换工作线程实现
 *
 * 实现 ImageConverterWorker 的工作线程逻辑，通过回调桥接模式
 * 将 ImageConverterEngine 的 C 风格回调转换为 Qt 信号。
 */

#include "ImageConverterWorker.h"

// 静态成员初始化
ImageConverterWorker* ImageConverterWorker::s_pCurrentWorker = NULL;

// ============================================================================
// 构造 / 析构
// ============================================================================

ImageConverterWorker::ImageConverterWorker(QObject *parent)
    : QThread(parent)
    , m_targetFormat(FORMAT_JPEG)
    , m_quality(85)
    , m_overwrite(false)
    , m_cancelFlag(false)
{
}

ImageConverterWorker::~ImageConverterWorker()
{
    if (isRunning())
    {
        cancel();
        wait();
    }
}

// ============================================================================
// 配置方法
// ============================================================================

void ImageConverterWorker::setSourceFiles(const QStringList& files)
{
    m_sourceFiles = files;
}

void ImageConverterWorker::setOutputDir(const QString& dir)
{
    m_outputDir = dir;
}

void ImageConverterWorker::setTargetFormat(ImageFormat format)
{
    m_targetFormat = format;
}

void ImageConverterWorker::setQuality(int quality)
{
    m_quality = quality;
}

void ImageConverterWorker::setOverwrite(bool overwrite)
{
    m_overwrite = overwrite;
}

void ImageConverterWorker::cancel()
{
    m_cancelFlag = true;
}

const QList<ImageConversionResult>& ImageConverterWorker::getResults() const
{
    return m_results;
}

// ============================================================================
// 线程入口
// ============================================================================

/**
 * @brief 线程入口函数
 *
 * 创建 ImageConverterEngine，配置参数，注册回调，执行转换。
 * 通过静态指针 s_pCurrentWorker 实现回调到信号的桥接。
 */
void ImageConverterWorker::run()
{
    // 重置取消标志
    m_cancelFlag = false;

    // 设置静态指针，用于回调中访问当前实例
    s_pCurrentWorker = this;

    // 创建转换引擎
    ImageConverterEngine engine;

    // 配置参数
    engine.addSourceFiles(m_sourceFiles);
    
    if (!m_outputDir.isEmpty())
    {
        engine.setOutputDir(m_outputDir);
    }

    engine.setTargetFormat(m_targetFormat);
    engine.setQuality(m_quality);
    engine.setOverwrite(m_overwrite);

    // 注册回调
    engine.setProgressCallback(progressCallback);
    engine.setLogCallback(logCallback);
    engine.setCancelFlag(&m_cancelFlag);

    // 执行转换
    int successCount = engine.convert();

    // 保存结果
    m_results = engine.getResults();

    // 清除静态指针
    s_pCurrentWorker = NULL;

    // 计算总数（可能因取消而减少）
    int totalFiles = m_results.size();
    if (totalFiles == 0)
    {
        totalFiles = m_sourceFiles.size();
    }

    // 发射完成信号
    emit conversionFinished(successCount, totalFiles);
}

// ============================================================================
// 静态回调函数
// ============================================================================

/**
 * @brief 进度回调
 *
 * 由 ImageConverterEngine 调用，通过 s_pCurrentWorker
 * 将进度信息转发为 progressChanged 信号。
 */
void ImageConverterWorker::progressCallback(int currentFile, int totalFiles, int percent)
{
    if (s_pCurrentWorker != NULL)
    {
        emit s_pCurrentWorker->progressChanged(currentFile, totalFiles, percent);
    }
}

/**
 * @brief 日志回调
 *
 * 由 ImageConverterEngine 调用，通过 s_pCurrentWorker
 * 将日志信息转发为 logMessage 信号。
 */
void ImageConverterWorker::logCallback(const char* message)
{
    if (s_pCurrentWorker != NULL)
    {
        emit s_pCurrentWorker->logMessage(QString::fromUtf8(message));
    }
}
