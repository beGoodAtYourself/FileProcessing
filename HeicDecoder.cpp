/**
 * @file HeicDecoder.cpp
 * @brief HEIC 图片解码器实现
 *
 * 基于 libheif C API 实现 HEIC 文件解码，将像素数据转换为 QImage 格式。
 */

#include "HeicDecoder.h"
#include <libheif/heif.h>

#include <QFile>
#include <QFileInfo>
#include <QDebug>

// ============================================================================
// 公共接口
// ============================================================================

QImage HeicDecoder::decodeFromFile(const QString& filePath, QString* errorMsg)
{
    // 检查文件是否存在
    if (!QFile::exists(filePath))
    {
        if (errorMsg)
        {
            *errorMsg = QString("File does not exist: %1").arg(filePath);
        }
        return QImage();
    }

    // 创建 heif 上下文
    heif_context* ctx = heif_context_alloc();
    if (ctx == NULL)
    {
        if (errorMsg)
        {
            *errorMsg = "Failed to allocate HEIF context";
        }
        return QImage();
    }

    heif_image_handle* handle = NULL;
    heif_image* heifImg = NULL;
    QImage result;

    do
    {
        // 读取文件
        heif_error err = heif_context_read_from_file(ctx, filePath.toUtf8().constData(), NULL);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to read HEIC file: %1").arg(errorToString(err));
            }
            break;
        }

        // 获取主图像句柄
        err = heif_context_get_primary_image_handle(ctx, &handle);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to get primary image handle: %1").arg(errorToString(err));
            }
            break;
        }

        // 判断是否有 Alpha 通道，选择合适的输出格式
        int hasAlpha = heif_image_handle_has_alpha_channel(handle);

        // 解码图像
        // 使用 interleaved RGB/RGBA 格式输出，便于直接构建 QImage
        enum heif_chroma chroma = hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB;
        enum heif_colorspace colorspace = heif_colorspace_RGB;

        err = heif_decode_image(handle, &heifImg, colorspace, chroma, NULL);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to decode HEIC image: %1").arg(errorToString(err));
            }
            break;
        }

        if (heifImg == NULL)
        {
            if (errorMsg)
            {
                *errorMsg = "Decoded image is null";
            }
            break;
        }

        // 转换为 QImage
        result = convertToQImage(heifImg);

        if (result.isNull() && errorMsg)
        {
            *errorMsg = "Failed to convert HEIC pixel data to QImage";
        }

    } while (false);

    // 清理资源
    if (heifImg != NULL)
    {
        heif_image_release(heifImg);
    }
    if (handle != NULL)
    {
        heif_image_handle_release(handle);
    }
    if (ctx != NULL)
    {
        heif_context_free(ctx);
    }

    return result;
}

bool HeicDecoder::isHeicFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    // HEIC 文件头至少读取 12 字节用于检测
    // 格式：[4字节size][4字节"ftyp"][4字节子类型(如heic/heix/hevc等)]
    const int CHECK_LEN = 12;
    QByteArray header = file.read(CHECK_LEN);
    file.close();

    if (header.size() < CHECK_LEN)
    {
        return false;
    }

    // 检查 ftyp box
    if (header.mid(4, 4) != "ftyp")
    {
        return false;
    }

    // 检查子类型
    QByteArray subType = header.mid(8, 4);
    if (subType == "heic" || subType == "heix" ||
        subType == "hevc" || subType == "hevx" ||
        subType == "mif1" || subType == "msf1")
    {
        return true;
    }

    // 使用 libheif API 进行更精确的检测
    const uint8_t* data = reinterpret_cast<const uint8_t*>(header.constData());
    enum heif_filetype_result result = heif_check_filetype(data, header.size());
    return (result == heif_filetype_yes_supported || result == heif_filetype_maybe);
}

// ============================================================================
// 私有方法
// ============================================================================

QString HeicDecoder::errorToString(struct heif_error err)
{
    const char* codeStr = "Unknown";
    switch (err.code)
    {
    case heif_error_Ok: codeStr = "Ok"; break;
    case heif_error_Input_does_not_exist: codeStr = "Input does not exist"; break;
    case heif_error_Invalid_input: codeStr = "Invalid input"; break;
    case heif_error_Unsupported_filetype: codeStr = "Unsupported filetype"; break;
    case heif_error_Unsupported_feature: codeStr = "Unsupported feature"; break;
    case heif_error_Usage_error: codeStr = "Usage error"; break;
    case heif_error_Memory_allocation_error: codeStr = "Memory allocation error"; break;
    case heif_error_Decoder_plugin_error: codeStr = "Decoder plugin error"; break;
    case heif_error_Encoder_plugin_error: codeStr = "Encoder plugin error"; break;
    case heif_error_Encoding_error: codeStr = "Encoding error"; break;
    case heif_error_Color_profile_does_not_exist: codeStr = "Color profile does not exist"; break;
    default: break;
    }

    QString msg = QString("[%1] %2").arg(codeStr).arg(err.message ? err.message : "No detail");
    return msg;
}

QImage HeicDecoder::convertToQImage(heif_image* heifImg)
{
    if (heifImg == NULL)
    {
        return QImage();
    }

    int width = heif_image_get_primary_width(heifImg);
    int height = heif_image_get_primary_height(heifImg);

    if (width <= 0 || height <= 0 || width > 65535 || height > 65535)
    {
        return QImage();
    }

    // 获取 interleaved RGB/RGBA 像素数据
    // 对于 interleaved 格式，所有像素数据在 heif_channel_interleaved 平面中
    int stride = 0;
    const uint8_t* data = heif_image_get_plane_readonly(heifImg, heif_channel_interleaved, &stride);

    if (data == NULL)
    {
        // 回退：尝试从 R 通道获取（planar RGB 格式）
        data = heif_image_get_plane_readonly(heifImg, heif_channel_R, &stride);
        if (data == NULL)
        {
            // 最终回退：尝试从 Y 通道获取（YCbCr 格式）
            data = heif_image_get_plane_readonly(heifImg, heif_channel_Y, &stride);
            if (data == NULL)
            {
                return QImage();
            }
        }
    }

    // 根据 stride 计算每行字节数和像素格式
    // stride 通常为 width * 3 (RGB) 或 width * 4 (RGBA)
    int bytesPerPixel = stride / width;
    QImage::Format format;

    if (bytesPerPixel == 4)
    {
        // RGBA 格式
        format = QImage::Format_RGBA8888;
    }
    else if (bytesPerPixel == 3)
    {
        // RGB 格式 - 需要转换为 RGB32 以便 Qt 处理
        format = QImage::Format_RGB888;
    }
    else
    {
        // 无法确定像素格式，尝试按 RGB 处理
        bytesPerPixel = 3;
        format = QImage::Format_RGB888;
    }

    // 从像素数据构造 QImage
    QImage result(data, width, height, stride, format);

    if (result.isNull())
    {
        return QImage();
    }

    // 深拷贝，确保数据独立于 libheif 的内存管理
    QImage copy = result.copy();

    return copy;
}
