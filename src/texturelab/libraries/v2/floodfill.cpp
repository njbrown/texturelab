#include "../../graphics/renderworker.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv2.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

// Use normalized float range [0.0, 1.0] for GL_RGBA32F textures
static const float VALUE_MAX = 1.0f;
static const float ONE_OVER_VALUE_MAX = 1.0f / VALUE_MAX;

struct Vector2i {
    int x, y;
    Vector2i(int x, int y) : x(x), y(y) {}
};

struct FloodFillPixel {
    int localX, localY;
    int globalX, globalY;
    FloodFillPixel(int lx, int ly, int gx, int gy)
        : localX(lx), localY(ly), globalX(gx), globalY(gy)
    {
    }
};

struct Box {
    int left = 0, top = 0, right = 0, bottom = 0;
    std::vector<FloodFillPixel> pixels;

    void expand(int x, int y)
    {
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x);
        bottom = std::max(bottom, y);
    }

    void negativeInfinity()
    {
        left = INT_MAX;
        top = INT_MAX;
        right = INT_MIN;
        bottom = INT_MIN;
    }

    int width() const { return right - left; }
    int height() const { return bottom - top; }
};

static int wrapAround(int value, int upperBound)
{
    return ((value % upperBound) + upperBound) % upperBound;
}

static Vector2i mapPixelToLocal(const Vector2i& globalPixel, int width,
                                int height)
{
    return Vector2i(wrapAround(globalPixel.x, width),
                    wrapAround(globalPixel.y, height));
}

static float getIntensity(int x, int y, const std::vector<float>& pixels,
                          int width, int height)
{
    int idx = 4 * (width * y + x);
    float col = pixels[idx + 0] + pixels[idx + 1] + pixels[idx + 2];
    return col * (1.0f / 3.0f) * ONE_OVER_VALUE_MAX;
}

static void setColorAtPixel(std::vector<float>& data, int width, int x, int y,
                            float r, float g, float b, float a)
{
    int idx = 4 * (width * y + x);
    data[idx + 0] = r;
    data[idx + 1] = g;
    data[idx + 2] = b;
    data[idx + 3] = a;
}

void FloodFillNode::init()
{
    this->title = "Flood Fill";
    this->addInput("image");
    this->usesCpuProcessing = true;

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            vec4 col = texture(image, uv);
            return col;
        }
		)"""";
    this->setShaderSource(source);
}

void FloodFillNode::cpuProcess(void* glPtr, const RenderCommand& command)
{
    auto gl = static_cast<QOpenGLFunctions_3_2_Core*>(glPtr);

    GLuint inputTextureId = 0;
    if (!command.inputs.isEmpty()) {
        inputTextureId = command.inputs[0].textureId;
    }

    if (inputTextureId == 0)
        return;

    int width = command.textureWidth;
    int height = command.textureHeight;
    // Guard against zero/negative dimensions: wrapAround() below does value %
    // width/height (division by zero), and a negative product would wrap to a
    // huge size_t at allocation.
    if (width <= 0 || height <= 0)
        return;
    // size_t so width*height*4 can't overflow int for large textures.
    size_t gridSize = (size_t)width * height;

    // Read pixels from input texture
    std::vector<float> readPixels(gridSize * 4);
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

    // Process flood fill
    std::vector<bool> visited(gridSize, false);
    std::vector<Box> rects;
    float threshold = 0.1f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (visited[y * width + x])
                continue;

            // Capture island
            Box rect;
            rect.negativeInfinity();

            std::queue<Vector2i> queue;
            queue.push(Vector2i(x, y));

            while (!queue.empty()) {
                Vector2i globalPixel = queue.front();
                queue.pop();

                Vector2i pixel = mapPixelToLocal(globalPixel, width, height);

                if (visited[pixel.y * width + pixel.x])
                    continue;
                visited[pixel.y * width + pixel.x] = true;

                float intensity =
                    getIntensity(pixel.x, pixel.y, readPixels, width, height);
                if (intensity < threshold)
                    continue;

                rect.expand(globalPixel.x, globalPixel.y);
                rect.pixels.emplace_back(pixel.x, pixel.y, globalPixel.x,
                                         globalPixel.y);

                queue.push(Vector2i(globalPixel.x + 1, globalPixel.y));
                queue.push(Vector2i(globalPixel.x - 1, globalPixel.y));
                queue.push(Vector2i(globalPixel.x, globalPixel.y + 1));
                queue.push(Vector2i(globalPixel.x, globalPixel.y - 1));
            }

            if (rect.width() > 0 && rect.height() > 0) {
                rects.push_back(rect);
            }
        }
    }

    // Render output
    std::vector<float> results(gridSize * 4, 0.0f);

    for (const auto& rect : rects) {
        float sx = rect.width() * (1.0f / width);
        float sy = rect.height() * (1.0f / height);

        for (const auto& pixel : rect.pixels) {
            float u = (pixel.globalX - rect.left) / (float)rect.width();
            float v = (pixel.globalY - rect.top) / (float)rect.height();

            setColorAtPixel(results, width, pixel.localX, pixel.localY, u, v,
                            sx, sy);
        }
    }

    // Upload result to texture
    gl->glBindTexture(GL_TEXTURE_2D, command.textureId);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, results.data());
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gl->glBindTexture(GL_TEXTURE_2D, 0);
}