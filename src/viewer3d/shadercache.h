#pragma once

#include <QMap>
#include <QVector>

class QOpenGLShaderProgram;

class ShaderCache {
    QMap<QString, QString> shaders;
    QString shaderVersion = "#version 150 core \n";

public:
    void addShaderSource(const QString& shaderName,
                         const QString& shaderSource);
    void addShaderFile(const QString& shaderName, const QString& filePath);

    QString generateShaderSource(const QString& shaderName,
                                 const QVector<QString> defines = {});

private:
    QString getShaderAndResolveIncludes(const QString& shader);
};