// Shader prelude, ported from RenderResourceCache in
// src/texturelab/graphics/noderenderer.cpp. randomLib/gradientLib/curveLib are
// verbatim copies; keep them in sync with the C++. The vertex shader and
// preamble differ only in the GLSL ES 3.00 header (#version + precision).

const ES_HEADER = `#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;
`;

export const standardVertexSource =
    ES_HEADER +
    /* glsl */ `
in vec3 a_pos;
in vec2 a_texCoord;
out vec2 v_texCoord;
void main() {
    v_texCoord = a_texCoord;
    gl_Position = vec4(a_pos, 1);
}
`;

// fragmentPreamble() minus its `#version 150 core` line.
export const fragmentPreamble =
    ES_HEADER +
    /* glsl */ `
        in vec2 v_texCoord;

        #define GRADIENT_MAX_POINTS 32
        #define CURVE_MAX_POINTS 32

        vec4 process(vec2 uv);
        void initRandom();

        uniform vec2 _textureSize;

        out vec4 fragColor;

        void main() {
            initRandom();
            vec4 result = process(v_texCoord);
            fragColor = clamp(result, 0.0, 1.0);
        }
    `;

export const randomLib = /* glsl */ `
        uniform float _seed;
        vec2 _randomStart;

        #define RANDOM_ITERATIONS 1

        #define HASHSCALE1 443.8975
        #define HASHSCALE3 vec3(443.897, 441.423, 437.195)
        #define HASHSCALE4 vec4(443.897, 441.423, 437.195, 444.129)

        float hash12(vec2 p)
        {
            vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
            p3 += dot(p3, p3.yzx + 19.19);
            return fract((p3.x + p3.y) * p3.z);
        }

        vec2 hash22(vec2 p)
        {
            vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
            p3 += dot(p3, p3.yzx+19.19);
            return fract((p3.xx+p3.yz)*p3.zy);
        }

        float _rand(vec2 uv)
        {
            float a = 0.0;
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                vec2 pos = (uv * v);
                a += hash12(pos);
            }
            return a/float(RANDOM_ITERATIONS);
        }

        vec2 _rand2(vec2 uv)
        {
            vec2 a = vec2(0.0);
            for (int t = 0; t < RANDOM_ITERATIONS; t++)
            {
                float v = float(t+1)*.152;
                vec2 pos = (uv * v);
                a += hash22(pos);
            }
            return a/float(RANDOM_ITERATIONS);
        }

        float randomFloat(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomVec2(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index));
        }

        float randomFloat(int index, float start, float end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + r*(end-start);
        }

        int randomInt(int index, int start, int end)
        {
            float r = _rand(_randomStart + vec2(_seed) + vec2(index));
            return start + int(r*float(end-start));
        }

        bool randomBool(int index)
        {
            return _rand(_randomStart + vec2(_seed) + vec2(index)) > 0.5;
        }

        void initRandom()
        {
            _randomStart = v_texCoord;
        }
    `;

export const gradientLib = /* glsl */ `
        struct Gradient {
            vec3 colors[GRADIENT_MAX_POINTS];
            float positions[GRADIENT_MAX_POINTS];
            int numPoints;
        };

        vec3 sampleGradient(vec3 colors[GRADIENT_MAX_POINTS],
                            float positions[GRADIENT_MAX_POINTS],
                            int numPoints, float t)
        {
            if (numPoints == 0)
                return vec3(1,0,0);

            if (numPoints == 1)
                return colors[0];

            if (t <= positions[0])
                return colors[0];

            int last = numPoints - 1;
            if (t >= positions[last])
                return colors[last];

            for(int i = 0; i < numPoints-1; i++) {
                if (positions[i+1] > t) {
                    vec3 colorA = colors[i];
                    vec3 colorB = colors[i+1];

                    float t1 = positions[i];
                    float t2 = positions[i+1];

                    float lerpPos = (t - t1)/(t2 - t1);
                    return mix(colorA, colorB, lerpPos);
                }
            }

            return vec3(0,0,0);
        }

        vec3 sampleGradient(Gradient gradient, float t)
        {
            return sampleGradient(gradient.colors, gradient.positions,
                                  gradient.numPoints, t);
        }
    `;

export const curveLib = /* glsl */ `
        struct Curve {
            int  numPoints;
            vec2 anchors[CURVE_MAX_POINTS];
            vec2 handleR[CURVE_MAX_POINTS];
            vec2 handleL[CURVE_MAX_POINTS];
        };

        float _cubicBez(float p0, float p1, float p2, float p3, float t) {
            float mt = 1.0 - t;
            return mt*mt*mt*p0 + 3.0*mt*mt*t*p1 + 3.0*mt*t*t*p2 + t*t*t*p3;
        }

        float _cubicBezD(float p0, float p1, float p2, float p3, float t) {
            float mt = 1.0 - t;
            return 3.0 * (mt*mt*(p1-p0) + 2.0*mt*t*(p2-p1) + t*t*(p3-p2));
        }

        float evalCurve(Curve c, float x) {
            // Find segment: last anchor whose x <= input x
            int seg = c.numPoints - 2;
            for (int i = 0; i < CURVE_MAX_POINTS - 1; i++) {
                if (i >= c.numPoints - 1) break;
                if (x <= c.anchors[i + 1].x) { seg = i; break; }
            }

            vec2 P0 = c.anchors[seg];
            vec2 P1 = c.handleR[seg];
            vec2 P2 = c.handleL[seg + 1];
            vec2 P3 = c.anchors[seg + 1];

            // Newton-Raphson: solve Bx(t) = x (8 iterations, constant bound)
            float t = clamp((x - P0.x) / max(P3.x - P0.x, 1e-5), 0.0, 1.0);
            for (int i = 0; i < 8; i++) {
                float bx  = _cubicBez(P0.x, P1.x, P2.x, P3.x, t);
                float dbx = _cubicBezD(P0.x, P1.x, P2.x, P3.x, t);
                t -= (abs(dbx) > 1e-6) ? (bx - x) / dbx : 0.0;
                t  = clamp(t, 0.0, 1.0);
            }

            return _cubicBez(P0.y, P1.y, P2.y, P3.y, t);
        }
    `;
