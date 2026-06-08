/**
 * @file HeicEncoder.h
 * @brief HEIC 图片编码器声明
 *
 * HeicEncoder 封装 libheif C API 的编码流程，将 QImage 编码为 HEIC 文件。
 * 支持 HEVC 压缩、质量参数调节、Alpha 通道保留。
 */

#ifndef HEICENCODER_H
#define HEICENCODER_H

#include <QString>
#include <QImage>

/**
 * @brief HEIC 图片编码器
 *
 * 使用 libheif C API 将 QImage 编码为 HEIC 文件。
 * 支持 HEVC 压缩格式，可调节编码质量（0-100），
 * 自动处理带/不带 Alpha 通道的图像。
 *
 * 典型用法：
 *   QString errorMsg;
 *   bool ok = HeicEncoder::encodeToFile("output.heic", image, 50, &errorMsg);
 *   if (!ok) {
 *       qDebug() << "Encode failed:" << errorMsg;
 *   }
 */
class HeicEncoder
{
public:
    /**
     * @brief 默认编码质量（0-100 范围，默认 50）
     */
    static const int DEFAULT_QUALITY = 50;

    /**
     * @brief 将 QImage 编码并保存为 HEIC 文件
     *
     * @param filePath  输出 HEIC 文件路径
     * @param image     要编码的 QImage 对象
     * @param quality   编码质量（0-100），值越大质量越高但文件越大。默认 50。
     * @param errorMsg  错误信息输出（可选），编码失败时填入详细错误描述
     * @return true 编码成功，false 编码失败
     */
    static bool encodeToFile(const QString& filePath, const QImage& image,
                             int quality = DEFAULT_QUALITY, QString* errorMsg = NULL);

private:
    /**
     * @brief 将 libheif 错误结构转换为可读字符串
     * @param err  libheif 错误结构
     * @return 错误描述字符串
     */
    static QString errorToString(struct heif_error err);

    /**
     * @brief 将 QImage 的像素数据写入 heif_image 对象
     *
     * 处理 RGB 和 RGBA 两种 QImage 格式，完成从 QImage 内存布局
     * 到 libheif 像素平面的转换。
     *
     * @param image   源 QImage
     * @param heifImg 目标 heif_image 对象
     * @return true 成功，false 失败
     */
    static bool fillHeifImageFromQImage(const QImage& image, struct heif_image* heifImg);
};

#endif // HEICENCODER_H
