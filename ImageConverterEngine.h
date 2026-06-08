/**
 * @file ImageConverterEngine.h
 * @brief 图片格式转换引擎核心类声明
 *
 * ImageConverterEngine 封装了图片格式转换的核心逻辑，与 UI 完全解耦。
 * 支持 BMP、JPEG、PNG、ICO、HEIC 五种格式之间的相互转换。
 *
 * 核心特性：
 *   - 基于 Qt QImage 实现，支持多种图片格式
 *   - 支持 JPEG/HEIC 质量参数设置
 *   - 支持 ICO 多尺寸处理
 *   - 通过 HEIC 编解码器（libheif）支持 HEIC 格式读写
 *   - 通过回调机制通知转换进度和日志
 */

#ifndef IMAGECONVERTERENGINE_H
#define IMAGECONVERTERENGINE_H

#include <QString>
#include <QStringList>
#include <QImage>

/**
 * @brief 支持的图片格式枚举
 */
enum ImageFormat
{
    FORMAT_BMP = 0,
    FORMAT_JPEG = 1,
    FORMAT_PNG = 2,
    FORMAT_ICO = 3,
    FORMAT_HEIC = 4
};

/**
 * @brief 转换进度回调函数类型
 *
 * @param currentFile 当前文件序号（从1开始）
 * @param totalFiles  总文件数
 * @param percent     当前文件转换进度（0-100）
 */
typedef void (*ImageProgressCallback)(int currentFile, int totalFiles, int percent);

/**
 * @brief 转换日志回调函数类型
 *
 * @param message 日志文本
 */
typedef void (*ImageLogCallback)(const char* message);

/**
 * @brief 单个转换任务配置
 */
struct ImageConversionTask
{
    QString sourcePath;     ///< 源文件路径
    QString outputPath;     ///< 输出文件路径
    ImageFormat format;     ///< 目标格式
    int quality;            ///< JPEG/HEIC质量（1-100）
};

/**
 * @brief 转换结果
 */
struct ImageConversionResult
{
    bool success;           ///< 是否成功
    QString sourcePath;     ///< 源文件路径
    QString outputPath;     ///< 输出文件路径
    QString errorMessage;   ///< 错误信息（失败时）
};

/**
 * @brief 图片格式转换核心类
 *
 * 纯逻辑层，不依赖 UI 框架。通过 setter 方法配置参数，
 * 通过回调接口输出进度和日志。
 *
 * 典型使用流程：
 *   1. addConversionTask()  添加转换任务
 *   2. setOutputDir()       设置输出目录
 *   3. setTargetFormat()    设置目标格式
 *   4. setQuality()         设置JPEG质量
 *   5. setProgressCallback() / setLogCallback()  注册回调
 *   6. convert()            执行转换
 */
class ImageConverterEngine
{
public:
    ImageConverterEngine();
    ~ImageConverterEngine();

    /**
     * @brief 添加单个转换任务
     * @param sourcePath 源文件路径
     */
    void addSourceFile(const QString& sourcePath);

    /**
     * @brief 添加多个转换任务
     * @param sourceFiles 源文件路径列表
     */
    void addSourceFiles(const QStringList& sourceFiles);

    /**
     * @brief 清空任务列表
     */
    void clearTasks();

    /**
     * @brief 设置输出目录
     * @param dir 输出目录路径，为空时输出到源文件所在目录
     */
    void setOutputDir(const QString& dir);

    /**
     * @brief 设置目标格式
     * @param format 目标图片格式
     */
    void setTargetFormat(ImageFormat format);

    /**
     * @brief 设置 JPEG/HEIC 质量
     * @param quality 质量参数（1-100），默认85
     */
    void setQuality(int quality);

    /**
     * @brief 设置是否覆盖已存在的文件
     * @param overwrite true 覆盖，false 跳过
     */
    void setOverwrite(bool overwrite);

    /**
     * @brief 注册进度回调
     * @param cb 进度回调函数指针，NULL 则不回调
     */
    void setProgressCallback(ImageProgressCallback cb);

    /**
     * @brief 注册日志回调
     * @param cb 日志回调函数指针，NULL 则不回调
     */
    void setLogCallback(ImageLogCallback cb);

    /**
     * @brief 设置取消标志
     *
     * 转换过程中会周期性检查该标志，若为 true 则中止操作。
     *
     * @param pCancel 指向外部的 bool 标志，NULL 则不检查取消
     */
    void setCancelFlag(const bool* pCancel);

    /**
     * @brief 执行批量转换
     *
     * @return 成功转换的文件数量，负数表示错误
     */
    int convert();

    /**
     * @brief 获取转换结果列表
     * @return 转换结果列表
     */
    const QList<ImageConversionResult>& getResults() const;

    /**
     * @brief 获取支持的格式扩展名列表
     * @param format 图片格式
     * @return 扩展名列表（如 "bmp", "jpg"）
     */
    static QString getFormatExtension(ImageFormat format);

    /**
     * @brief 根据文件扩展名判断格式
     * @param filePath 文件路径
     * @return 图片格式，无法识别返回 FORMAT_BMP
     */
    static ImageFormat detectFormat(const QString& filePath);

    /**
     * @brief 获取格式显示名称
     * @param format 图片格式
     * @return 显示名称（如 "BMP", "JPEG"）
     */
    static QString getFormatName(ImageFormat format);

private:
    QStringList m_sourceFiles;      ///< 源文件列表
    QString m_outputDir;            ///< 输出目录
    ImageFormat m_targetFormat;     ///< 目标格式
    int m_quality;                  ///< JPEG/HEIC质量
    bool m_overwrite;               ///< 是否覆盖
    ImageProgressCallback m_progressCb;  ///< 进度回调
    ImageLogCallback m_logCb;       ///< 日志回调
    const bool* m_pCancel;          ///< 取消标志
    QList<ImageConversionResult> m_results;  ///< 转换结果

    /**
     * @brief 生成输出文件路径
     * @param sourcePath 源文件路径
     * @return 输出文件路径
     */
    QString generateOutputPath(const QString& sourcePath) const;

    /**
     * @brief 转换单个文件
     * @param sourcePath 源文件路径
     * @param outputPath 输出文件路径
     * @return 是否成功
     */
    bool convertSingleFile(const QString& sourcePath, const QString& outputPath);

    /**
     * @brief 输出日志
     * @param msg 日志文本
     */
    void log(const QString& msg) const;

    /**
     * @brief 报告进度
     * @param currentFile 当前文件序号
     * @param totalFiles 总文件数
     * @param percent 当前进度
     */
    void reportProgress(int currentFile, int totalFiles, int percent) const;
};

#endif // IMAGECONVERTERENGINE_H
