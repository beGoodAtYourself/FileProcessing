/**
 * @file ImageConverterWorker.h
 * @brief 图片转换工作线程声明
 *
 * ImageConverterWorker 继承 QThread，在独立线程中执行图片格式转换操作，
 * 避免阻塞 UI 主线程。
 *
 * 采用与 SplitWorker 相同的回调到信号桥接模式：
 *   ImageConverterEngine 使用 C 风格函数指针回调，
 *   而 Qt UI 需要通过信号槽通信。ImageConverterWorker 通过静态指针
 *   将 C 回调转发为 Qt 信号，实现核心逻辑与 UI 的解耦。
 */

#ifndef IMAGECONVERTERWORKER_H
#define IMAGECONVERTERWORKER_H

#include <QThread>
#include <QStringList>
#include "ImageConverterEngine.h"

class ImageConverterWorker : public QThread
{
    Q_OBJECT

public:
    explicit ImageConverterWorker(QObject *parent = 0);
    ~ImageConverterWorker();

    /**
     * @brief 设置源文件路径列表
     * @param files 源文件路径列表
     */
    void setSourceFiles(const QStringList& files);

    /**
     * @brief 设置输出目录
     * @param dir 输出目录路径
     */
    void setOutputDir(const QString& dir);

    /**
     * @brief 设置目标格式
     * @param format 目标图片格式
     */
    void setTargetFormat(ImageFormat format);

    /**
     * @brief 设置 JPEG 质量
     * @param quality 质量参数（1-100）
     */
    void setQuality(int quality);

    /**
     * @brief 设置是否覆盖已存在文件
     * @param overwrite true 覆盖，false 跳过
     */
    void setOverwrite(bool overwrite);

    /**
     * @brief 请求取消转换操作
     *
     * 设置内部取消标志为 true，转换循环将在下次检查时中止。
     */
    void cancel();

    /**
     * @brief 获取转换结果列表
     * @return 转换结果列表
     */
    const QList<ImageConversionResult>& getResults() const;

signals:
    /**
     * @brief 进度变化信号
     * @param currentFile 当前文件序号（从1开始）
     * @param totalFiles  总文件数
     * @param percent     当前文件转换进度（0-100）
     */
    void progressChanged(int currentFile, int totalFiles, int percent);

    /**
     * @brief 日志消息信号
     * @param message 日志文本
     */
    void logMessage(const QString& message);

    /**
     * @brief 转换完成信号
     * @param successCount 成功转换的文件数量
     * @param totalFiles   总文件数
     */
    void conversionFinished(int successCount, int totalFiles);

protected:
    /**
     * @brief 线程入口函数
     *
     * 创建 ImageConverterEngine 实例，配置参数，注册回调，执行转换。
     * 转换完成后发射 conversionFinished 信号。
     */
    void run();

private:
    QStringList m_sourceFiles;      ///< 源文件列表
    QString m_outputDir;            ///< 输出目录
    ImageFormat m_targetFormat;     ///< 目标格式
    int m_quality;                  ///< JPEG质量
    bool m_overwrite;               ///< 是否覆盖
    bool m_cancelFlag;              ///< 取消标志
    QList<ImageConversionResult> m_results;  ///< 转换结果

    /**
     * @brief 当前工作线程实例的静态指针
     */
    static ImageConverterWorker* s_pCurrentWorker;

    /**
     * @brief 进度回调（C 风格静态函数）
     */
    static void progressCallback(int currentFile, int totalFiles, int percent);

    /**
     * @brief 日志回调（C 风格静态函数）
     */
    static void logCallback(const char* message);
};

#endif // IMAGECONVERTERWORKER_H
