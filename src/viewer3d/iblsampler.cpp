#include "iblsampler.h"
#define STB_IMAGE_IMPLEMENTATION
#include "gltf/stb_image.h"

#include <QFile>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

// stbi_loadf_from_memory
// https://stackoverflow.com/questions/32666824/qopengltexture-qt-from-raw-data-freeimage
void IblSampler::loadPanorama(const QString& path)
{
    int width, height, numComponents;
    QFile file(path);
    file.open(QFile::ReadOnly);
    auto byteArray = file.readAll();
    auto textureData = stbi_loadf_from_memory(
        (unsigned char*)byteArray.constData(), byteArray.length(), &width,
        &height, &numComponents, 3);
    QOpenGLTexture* text = new QOpenGLTexture(QOpenGLTexture::Target2D);
    text->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    text->create();

    // given some `width`, `height` and `data_ptr`
    text->setSize(width, height, 3);
    text->setFormat(QOpenGLTexture::RG32F);
    text->allocateStorage();
    text->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, textureData);

    inputTexture = text;
}

// https://stackoverflow.com/questions/50666781/create-cubemap-from-qopenglframebuffer

QOpenGLTexture* IblSampler::createCubemap(bool withMipmaps)
{
    auto cubemap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    if (withMipmaps)
        cubemap->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,
                                  QOpenGLTexture::Linear);
    else
        cubemap->setMinMagFilters(QOpenGLTexture::Linear,
                                  QOpenGLTexture::Linear);

    cubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    cubemap->create();

    cubemap->setSize(textureSize, textureSize, 3);
    // cubemap->setMipLevels()
    cubemap->setFormat(QOpenGLTexture::RGB32F);
    cubemap->allocateStorage();

    return cubemap;
}

QOpenGLTexture* IblSampler::createLut()
{
    auto cubemap = new QOpenGLTexture(QOpenGLTexture::Target2D);
    cubemap->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    cubemap->setWrapMode(QOpenGLTexture::ClampToEdge);
    cubemap->create();

    cubemap->setSize(textureSize, textureSize, 3);
    // cubemap->setMipLevels()
    cubemap->setFormat(QOpenGLTexture::RGBA32F);
    cubemap->allocateStorage();

    return cubemap;
}