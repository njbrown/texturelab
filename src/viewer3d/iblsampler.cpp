#include "iblsampler.h"
#include "shadercache.h"

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

void IblSampler::init(const QString& panoramaPath)
{
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

    this->panoramaToCubemap();
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