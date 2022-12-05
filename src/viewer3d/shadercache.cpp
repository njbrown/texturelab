#include "shadercache.h"
#include <QFile>

void ShaderCache::addShaderSource(const QString& shaderName,
                                  const QString& shaderSource)
{
    shaders[shaderName] = shaderSource;
}

void ShaderCache::addShaderFile(const QString& shaderName,
                                const QString& filePath)
{
    QFile file(filePath);
    file.open(QFile::ReadOnly);

    auto source = file.readAll();
    this->addShaderSource(shaderName, source);

    file.close();
}

QString ShaderCache::generateShaderSource(const QString& shaderName,
                                          const QVector<QString> defines = {})
{
    QString flags = "";
    for (auto define : defines) {
        flags += "#define " + define + "\n";
    }

    auto source = getShaderAndResolveIncludes(shaderName);
    auto finalSource = shaderVersion + flags + source;
}

QString ShaderCache::getShaderAndResolveIncludes(const QString& shader)
{
    auto source = this->shaders[shader];

    // https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/Renderer/shader_cache.js#L15
    // todo: re-add changes to shaders?
    for (auto shaderFile : this->shaders.keys()) {
        auto pattern = "#include <" + shaderFile + ">";

        if (source.contains(pattern)) {
            auto shaderSource = this->shaders[shaderFile];
            source.replace(pattern, shaderSource);
        }
    }

    return source;
}