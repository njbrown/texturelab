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
    float intensityScale = 1.0;

    int mipmapLevels = -1;

    // QOpenGLTexture* lambertianTexture;
    // QOpenGLTexture* ggxTexture;
    // QOpenGLTexture* sheenTexture;
    GLuint lambertianTextureID = 0;
    GLuint ggxTextureID = 0;
    GLuint sheenTextureID = 0;

    // QOpenGLTexture* ggxLutTexture;
    // QOpenGLTexture* charlieLutTexture;
    GLuint ggxLutTextureID = 0;
    GLuint charlieLutTextureID = 0;

    QOpenGLTexture* inputTexture = nullptr;
    // QOpenGLTexture* cubemapTexture;
    // GLuint inputTextureID;
    GLuint cubemapTextureID = 0;

    QOpenGLFramebufferObject* framebuffer = nullptr;
    QOpenGLVertexArrayObject* vao = nullptr;
    QOpenGLBuffer* vbo = nullptr;

    // Frees everything init() allocated (~70 MB of float cubemaps and LUTs).
    // Called at the start of init(), so switching environments doesn't stack
    // a new set on top of the old. GL context must be current.
    void releaseResources();

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