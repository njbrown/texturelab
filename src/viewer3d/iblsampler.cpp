#include "iblsampler.h"
#include "shadercache.h"
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "gltf/stb_image.h"

#include <QFile>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

enum class VertexUsage : int {
    Position = 0,
    Color = 1,
    TexCoord0 = 2,
    TexCoord1 = 3,
    TexCoord2 = 4,
    TexCoord3 = 5,
    Normal = 6,
    Tangent = 7,
    Count = 8
};

IblSampler::IblSampler()
{
    shaderCache = new ShaderCache();
    shaderCache->addShaderFile("fullscreen.vert", ":assets/fullscreen.vert");
    shaderCache->addShaderFile("panorama_to_cubemap.frag",
                               ":assets/panorama_to_cubemap.frag");
    shaderCache->addShaderFile("ibl_filtering.frag",
                               ":assets/ibl_filtering.frag");
}

void IblSampler::init(const QString& panoramaPath)
{
    mipmapLevels =
        std::floor(std::log2(this->textureSize)) + 1 - this->lowestMipLevel;

    framebuffer = new QOpenGLFramebufferObject(textureSize, textureSize);
    if (!framebuffer->isValid()) {
        qFatal("FBO could not be created");
    }
    // setup vbo
    vao = new QOpenGLVertexArrayObject;
    if (vao->create())
        vao->bind();

    // setup vertex array
    QVector<float> data;
    // TRIANGLE 1
    data.append(-1);
    data.append(-1);
    data.append(0);

    data.append(0);
    data.append(0);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    // TRIANGLE 2
    data.append(-1);
    data.append(1);
    data.append(0);

    data.append(0);
    data.append(1);

    data.append(1);
    data.append(-1);
    data.append(0);

    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);
    data.append(0);

    data.append(1);
    data.append(1);

    vbo = new QOpenGLBuffer;
    vbo->create();
    vbo->bind();
    vbo->allocate(data.constData(), data.count() * sizeof(float));
    vbo->release();

    this->loadPanorama(panoramaPath);

    cubemapTexture = this->createCubemap(true);
    lambertianTexture = this->createCubemap(false);
    ggxTexture = this->createCubemap(true);
    sheenTexture = this->createCubemap(true);

    ggxLutTexture = this->createLut();
    charlieLutTexture = this->createLut();
}

void IblSampler::filterAll()
{
    this->panoramaToCubemap();
    this->cubeMapToLambertian();
    this->cubeMapToGGX();
    this->cubeMapToSheen();

    this->sampleGGXLut();
    this->sampleCharlieLut();
}

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
    auto texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->create();

    texture->setSize(textureSize, textureSize, 3);
    // cubemap->setMipLevels()
    texture->setFormat(QOpenGLTexture::RGBA32F);
    texture->allocateStorage();

    return texture;
}

void IblSampler::panoramaToCubemap()
{
    // todo: cleanup old cubemap

    auto vertSource = shaderCache->generateShaderSource("fullscreen.vert");
    auto fragSource =
        shaderCache->generateShaderSource("panorama_to_cubemap.frag");

    auto shader = createShader(vertSource, fragSource);

    // render each face
    for (int i = 0; i < 6; i++) {
        gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->handle());
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   cubemapTexture->textureId(), 0);

        gl->glViewport(0, 0, textureSize, textureSize);
        gl->glClearColor(0, 0, 0, 1);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->bind();
        inputTexture->bind(0);

        shader->setUniformValue("u_panorama", 0);
        shader->setUniformValue("u_currentFace", i);

        vbo->bind();
        gl->glEnableVertexAttribArray((int)VertexUsage::Position);
        gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
        gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float), nullptr);
        gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(3 * sizeof(float)));

        gl->glDrawArrays(GL_TRIANGLES, 0, 6);

        vbo->release();
    }

    cubemapTexture->bind();
    cubemapTexture->generateMipMaps();
    cubemapTexture->release();

    // gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    auto ctx = QOpenGLContext::currentContext();
    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
}

QOpenGLShaderProgram* IblSampler::createShader(const QString& vertSource,
                                               const QString& fragSource)
{
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment);
    auto program = new QOpenGLShaderProgram;

    if (!vshader->compileSourceCode(vertSource)) {
        qDebug() << "VERTEX SHADER ERROR";
        qDebug() << vshader->log();
    }

    if (!fshader->compileSourceCode(fragSource)) {
        qDebug() << "FRAGMENT SHADER ERROR";
        qDebug() << fshader->log();
    }

    program->removeAllShaders();

    program->addShader(vshader);
    program->addShader(fshader);

    program->bindAttributeLocation("a_pos", (int)VertexUsage::Position);
    program->bindAttributeLocation("a_color", (int)VertexUsage::Color);
    program->bindAttributeLocation("a_texCoord", (int)VertexUsage::TexCoord0);

    if (!program->link()) {
        qDebug() << "SHADER LINK ERROR";
        qDebug() << program->log();
    }

    return program;
}

void IblSampler::applyFilter(int distribution, float roughness,
                             int targetMipLevel, int targetTexture,
                             int sampleCount, float lodBias)
{
    auto currentTextureSize = this->textureSize >> targetMipLevel;

    auto vertSource = shaderCache->generateShaderSource("fullscreen.vert");
    auto fragSource = shaderCache->generateShaderSource("ibl_filtering.frag");

    auto shader = createShader(vertSource, fragSource);

    // render each face
    for (int i = 0; i < 6; i++) {
        gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->handle());
        gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   targetTexture, targetMipLevel);

        gl->glViewport(0, 0, currentTextureSize, currentTextureSize);
        gl->glClearColor(0, 0, 0, 1);
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->bind();
        cubemapTexture->bind(0);
        shader->setUniformValue("uCubeMap", 0);

        shader->setUniformValue("u_roughness", roughness);
        shader->setUniformValue("u_sampleCount", sampleCount);
        shader->setUniformValue("u_width", textureSize);
        shader->setUniformValue("u_lodBias", lodBias);
        shader->setUniformValue("u_distribution", distribution);
        shader->setUniformValue("u_currentFace", i);
        shader->setUniformValue("u_isGeneratingLUT", 0);

        vbo->bind();
        gl->glEnableVertexAttribArray((int)VertexUsage::Position);
        gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
        gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float), nullptr);
        gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                                  GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(3 * sizeof(float)));

        gl->glDrawArrays(GL_TRIANGLES, 0, 6);

        vbo->release();
    }

    // gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    auto ctx = QOpenGLContext::currentContext();
    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
}

void IblSampler::cubeMapToLambertian()
{
    this->applyFilter(0, 0.0, 0, this->lambertianTexture->textureId(),
                      this->lambertianSampleCount);
}

void IblSampler::cubeMapToGGX()
{
    for (int currentMipLevel = 0; currentMipLevel <= this->mipmapLevels;
         ++currentMipLevel) {
        auto roughness = (currentMipLevel) / (this->mipmapLevels - 1);
        this->applyFilter(1, roughness, currentMipLevel,
                          this->ggxTexture->textureId(), this->ggxSampleCount);
    }
}
void IblSampler::cubeMapToSheen()
{
    for (auto currentMipLevel = 0; currentMipLevel <= this->mipmapLevels;
         ++currentMipLevel) {
        auto roughness = (currentMipLevel) / (this->mipmapLevels - 1);
        this->applyFilter(2, roughness, currentMipLevel,
                          this->sheenTexture->textureId(),
                          this->sheenSamplCount);
    }
}

void IblSampler::sampleLut(int distribution, int targetTextureId,
                           int currentTextureSize)
{
    auto vertSource = shaderCache->generateShaderSource("fullscreen.vert");
    auto fragSource = shaderCache->generateShaderSource("ibl_filtering.frag");

    auto shader = createShader(vertSource, fragSource);

    // render each face
    gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->handle());
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, targetTextureId, 0);

    gl->glViewport(0, 0, currentTextureSize, currentTextureSize);
    gl->glClearColor(0, 0, 0, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->bind();
    cubemapTexture->bind(0);
    shader->setUniformValue("uCubeMap", 0);

    shader->setUniformValue("u_roughness", 0.0f);
    shader->setUniformValue("u_sampleCount", 512);
    shader->setUniformValue("u_width", 0.0f);
    shader->setUniformValue("u_lodBias", 0.0f);
    shader->setUniformValue("u_distribution", distribution);
    shader->setUniformValue("u_currentFace", 0);
    shader->setUniformValue("u_isGeneratingLUT", 1);

    vbo->bind();
    gl->glEnableVertexAttribArray((int)VertexUsage::Position);
    gl->glEnableVertexAttribArray((int)VertexUsage::TexCoord0);
    gl->glVertexAttribPointer((int)VertexUsage::Position, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), nullptr);
    gl->glVertexAttribPointer((int)VertexUsage::TexCoord0, 2, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));

    gl->glDrawArrays(GL_TRIANGLES, 0, 6);

    vbo->release();

    // gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    auto ctx = QOpenGLContext::currentContext();
    gl->glBindFramebuffer(GL_FRAMEBUFFER, ctx->defaultFramebufferObject());
}

void IblSampler::sampleGGXLut()
{
    this->ggxLutTexture = this->createLut();
    this->sampleLut(1, this->ggxLutTexture->textureId(), this->lutResolution);
}

void IblSampler::sampleCharlieLut()
{
    this->charlieLutTexture = this->createLut();
    this->sampleLut(2, this->charlieLutTexture->textureId(),
                    this->lutResolution);
}