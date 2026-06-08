/**
 * @file ImageConverterEngine.cpp
 * @brief 图片格式转换引擎核心类实现
 *
 * 实现基于 Qt QImage 的图片格式转换功能。
 * 支持 BMP、JPEG、PNG、ICO、HEIC 五种格式之间的相互转换。
 * HEIC 格式通过 libheif 库实现编解码。
 */

#include "ImageConverterEngine.h"
#include "HeicDecoder.h"
#include "HeicEncoder.h"
#include <QFileInfo>
#include <QDir>
#include <QImageReader>
#include <QImageWriter>

// ============================================================================
// 构造 / 析构
// ============================================================================

ImageConverterEngine::ImageConverterEngine()
    : m_targetFormat(FORMAT_JPEG)
    , m_quality(85)
    , m_overwrite(false)
    , m_progressCb(NULL)
    , m_logCb(NULL)
    , m_pCancel(NULL)
{
}

ImageConverterEngine::~ImageConverterEngine()
{
}

// ============================================================================
// 配置方法
// ============================================================================

void ImageConverterEngine::addSourceFile(const QString& sourcePath)
{
    if (!sourcePath.isEmpty())
    {
        m_sourceFiles.append(sourcePath);
    }
}

void ImageConverterEngine::addSourceFiles(const QStringList& sourceFiles)
{
    m_sourceFiles.append(sourceFiles);
}

void ImageConverterEngine::clearTasks()
{
    m_sourceFiles.clear();
    m_results.clear();
}

void ImageConverterEngine::setOutputDir(const QString& dir)
{
    m_outputDir = dir;
}

void ImageConverterEngine::setTargetFormat(ImageFormat format)
{
    m_targetFormat = format;
}

void ImageConverterEngine::setQuality(int quality)
{
    m_quality = quality;
    if (m_quality < 1) m_quality = 1;
    if (m_quality > 100) m_quality = 100;
}

void ImageConverterEngine::setOverwrite(bool overwrite)
{
    m_overwrite = overwrite;
}

void ImageConverterEngine::setProgressCallback(ImageProgressCallback cb)
{
    m_progressCb = cb;
}

void ImageConverterEngine::setLogCallback(ImageLogCallback cb)
{
    m_logCb = cb;
}

void ImageConverterEngine::setCancelFlag(const bool* pCancel)
{
    m_pCancel = pCancel;
}

// ============================================================================
// 核心转换方法
// ============================================================================

/**
 * @brief 执行批量转换
 *
 * 流程：
 *   1. 遍历所有源文件
 *   2. 生成输出路径
 *   3. 加载图片
 *   4. 转换并保存
 *   5. 记录结果
 */
int ImageConverterEngine::convert()
{
    m_results.clear();

    if (m_sourceFiles.isEmpty())
    {
        log("[ERROR] No source files specified");
        return -1;
    }

    int totalFiles = m_sourceFiles.size();
    int successCount = 0;

    log(QString("[INFO] Starting conversion: %1 files to %2 format")
        .arg(totalFiles)
        .arg(getFormatName(m_targetFormat)));

    for (int i = 0; i < totalFiles; ++i)
    {
        // 检查取消标志
        if (m_pCancel != NULL && *m_pCancel)
        {
            log("[INFO] Conversion cancelled by user");
            break;
        }

        const QString& sourcePath = m_sourceFiles[i];
        QString outputPath = generateOutputPath(sourcePath);

        // 检查文件是否已存在
        if (!m_overwrite && QFileInfo::exists(outputPath))
        {
            log(QString("[INFO] Skipped (file exists): %1").arg(outputPath));
            ImageConversionResult result;
            result.success = true;
            result.sourcePath = sourcePath;
            result.outputPath = outputPath;
            result.errorMessage = "Skipped (file exists)";
            m_results.append(result);
            reportProgress(i + 1, totalFiles, 100);
            continue;
        }

        // 执行转换
        reportProgress(i + 1, totalFiles, 0);
        bool success = convertSingleFile(sourcePath, outputPath);
        reportProgress(i + 1, totalFiles, 100);

        // 记录结果
        ImageConversionResult result;
        result.success = success;
        result.sourcePath = sourcePath;
        result.outputPath = outputPath;

        if (success)
        {
            successCount++;
            log(QString("[INFO] Converted: %1 -> %2").arg(sourcePath).arg(outputPath));
            result.errorMessage = "";
        }
        else
        {
            result.errorMessage = "Failed to convert";
            log(QString("[ERROR] Failed to convert: %1").arg(sourcePath));
        }

        m_results.append(result);
    }

    log(QString("[INFO] Conversion completed: %1/%2 files converted")
        .arg(successCount)
        .arg(totalFiles));

    return successCount;
}

/**
 * @brief 转换单个文件
 *
 * HEIC 格式需要特殊处理：
 *   - 读取时使用 HeicDecoder 解码
 *   - 写入时使用 HeicEncoder 编码
 * 其他格式使用 Qt 原生 QImageReader/QImageWriter
 */
bool ImageConverterEngine::convertSingleFile(const QString& sourcePath, const QString& outputPath)
{
    // 加载图片
    QImage image;
    ImageFormat sourceFormat = detectFormat(sourcePath);

    if (sourceFormat == FORMAT_HEIC)
    {
        // HEIC 文件需要通过 HeicDecoder 解码
        QString errorMsg;
        image = HeicDecoder::decodeFromFile(sourcePath, &errorMsg);
        if (image.isNull())
        {
            log(QString("[ERROR] Failed to load HEIC image: %1 (%2)")
                .arg(sourcePath)
                .arg(errorMsg));
            return false;
        }
    }
    else
    {
        // 其他格式使用 Qt 原生读取
        QImageReader reader(sourcePath);
        image = reader.read();

        if (image.isNull())
        {
            log(QString("[ERROR] Failed to load image: %1 (%2)")
                .arg(sourcePath)
                .arg(reader.errorString()));
            return false;
        }
    }

    // 确保输出目录存在
    QFileInfo outputInfo(outputPath);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists())
    {
        if (!outputDir.mkpath("."))
        {
            log(QString("[ERROR] Failed to create output directory: %1").arg(outputDir.absolutePath()));
            return false;
        }
    }

    // 保存为目标格式
    if (m_targetFormat == FORMAT_HEIC)
    {
        // HEIC 格式使用 HeicEncoder 编码
        QString errorMsg;
        bool ok = HeicEncoder::encodeToFile(outputPath, image, m_quality, &errorMsg);
        if (!ok)
        {
            log(QString("[ERROR] Failed to encode HEIC image: %1 (%2)")
                .arg(outputPath)
                .arg(errorMsg));
            return false;
        }
        return true;
    }

    // 其他格式使用 Qt 原生写入
    QImageWriter writer(outputPath);
    
    // 设置格式特定参数
    switch (m_targetFormat)
    {
    case FORMAT_JPEG:
        writer.setFormat("JPEG");
        writer.setQuality(m_quality);
        break;
    case FORMAT_PNG:
        writer.setFormat("PNG");
        // PNG压缩级别通过quality设置（0-100映射到0-9）
        writer.setQuality(qBound(0, m_quality / 10, 9));
        break;
    case FORMAT_BMP:
        writer.setFormat("BMP");
        break;
    case FORMAT_ICO:
        writer.setFormat("ICO");
        break;
    case FORMAT_HEIC:
        // 已在上方处理
        break;
    }

    if (!writer.write(image))
    {
        log(QString("[ERROR] Failed to write image: %1 (%2)")
            .arg(outputPath)
            .arg(writer.errorString()));
        return false;
    }

    return true;
}

/**
 * @brief 生成输出文件路径
 */
QString ImageConverterEngine::generateOutputPath(const QString& sourcePath) const
{
    QFileInfo sourceInfo(sourcePath);
    QString baseName = sourceInfo.completeBaseName();
    QString extension = getFormatExtension(m_targetFormat);
    
    QString outputDir = m_outputDir.isEmpty() 
        ? sourceInfo.absolutePath() 
        : m_outputDir;

    return QString("%1/%2.%3")
        .arg(outputDir)
        .arg(baseName)
        .arg(extension);
}

// ============================================================================
// 辅助方法
// ============================================================================

const QList<ImageConversionResult>& ImageConverterEngine::getResults() const
{
    return m_results;
}

QString ImageConverterEngine::getFormatExtension(ImageFormat format)
{
    switch (format)
    {
    case FORMAT_BMP:
        return "bmp";
    case FORMAT_JPEG:
        return "jpg";
    case FORMAT_PNG:
        return "png";
    case FORMAT_ICO:
        return "ico";
    case FORMAT_HEIC:
        return "heic";
    default:
        return "jpg";
    }
}

ImageFormat ImageConverterEngine::detectFormat(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    
    if (ext == "bmp") return FORMAT_BMP;
    if (ext == "jpg" || ext == "jpeg") return FORMAT_JPEG;
    if (ext == "png") return FORMAT_PNG;
    if (ext == "ico") return FORMAT_ICO;
    if (ext == "heic" || ext == "heif") return FORMAT_HEIC;
    
    return FORMAT_BMP;  // 默认
}

QString ImageConverterEngine::getFormatName(ImageFormat format)
{
    switch (format)
    {
    case FORMAT_BMP:
        return "BMP";
    case FORMAT_JPEG:
        return "JPEG";
    case FORMAT_PNG:
        return "PNG";
    case FORMAT_ICO:
        return "ICO";
    case FORMAT_HEIC:
        return "HEIC";
    default:
        return "Unknown";
    }
}

void ImageConverterEngine::log(const QString& msg) const
{
    if (m_logCb != NULL)
    {
        m_logCb(msg.toUtf8().constData());
    }
}

void ImageConverterEngine::reportProgress(int currentFile, int totalFiles, int percent) const
{
    if (m_progressCb != NULL)
    {
        m_progressCb(currentFile, totalFiles, percent);
    }
}
