#pragma once

#include <QImage>
#include <QString>
#include <vector>

class QOpenGLFramebufferObject;

struct ExportResult {
    bool success;
    QString errorMessage;
};

class Exporter {
public:
    Exporter();

    // Export a single texture to a file
    ExportResult
    exportTexture(QOpenGLFramebufferObject* texture, const QString& filePath,
                  int precision, // 0 = 8-bit, 1 = 16-bit
                  int components // 0=RGBA, 1=RGB, 2=R, 3=G, 4=B, 5=A
    );

private:
    QImage convertToImage(const std::vector<float>& floatData, int width,
                          int height, int precision, int components);
};
