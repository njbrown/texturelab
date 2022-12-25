#pragma once

#include <QOpenGLContext>

class QOpenGLFramebufferObject;
class QOpenGLFunctions;
class QOpenGLTexture;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class ShaderCache;
class QOpenGLShaderProgram;

// reference:
// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/source/ibl_sampler.js

class IblSampler {
public:
    QOpenGLFunctions* gl;

    int textureSize = 256;
    int ggxSampleCount = 1024;
    int lambertianSampleCount = 2048;
    int sheenSamplCount = 64;
    float lodBias = 0.0;
    int lowestMipLevel = 4;
    int lutResolution = 1024;

    int mipmapLevels = -1;

    // QOpenGLTexture* lambertianTexture;
    // QOpenGLTexture* ggxTexture;
    // QOpenGLTexture* sheenTexture;
    GLuint lambertianTextureID;
    GLuint ggxTextureID;
    GLuint sheenTextureID;

    // QOpenGLTexture* ggxLutTexture;
    // QOpenGLTexture* charlieLutTexture;
    GLuint ggxLutTextureID;
    GLuint charlieLutTextureID;

    QOpenGLTexture* inputTexture;
    // QOpenGLTexture* cubemapTexture;
    // GLuint inputTextureID;
    GLuint cubemapTextureID;

    QOpenGLFramebufferObject* framebuffer;
    QOpenGLVertexArrayObject* vao;
    QOpenGLBuffer* vbo;

    ShaderCache* shaderCache;

    IblSampler();

    void loadPanorama(const QString& path);

    // QOpenGLTexture* createCubemap(bool withMipmaps);
    // QOpenGLTexture* createLut();
    GLuint createCubemap(bool withMipmaps);
    GLuint createLut();

    // generation functions
    void panoramaToCubemap();
    void cubeMapToLambertian();
    void cubeMapToGGX();
    void cubeMapToSheen();

    void sampleGGXLut();
    void sampleCharlieLut();

    void applyFilter(int distribution, float roughness, int targetMipLevel,
                     int targetTexture, int sampleCount, float lodBias = 0.0);
    void sampleLut(int distribution, int targetTextureId,
                   int currentTextureSize);

    void init(const QString& panoramaPath);

    QOpenGLShaderProgram* createShader(const QString& vertSource,
                                       const QString& fragSource);

    void filterAll();
};