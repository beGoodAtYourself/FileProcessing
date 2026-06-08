/**
 * @file HeicDecoder.h
 * @brief HEIC 图片解码器声明
 *
 * HeicDecoder 封装 libheif C API 的解码流程，将 HEIC 文件解码为 QImage。
 * 支持带 Alpha 通道的 HEIC 图像，自动处理色彩空间转换。
 */

#ifndef HEICDECODER_H
#define HEICDECODER_H

#include <QString>
#include <QImage>

/**
 * @brief HEIC 图片解码器
 *
 * 使用 libheif C API 将 HEIC/HEIF 文件解码为 QImage 对象。
 * 自动检测文件类型，处理带/不带 Alpha 通道的图像，
 * 完成色彩空间从 YCbCr 到 RGB 的转换。
 *
 * 典型用法：
 *   QString errorMsg;
 *   QImage image = HeicDecoder::decodeFromFile("photo.heic", &errorMsg);
 *   if (image.isNull()) {
 *       qDebug() << "Decode failed:" << errorMsg;
 *   }
 */
class HeicDecoder
{
public:
    /**
     * @brief 从文件解码 HEIC 为 QImage
     *
     * @param filePath  HEIC 文件路径
     * @param errorMsg  错误信息输出（可选），解码失败时填入详细错误描述
     * @return 解码后的 QImage，失败时返回 null QImage
     */
    static QImage decodeFromFile(const QString& filePath, QString* errorMsg = NULL);

    /**
     * @brief 检查文件是否为有效的 HEIC 格式
     *
     * 通过文件头 magic number 判断，不依赖文件扩展名。
     * HEIC 文件以 "ftyp" 标识开头，常见子类型包括 heic、heix、hevc、hevx 等。
     *
     * @param filePath  文件路径
     * @return true 表示文件是 HEIC 格式，false 表示不是
     */
    static bool isHeicFile(const QString& filePath);

private:
    /**
     * @brief 将 libheif 错误结构转换为可读字符串
     * @param err  libheif 错误结构
     * @return 错误描述字符串
     */
    static QString errorToString(struct heif_error err);

    /**
     * @brief 将 heif_image 中的像素数据转换为 QImage
     *
     * 处理 RGB 和 RGBA 两种布局，完成从 libheif 像素缓冲区
     * 到 QImage 内存格式的转换。
     *
     * @param heifImg  libheif 解码后的图像对象
     * @return 转换后的 QImage
     */
    static QImage convertToQImage(struct heif_image* heifImg);
};

#endif // HEICDECODER_H
