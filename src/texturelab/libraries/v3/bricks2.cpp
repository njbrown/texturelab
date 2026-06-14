#include "../../models.h"
#include "../../props.h"
#include "../libv3.h"

void Bricks2Node::init()
{
    this->title = "Bricks 2";

    this->addInput("mortarMap");
    this->addInput("bevelMap");

    this->addEnumProp("pattern", "Pattern",
                      {"Running Bond", "Stack Bond", "Herringbone",
                       "Basket Weave"});

    this->addIntProp("rows", "Rows", 6, 1, 20, 1);
    this->addIntProp("columns", "Columns", 6, 1, 20, 1);
    this->addFloatProp("brickAspect", "Brick Aspect Ratio", 2.0, 0.5, 4.0, 0.1);
    this->addFloatProp("offset", "Offset", 0.5, 0, 1, 0.1);

    // mortar
    auto mortarProps = this->createGroup("Mortar");
    mortarProps->collapsed = false;
    mortarProps->add(
        this->addFloatProp("mortarWidth", "Mortar Width", 0.08, 0, 0.5, 0.01));
    mortarProps->add(this->addFloatProp("mortarMapStrength",
                                        "Mortar Map Strength", 0.5, 0, 1,
                                        0.01));

    // shape
    auto shapeProps = this->createGroup("Shape");
    shapeProps->collapsed = false;
    shapeProps->add(this->addFloatProp("shapeVariance", "Shape Variance", 0.0,
                                       0, 1, 0.01));
    shapeProps->add(
        this->addFloatProp("roundness", "Roundness", 0.0, 0, 1, 0.01));

    // bevel
    auto bevelProps = this->createGroup("Bevel");
    bevelProps->collapsed = false;
    bevelProps->add(
        this->addFloatProp("bevelAmount", "Bevel Amount", 0.0, 0, 1, 0.01));
    bevelProps->add(this->addFloatProp("bevelMapStrength",
                                       "Bevel Map Strength", 1.0, 0, 1, 0.01));

    // height
    auto heightProps = this->createGroup("Height");
    heightProps->collapsed = false;
    heightProps->add(
        this->addFloatProp("heightMin", "Height Min", 0.0, 0, 1, 0.05));
    heightProps->add(
        this->addFloatProp("heightMax", "Height Max", 1.0, 0, 1, 0.05));
    heightProps->add(
        this->addFloatProp("heightBalance", "Height Balance", 1.0, 0, 1, 0.05));
    heightProps->add(
        this->addFloatProp("heightVariance", "Height Variance", 0, 0, 1, 0.05));

    auto source = R""""(
        // ===================== height variation =====================
        float calculateHeight(vec2 brickId)
        {
            float heightMin = prop_heightMin;
            float heightMax = prop_heightMax;
            float heightBalance = prop_heightBalance;
            float heightVariance = prop_heightVariance;

            float balRand = _rand(vec2(_seed) + brickId * vec2(0.01));
            if (balRand > heightBalance) {
                return 1.0;
            }

            float randVariance =
                _rand(vec2(_seed) + (brickId + vec2(1)) * vec2(0.01));
            randVariance *= heightVariance;

            float range = (heightMax - heightMin);
            float height = heightMax - range * randVariance;

            return height;
        }

        // ===================== brick cell solver =====================
        // localUV: position within the brick's bounding cell (0..1)
        // brickId: stable per-brick id used for hashing
        // brickDim: relative (width,height) proportions of the brick,
        //           used to give roundness/bevel the correct aspect ratio
        struct CellInfo {
            vec2 localUV;
            vec2 brickId;
            vec2 brickDim;
        };

        // Running Bond (staggered rows) and Stack Bond (no stagger)
        CellInfo solveOrthoBond(vec2 uv, bool stagger)
        {
            vec2 tileSize = vec2(prop_columns, prop_rows);
            vec2 pos = uv * tileSize;

            if (stagger) {
                float xOffset = 0.0;
                if (fract(pos.y * 0.5) > 0.5) {
                    xOffset = prop_offset;
                }
                pos.x += xOffset;
            }

            vec2 brickId = floor(pos);

            // wrap around x so the hash matches the brick this one
            // continues into on the opposite edge
            if (brickId.x > tileSize.x - 1.0)
                brickId.x = 0.0;

            CellInfo c;
            c.localUV = fract(pos);
            c.brickId = brickId;
            c.brickDim = vec2(prop_brickAspect, 1.0);
            return c;
        }

        // Herringbone and Basket Weave share an LxL "weave" grid where L is
        // the (rounded) brick aspect ratio. Each weave cell is either filled
        // with L stacked horizontal bricks or L side-by-side vertical
        // bricks, alternating in a checkerboard. Herringbone additionally
        // staggers each row/column by its own index, producing the
        // characteristic zig-zag.
        CellInfo solveWeave(vec2 uv, bool herringbone)
        {
            vec2 tileSize = vec2(prop_columns, prop_rows);
            vec2 pos = uv * tileSize;

            float L = max(1.0, floor(prop_brickAspect + 0.5));

            vec2 weaveId = floor(pos / L);
            vec2 q = pos - weaveId * L;

            bool vertical = mod(weaveId.x + weaveId.y, 2.0) > 0.5;
            float stagger = herringbone ? 1.0 : 0.0;

            CellInfo c;

            if (!vertical) {
                // L horizontal (L x 1) bricks stacked vertically
                float row = floor(q.y);
                float shiftedX = mod(pos.x + row * stagger, L);

                c.localUV = vec2(shiftedX / L, fract(pos.y));
                c.brickId =
                    vec2(floor((pos.x + row * stagger) / L), floor(pos.y));
                c.brickDim = vec2(L, 1.0);
            }
            else {
                // L vertical (1 x L) bricks side by side
                float col = floor(q.x);
                float shiftedY = mod(pos.y + col * stagger, L);

                c.localUV = vec2(fract(pos.x), shiftedY / L);
                c.brickId =
                    vec2(floor(pos.x), floor((pos.y + col * stagger) / L));
                c.brickDim = vec2(1.0, L);
            }

            return c;
        }

        // ===================== shape =====================
        // per-edge jitter amounts in [-1,1]: x=left, y=right, z=bottom, w=top
        vec4 brickEdgeJitter(vec2 brickId)
        {
            vec4 j;
            j.x = _rand(vec2(_seed) + brickId * vec2(0.0123) + vec2(11.0, 3.0));
            j.y = _rand(vec2(_seed) + brickId * vec2(0.0123) + vec2(23.0, 7.0));
            j.z =
                _rand(vec2(_seed) + brickId * vec2(0.0123) + vec2(37.0, 17.0));
            j.w =
                _rand(vec2(_seed) + brickId * vec2(0.0123) + vec2(51.0, 29.0));
            return j * 2.0 - 1.0;
        }

        // rounded box SDF (centered at origin, half-extents b, corner radius r)
        float sdRoundBox(vec2 p, vec2 b, float r)
        {
            vec2 q = abs(p) - b + r;
            return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - r;
        }

        vec4 process(vec2 uv)
        {
            CellInfo c;
            if (prop_pattern == 0)
                c = solveOrthoBond(uv, true); // Running Bond
            else if (prop_pattern == 1)
                c = solveOrthoBond(uv, false); // Stack Bond
            else if (prop_pattern == 2)
                c = solveWeave(uv, true); // Herringbone
            else
                c = solveWeave(uv, false); // Basket Weave

            // mortar width: uniform + optional per-pixel map
            float mortarW = prop_mortarWidth;
            if (mortarMap_connected) {
                float m = texture(mortarMap, uv).r;
                mortarW += (m - 0.5) * prop_mortarMapStrength;
            }
            mortarW = clamp(mortarW, 0.0, 0.45);

            // non-uniform brick shapes: jitter each edge independently,
            // capped so edges never cross into the mortar of the
            // neighboring brick
            vec4 jitter = brickEdgeJitter(c.brickId);
            float jitterAmount = prop_shapeVariance * mortarW * 0.9;

            float left = mortarW + jitter.x * jitterAmount;
            float right = mortarW + jitter.y * jitterAmount;
            float bottom = mortarW + jitter.z * jitterAmount;
            float top = mortarW + jitter.w * jitterAmount;

            vec2 boxMin = vec2(left, bottom);
            vec2 boxMax = vec2(1.0 - right, 1.0 - top);
            vec2 center = (boxMin + boxMax) * 0.5;
            vec2 halfExtent = (boxMax - boxMin) * 0.5;

            // scale into the brick's own proportions so roundness/bevel
            // are relative to the brick shape, not the grid cell
            vec2 p = (c.localUV - center) * c.brickDim;
            vec2 b = halfExtent * c.brickDim;

            float shortSide = 2.0 * min(b.x, b.y);

            float radius = clamp(prop_roundness, 0.0, 1.0) * 0.5 * shortSide;
            radius = min(radius, min(b.x, b.y));

            float sdf = sdRoundBox(p, b, radius);

            float mask = sdf <= 0.0 ? 1.0 : 0.0;

            // bevel: darken a band along the inside of each brick edge
            float bevelAmt = prop_bevelAmount;
            if (bevelMap_connected) {
                float bm = texture(bevelMap, uv).r;
                bevelAmt *=
                    mix(1.0, bm, clamp(prop_bevelMapStrength, 0.0, 1.0));
            }
            bevelAmt = clamp(bevelAmt, 0.0, 1.0);

            float bevelWidth = bevelAmt * 0.5 * shortSide;
            float bevelMix = 1.0;
            if (bevelWidth > 0.0001) {
                bevelMix = clamp(-sdf / bevelWidth, 0.0, 1.0);
            }

            const float bevelFloor = 0.5;
            float bevelMultiplier = mix(bevelFloor, 1.0, bevelMix);

            float height = calculateHeight(c.brickId);

            float finalHeight = mask * height * bevelMultiplier;

            return vec4(vec3(finalHeight), 1.0);
        }
        )"""";

    this->setShaderSource(source);
}
