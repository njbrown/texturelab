#include "../../graphics/noderenderer.h"
#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

#include <QOpenGLFunctions_3_2_Core>
#include <algorithm>
#include <climits>
#include <cmath>
#include <queue>
#include <vector>

// ============================================================================
// FloodFillV2RenderData
// ============================================================================

struct FloodFillV2RenderData : public NodeRenderData {
    float threshold = 0.1f;
};

// ============================================================================
// FloodFillV2Renderer — CPU BFS flood fill via NodeTextureRenderer
//
// Output encoding (RGBA32F per pixel):
//   R = island origin x (UV, wrapped to [0,1], quantized to texel center)
//   G = island origin y (same)
//   B = bbox width / texture width
//   A = bbox height / texture height
//   Background = (0, 0, 0, 0)
//
// The origin is stored directly — downstream nodes use it as a per-island
// identity without reconstruction arithmetic, eliminating precision issues.
// UV-within-bbox is derived by downstream nodes as (uv - origin) / bbox_size.
// ============================================================================

class FloodFillV2Renderer : public NodeTextureRenderer {
public:
    void render(NodeRenderContext& ctx,
                const NodeRenderData& baseData) override
    {
        auto& data = static_cast<const FloodFillV2RenderData&>(baseData);
        auto gl = ctx.gl;
        auto cache = ctx.cache;

        int w = ctx.textureWidth;
        int h = ctx.textureHeight;

        // No input — output black
        if (ctx.inputs.isEmpty() || ctx.inputs[0].textureId == 0) {
            cache->bindFboToTexture(ctx.outputTextureId);
            gl->glViewport(0, 0, w, h);
            gl->glClearColor(0, 0, 0, 1);
            gl->glClear(GL_COLOR_BUFFER_BIT);
            return;
        }

        int gridSize = w * h;

        // Read input pixels via FBO
        std::vector<float> readPixels(gridSize * 4);
        cache->bindFboToTexture(ctx.inputs[0].textureId);
        gl->glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, readPixels.data());

        // Helper: wrap pixel coordinate into [0, bound)
        auto wrapAround = [](int value, int bound) -> int {
            return ((value % bound) + bound) % bound;
        };

        // Helper: get pixel intensity (average of RGB)
        auto getIntensity = [&](int x, int y) -> float {
            int idx = 4 * (w * y + x);
            return (readPixels[idx] + readPixels[idx + 1] + readPixels[idx + 2])
                   / 3.0f;
        };

        // BFS flood fill with wrap-around
        struct Island {
            int left = INT_MAX, top = INT_MAX;
            int right = INT_MIN, bottom = INT_MIN;
            struct Pixel {
                int localX, localY, globalX, globalY;
            };
            std::vector<Pixel> pixels;

            void expand(int x, int y)
            {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x);
                bottom = std::max(bottom, y);
            }
            int width() const { return right - left; }
            int height() const { return bottom - top; }
        };

        std::vector<bool> visited(gridSize, false);
        std::vector<Island> islands;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (visited[y * w + x])
                    continue;

                Island island;
                std::queue<std::pair<int, int>> queue;
                queue.push({x, y});

                while (!queue.empty()) {
                    auto [gx, gy] = queue.front();
                    queue.pop();

                    int lx = wrapAround(gx, w);
                    int ly = wrapAround(gy, h);

                    if (visited[ly * w + lx])
                        continue;
                    visited[ly * w + lx] = true;

                    if (getIntensity(lx, ly) < data.threshold)
                        continue;

                    island.expand(gx, gy);
                    island.pixels.push_back({lx, ly, gx, gy});

                    queue.push({gx + 1, gy});
                    queue.push({gx - 1, gy});
                    queue.push({gx, gy + 1});
                    queue.push({gx, gy - 1});
                }

                if (island.width() > 0 && island.height() > 0)
                    islands.push_back(std::move(island));
            }
        }

        // Build output texture
        // Encoding: (origin_x, origin_y, bbox_w, bbox_h)
        // origin is the top-left of the bbox, wrapped to [0,1] UV space,
        // quantized to texel centers for consistency across all pixels
        std::vector<float> results(gridSize * 4, 0.0f);

        float invW = 1.0f / w;
        float invH = 1.0f / h;

        for (const auto& island : islands) {
            float bboxW = island.width() * invW;
            float bboxH = island.height() * invH;

            // Origin in UV space, wrapped to [0,1] and quantized to texel center
            int originLocalX = wrapAround(island.left, w);
            int originLocalY = wrapAround(island.top, h);
            float originU = (originLocalX + 0.5f) * invW;
            float originV = (originLocalY + 0.5f) * invH;

            for (const auto& px : island.pixels) {
                int idx = 4 * (w * px.localY + px.localX);
                results[idx + 0] = originU;
                results[idx + 1] = originV;
                results[idx + 2] = bboxW;
                results[idx + 3] = bboxH;
            }
        }

        // Upload result to output texture
        gl->glBindTexture(GL_TEXTURE_2D, ctx.outputTextureId);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA,
                         GL_FLOAT, results.data());
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl->glBindTexture(GL_TEXTURE_2D, 0);
    }
};

// ============================================================================
// FloodFillV2Node
// ============================================================================

void FloodFillV2Node::init()
{
    this->title = "Flood Fill V2";
    this->addInput("image");
    this->addFloatProp("threshold", "Threshold", 0.1, 0.0, 1.0, 0.01);

    auto source = R""""(
        vec4 process(vec2 uv)
        {
            return texture(image, uv);
        }
    )"""";
    this->setShaderSource(source);
}

std::shared_ptr<NodeTextureRenderer> FloodFillV2Node::createRenderer()
{
    return std::make_shared<FloodFillV2Renderer>();
}

std::shared_ptr<NodeRenderData> FloodFillV2Node::createRenderData()
{
    auto data = std::make_shared<FloodFillV2RenderData>();

    auto threshProp = static_cast<FloatProp*>(this->getProp("threshold"));
    if (threshProp)
        data->threshold = threshProp->value;

    return data;
}
