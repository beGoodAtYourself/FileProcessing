/**
 * @file HeicEncoder.cpp
 * @brief HEIC 图片编码器实现
 *
 * 基于 libheif C API 实现 QImage 到 HEIC 文件的编码。
 * 支持 HEVC 压缩、质量参数设置、Alpha 通道处理。
 */

#include "HeicEncoder.h"
#include <libheif/heif.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// ============================================================================
// 公共接口
// ============================================================================

bool HeicEncoder::encodeToFile(const QString& filePath, const QImage& image,
                                int quality, QString* errorMsg)
{
    // 检查输入图像
    if (image.isNull())
    {
        if (errorMsg)
        {
            *errorMsg = "Input image is null";
        }
        return false;
    }

    if (filePath.isEmpty())
    {
        if (errorMsg)
        {
            *errorMsg = "Output file path is empty";
        }
        return false;
    }

    // 确保输出目录存在
    QFileInfo outputInfo(filePath);
    QDir outputDir = outputInfo.absoluteDir();
    if (!outputDir.exists())
    {
        if (!outputDir.mkpath("."))
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to create output directory: %1").arg(outputDir.absolutePath());
            }
            return false;
        }
    }

    // 限制质量参数范围
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;

    heif_context* ctx = NULL;
    heif_encoder* encoder = NULL;
    heif_image* heifImg = NULL;
    heif_encoding_options* options = NULL;

    bool success = false;

    do
    {
        // 创建 heif 上下文
        ctx = heif_context_alloc();
        if (ctx == NULL)
        {
            if (errorMsg)
            {
                *errorMsg = "Failed to allocate HEIF context";
            }
            break;
        }

        // 获取 HEVC 编码器
        heif_error err = heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to get HEVC encoder: %1").arg(errorToString(err));
            }
            break;
        }

        // 设置编码质量
        err = heif_encoder_set_lossy_quality(encoder, quality);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to set encoder quality: %1").arg(errorToString(err));
            }
            break;
        }

        // 创建 heif_image 并填充像素数据
        bool hasAlpha = image.hasAlphaChannel();

        enum heif_chroma chroma = hasAlpha ? heif_chroma_interleaved_RGBA : heif_chroma_interleaved_RGB;
        enum heif_colorspace colorspace = heif_colorspace_RGB;

        err = heif_image_create(image.width(), image.height(), colorspace, chroma, &heifImg);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to create HEIF image: %1").arg(errorToString(err));
            }
            break;
        }

        // 添加像素平面
        // 对于 interleaved 格式，只需添加一个 heif_channel_interleaved 平面
        // bit_depth=8 表示每个颜色通道8位（不是总位深）
        int bitDepth = 8;
        err = heif_image_add_plane(heifImg, heif_channel_interleaved, image.width(), image.height(), bitDepth);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to add image plane: %1").arg(errorToString(err));
            }
            break;
        }

        // 填充像素数据
        if (!fillHeifImageFromQImage(image, heifImg))
        {
            if (errorMsg)
            {
                *errorMsg = "Failed to fill pixel data from QImage";
            }
            break;
        }

        // 设置编码选项（使用库函数分配，确保结构体版本兼容）
        options = heif_encoding_options_alloc();
        if (options == NULL)
        {
            if (errorMsg)
            {
                *errorMsg = "Failed to allocate encoding options";
            }
            break;
        }
        options->save_alpha_channel = hasAlpha ? 1 : 0;

        // 编码图像
        // heif_context_encode_image 参数顺序：ctx, image, encoder, options, out_image_handle
        err = heif_context_encode_image(ctx, heifImg, encoder, options, NULL);
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to encode HEIC image: %1").arg(errorToString(err));
            }
            break;
        }

        // 写入文件
        err = heif_context_write_to_file(ctx, filePath.toUtf8().constData());
        if (err.code != heif_error_Ok)
        {
            if (errorMsg)
            {
                *errorMsg = QString("Failed to write HEIC file: %1").arg(errorToString(err));
            }
            break;
        }

        success = true;

    } while (false);

    // 清理资源
    if (options != NULL)
    {
        heif_encoding_options_free(options);
    }
    if (heifImg != NULL)
    {
        heif_image_release(heifImg);
    }
    if (encoder != NULL)
    {
        heif_encoder_release(encoder);
    }
    if (ctx != NULL)
    {
        heif_context_free(ctx);
    }

    return success;
}

// ============================================================================
// 私有方法
// ============================================================================

QString HeicEncoder::errorToString(struct heif_error err)
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

bool HeicEncoder::fillHeifImageFromQImage(const QImage& image, heif_image* heifImg)
{
    if (heifImg == NULL || image.isNull())
    {
        return false;
    }

    int width = image.width();
    int height = image.height();
    bool hasAlpha = image.hasAlphaChannel();

    // 将 QImage 转换为适合的格式
    QImage::Format targetFormat = hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
    QImage converted = image.convertToFormat(targetFormat);

    if (converted.isNull())
    {
        return false;
    }

    // 获取 heif_image 的像素平面（interleaved 格式使用 heif_channel_interleaved）
    int stride = 0;
    uint8_t* planeData = heif_image_get_plane(heifImg, heif_channel_interleaved, &stride);
    if (planeData == NULL)
    {
        return false;
    }

    // 复制像素数据
    // QImage 和 heif_image_interleaved 的内存布局一致：
    //   RGB:  [R, G, B, R, G, B, ...]
    //   RGBA: [R, G, B, A, R, G, B, A, ...]
    int bytesPerPixel = hasAlpha ? 4 : 3;
    int srcBytesPerLine = converted.bytesPerLine();

    for (int y = 0; y < height; ++y)
    {
        const unsigned char* srcLine = converted.constScanLine(y);
        unsigned char* dstLine = planeData + y * stride;

        // 如果源和目标每行字节数一致，直接整行复制
        int rowBytes = width * bytesPerPixel;
        if (rowBytes <= stride && rowBytes <= srcBytesPerLine)
        {
            memcpy(dstLine, srcLine, rowBytes);
        }
        else
        {
            // 逐像素复制
            for (int x = 0; x < width; ++x)
            {
                int srcOffset = x * bytesPerPixel;
                int dstOffset = x * bytesPerPixel;
                for (int c = 0; c < bytesPerPixel; ++c)
                {
                    dstLine[dstOffset + c] = srcLine[srcOffset + c];
                }
            }
        }
    }

    return true;
}
