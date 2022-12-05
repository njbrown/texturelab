#pragma once

#include <QOpenGLContext>

class QOpenGLFramebufferObject;
class QOpenGLFunctions;
class QOpenGLTexture;
class ShaderCache;

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

    int mipmapCount = -1;

    GLuint lambertianTextureID;
    GLuint ggxTextureID;
    GLuint sheenTextureID;

    GLuint ggxLutTextureID;
    GLuint charlieLutTextureID;

    QOpenGLTexture* inputTexture;
    GLuint inputTextureID;
    GLuint cubemapTextureID;

    QOpenGLFramebufferObject* framebuffer;

    ShaderCache* shaderCache;

    IblSampler();

    void loadPanorama(const QString& path);

    QOpenGLTexture* createCubemap(bool withMipmaps);
    QOpenGLTexture* createLut();

    // generation functions
    void panoramaToCubemap();
    void cubeMapToLambertian();
    void cubeMapToGGX();
    void cubeMapToSheen();

    void sampleGGXLut();
    void sampleCharlieLut();

    void applyFilter(int distribution, float roughness, int targetMipLevel,
                     int targetTexture, int sampleCount, float lodBias = 0.0);
};