#include "../../graphics/renderworker.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>
#include <vector>

// Constants for Euclidean Distance Transform
static const double INF = 1e20;
static const float VALUE_MAX = 15360.0f;

// Forward declarations for EDT functions
static void edt(std::vector<double>& data, int width, int height,
                std::vector<double>& f, std::vector<uint16_t>& v,
                std::vector<double>& z);

static void edt1d(std::vector<double>& grid, int offset, int stride, int length,
                  std::vector<double>& f, std::vector<uint16_t>& v,
                  std::vector<double>& z);

void BevelNode::init()
{
    this->title = "Bevel";
    this->addInput("image");
    this->addFloatProp("distance", "Distance", 50.0, 0.0, 100.0, 0.01);

    // This node uses CPU processing instead of GPU shader
    this->usesCpuProcessing = true;

    // Set a passthrough shader initially (not used, but required for
    // initialization)
    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		)"""";
    this->setShaderSource(source);
}

void BevelNode::cpuProcess(void* glPtr, const RenderCommand& command)
{
    // Cast to QOpenGLFunctions_3_2_Core
    auto gl = static_cast<QOpenGLFunctions_3_2_Core*>(glPtr);

    // Get the first input texture if available
    GLuint inputTextureId = 0;
    if (!command.inputs.isEmpty()) {
        inputTextureId = command.inputs[0].textureId;
    }

    if (inputTextureId == 0)
        return;

    int width = command.textureWidth;
    int height = command.textureHeight;

    // Allocate buffers
    int gridSize = width * height;
    std::vector<float> readPixels(gridSize * 4);
    std::vector<float> resultPixels(gridSize * 4);

    // Read pixels from input texture
    GLuint fbo;
    gl->glGenFramebuffers(1, &fbo);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, inputTextureId, 0);

    if (gl->glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE) {
        gl->glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT,
                         readPixels.data());
    }

    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl->glDeleteFramebuffers(1, &fbo);

    // Allocate working arrays
    int maxSize = std::max(width, height);
    std::vector<double> f(maxSize * 3);
    std::vector<double> z(maxSize * 3 + 1);
    std::vector<uint16_t> v(maxSize * 3);

    std::vector<double> gridOuter(gridSize);
    std::vector<double> gridInner(gridSize);
    std::vector<double> grid(gridSize);

    // Convert pixels to distance fields
    for (int i = 0; i < gridSize; i++) {
        float a = readPixels[i * 4 + 0]; // Use red channel

        gridOuter[i] = (a == 1.0f)   ? 0.0
                       : (a == 0.0f) ? INF
                                     : std::pow(std::max(0.0f, 0.5f - a), 2);
        gridInner[i] = (a == 1.0f)   ? INF
                       : (a == 0.0f) ? 0.0
                                     : std::pow(std::max(0.0f, a - 0.5f), 2);
    }

    // Apply Euclidean Distance Transform
    edt(gridOuter, width, height, f, v, z);
    edt(gridInner, width, height, f, v, z);

    // Get distance property from RenderCommand props
    float radius = 50.0f;
    for (const auto& prop : command.props) {
        if (prop.propName == "distance" && prop.propType == PropType::Float) {
            radius = prop.value.toFloat();
            break;
        }
    }
    float offset = 0.25f;

    // Calculate bevel
    float minVal = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < gridSize; i++) {
        double d = std::sqrt(gridOuter[i]) - std::sqrt(gridInner[i]);
        float col = VALUE_MAX - VALUE_MAX * (d / radius + offset);
        col = std::max(0.0f, std::min(VALUE_MAX, col));

        minVal = std::min(minVal, col);
        maxVal = std::max(maxVal, col);
        grid[i] = col;
    }

    // Normalize and invert
    float range = maxVal - minVal;
    float scale = (range > 0.0f) ? (1.0f / range) : 1.0f;

    for (int i = 0; i < gridSize; i++) {
        float col = 1.0f - (grid[i] - minVal) * scale; // de-invert

        resultPixels[i * 4 + 0] = col;
        resultPixels[i * 4 + 1] = col;
        resultPixels[i * 4 + 2] = col;
        resultPixels[i * 4 + 3] = 1.0f;
    }

    // Upload result to texture
    gl->glBindTexture(GL_TEXTURE_2D, command.textureId);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, resultPixels.data());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
}

// 2D Euclidean squared distance transform by Felzenszwalb & Huttenlocher
// https://cs.brown.edu/~pff/papers/dt-final.pdf
static void edt(std::vector<double>& data, int width, int height,
                std::vector<double>& f, std::vector<uint16_t>& v,
                std::vector<double>& z)
{
    for (int x = 0; x < width; x++)
        edt1d(data, x, width, height, f, v, z);
    for (int y = 0; y < height; y++)
        edt1d(data, y * width, 1, width, f, v, z);
}

// 1D squared distance transform
static void edt1d(std::vector<double>& grid, int offset, int stride, int length,
                  std::vector<double>& f, std::vector<uint16_t>& v,
                  std::vector<double>& z)
{
    v[0] = 0;
    z[0] = -INF;
    z[1] = INF;

    // Load line in array three times for wrapping
    for (int q = 0; q < length; q++)
        f[q] = grid[offset + q * stride];
    for (int q = 0; q < length; q++)
        f[q + length] = grid[offset + q * stride];
    for (int q = 0; q < length; q++)
        f[q + length + length] = grid[offset + q * stride];

    int k = 0;
    for (int q = 1; q < length * 3; q++) {
        double s;
        do {
            int r = v[k];
            s = (f[q] - f[r] + q * q - r * r) / (q - r) / 2.0;
        } while (s <= z[k] && --k > -1);

        k++;
        v[k] = q;
        z[k] = s;
        z[k + 1] = INF;
    }

    // Copy over middle section
    for (int q = length, k = 0; q < length + length; q++) {
        while (z[k + 1] < q)
            k++;
        int r = v[k];
        grid[offset + (q - length) * stride] = f[r] + (q - r) * (q - r);
    }
}