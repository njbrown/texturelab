#include "exporter.h"

#include <QColor>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

Exporter::Exporter() {}

ExportResult Exporter::exportTexture(QOpenGLFramebufferObject* texture,
                                     const QString& filePath, int precision,
                                     int components)
{
    ExportResult result;
    result.success = false;

    if (!texture) {
        result.errorMessage = "Texture is null";
        return result;
    }

    int width = texture->width();
    int height = texture->height();

    // Bind the FBO and read pixels
    texture->bind();
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();

    // Read as float data (since texture is GL_RGBA32F)
    std::vector<float> floatData(width * height * 4);
    gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, floatData.data());
    texture->release();

    // Convert to QImage
    QImage img =
        convertToImage(floatData, width, height, precision, components);

    // Flip image vertically (OpenGL reads bottom-to-top)
    img = img.mirrored(false, true);

    // Save image
    if (img.save(filePath)) {
        result.success = true;
        qDebug() << "Exported:" << filePath;
    }
    else {
        result.errorMessage = "Failed to save image to " + filePath;
        qDebug() << result.errorMessage;
    }

    return result;
}

QImage Exporter::convertToImage(const std::vector<float>& floatData, int width,
                                int height, int precision, int components)
{
    QImage img;

    // Determine number of channels for output
    int outputChannels = 4;
    if (components == 1)
        outputChannels = 3; // RGB
    else if (components >= 2 && components <= 5)
        outputChannels = 1; // Single channel

    if (precision == 1) {
        // 16-bit precision
        if (outputChannels == 1) {
            // Grayscale 16-bit
            img = QImage(width, height, QImage::Format_Grayscale16);

            for (int y = 0; y < height; y++) {
                quint16* scanLine = reinterpret_cast<quint16*>(img.scanLine(y));
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;
                    float value = 0.0f;

                    // Extract the selected channel
                    if (components == 2)
                        value = floatData[idx + 0]; // Red
                    else if (components == 3)
                        value = floatData[idx + 1]; // Green
                    else if (components == 4)
                        value = floatData[idx + 2]; // Blue
                    else if (components == 5)
                        value = floatData[idx + 3]; // Alpha

                    // Convert to 16-bit (0-65535)
                    scanLine[x] =
                        qBound(0, static_cast<int>(value * 65535.0f), 65535);
                }
            }
        }
        else if (outputChannels == 3) {
            // RGB 16-bit
            img = QImage(width, height, QImage::Format_RGBX64);

            for (int y = 0; y < height; y++) {
                QRgba64* scanLine = reinterpret_cast<QRgba64*>(img.scanLine(y));
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;

                    quint16 r = qBound(
                        0, static_cast<int>(floatData[idx + 0] * 65535.0f),
                        65535);
                    quint16 g = qBound(
                        0, static_cast<int>(floatData[idx + 1] * 65535.0f),
                        65535);
                    quint16 b = qBound(
                        0, static_cast<int>(floatData[idx + 2] * 65535.0f),
                        65535);

                    scanLine[x] = qRgba64(r, g, b, 65535);
                }
            }
        }
        else {
            // RGBA 16-bit
            img = QImage(width, height, QImage::Format_RGBA64);

            for (int y = 0; y < height; y++) {
                QRgba64* scanLine = reinterpret_cast<QRgba64*>(img.scanLine(y));
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;

                    quint16 r = qBound(
                        0, static_cast<int>(floatData[idx + 0] * 65535.0f),
                        65535);
                    quint16 g = qBound(
                        0, static_cast<int>(floatData[idx + 1] * 65535.0f),
                        65535);
                    quint16 b = qBound(
                        0, static_cast<int>(floatData[idx + 2] * 65535.0f),
                        65535);
                    quint16 a = qBound(
                        0, static_cast<int>(floatData[idx + 3] * 65535.0f),
                        65535);

                    scanLine[x] = qRgba64(r, g, b, a);
                }
            }
        }
    }
    else {
        // 8-bit precision
        if (outputChannels == 1) {
            // Grayscale 8-bit
            img = QImage(width, height, QImage::Format_Grayscale8);

            for (int y = 0; y < height; y++) {
                quint8* scanLine = reinterpret_cast<quint8*>(img.scanLine(y));
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;
                    float value = 0.0f;

                    // Extract the selected channel
                    if (components == 2)
                        value = floatData[idx + 0]; // Red
                    else if (components == 3)
                        value = floatData[idx + 1]; // Green
                    else if (components == 4)
                        value = floatData[idx + 2]; // Blue
                    else if (components == 5)
                        value = floatData[idx + 3]; // Alpha

                    // Convert to 8-bit (0-255)
                    scanLine[x] =
                        qBound(0, static_cast<int>(value * 255.0f), 255);
                }
            }
        }
        else if (outputChannels == 3) {
            // RGB 8-bit
            img = QImage(width, height, QImage::Format_RGB888);

            for (int y = 0; y < height; y++) {
                uchar* scanLine = img.scanLine(y);
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;

                    quint8 r = qBound(
                        0, static_cast<int>(floatData[idx + 0] * 255.0f), 255);
                    quint8 g = qBound(
                        0, static_cast<int>(floatData[idx + 1] * 255.0f), 255);
                    quint8 b = qBound(
                        0, static_cast<int>(floatData[idx + 2] * 255.0f), 255);

                    scanLine[x * 3 + 0] = r;
                    scanLine[x * 3 + 1] = g;
                    scanLine[x * 3 + 2] = b;
                }
            }
        }
        else {
            // RGBA 8-bit
            img = QImage(width, height, QImage::Format_RGBA8888);

            for (int y = 0; y < height; y++) {
                uchar* scanLine = img.scanLine(y);
                for (int x = 0; x < width; x++) {
                    int idx = (y * width + x) * 4;

                    quint8 r = qBound(
                        0, static_cast<int>(floatData[idx + 0] * 255.0f), 255);
                    quint8 g = qBound(
                        0, static_cast<int>(floatData[idx + 1] * 255.0f), 255);
                    quint8 b = qBound(
                        0, static_cast<int>(floatData[idx + 2] * 255.0f), 255);
                    quint8 a = qBound(
                        0, static_cast<int>(floatData[idx + 3] * 255.0f), 255);

                    scanLine[x * 4 + 0] = r;
                    scanLine[x * 4 + 1] = g;
                    scanLine[x * 4 + 2] = b;
                    scanLine[x * 4 + 3] = a;
                }
            }
        }
    }

    return img;
}
