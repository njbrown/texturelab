#include "library.h"
#include "../models.h"
#include "libv1.h"
#include "libv2.h"
#include "libv3.h"

#include <QMap>

TextureNodePtr Library::createNode(QString name)
{
    if (this->items.contains(name)) {
        auto& item = items[name];
        if (item.name == name) {
            auto node = item.factoryFunction();
            node->typeName = name;

            // todo: put this in the appropriate place
            node->init();

            return node;
        }
    }

    return TextureNodePtr(nullptr);
}

void Library::addNode(QString name, QString displayName, QString iconPath,
                      std::function<TextureNodePtr()> factoryFunction)
{

    LibraryEntry entry;
    entry.name = name;
    entry.displayName = displayName;
    entry.icon = QIcon(iconPath);
    entry.factoryFunction = factoryFunction;

    items[name] = entry;
}

bool Library::hasNode(QString name) { return items.contains(name); }

LibraryV1::LibraryV1() : Library()
{
    // add items
}

Library* createLibraryV2()
{
    auto lib = new Library();
    lib->addNode<PolygonNode>("polygon", "Polygon", ":nodes/bevel.png");
    lib->addNode<CircleNode>("circle", "Circle", ":nodes/circle.png");
    lib->addNode<ColorNode>("color", "Color", ":nodes/color.png");
    lib->addNode<ColorizeNode>("colorize", "Colorize", ":nodes/colorize.png");
    lib->addNode<BlendNode>("blend", "Blend", ":nodes/blend.png");
    lib->addNode<OutputNode>("output", "Output", ":nodes/output.png");
    lib->addNode<NormalMapNode>("normalmap", "Normal Map",
                                ":nodes/normalmap.png");
    lib->addNode<BrickGeneratorNode>("brickgenerator", "Brick Generator",
                                     ":nodes/brickgenerator.png");

    lib->addNode<BrightnessContrastNode>("brightnesscontrast",
                                         "Brick Generator",
                                         ":nodes/brightnesscontrast.png");
    lib->addNode<CellNode>("cell", "Cell", ":nodes/cell.png");
    lib->addNode<CheckerboardNode>("checkerboard", "Checkerboard",
                                   ":nodes/checkerboard.png");
    lib->addNode<CopyNode>("copy", "Copy", ":nodes/copy.png");
    lib->addNode<DirectionalWarpNode>("directionalwarp", "Directional Warp",
                                      ":nodes/directionalwarp.png");
    lib->addNode<FractalNoiseNode>("fractalnoise", "Fractal Noise",
                                   ":nodes/fractalnoise.png");

    lib->addNode<GradientNode>("gradient", "Gradient", ":nodes/gradient.png");
    lib->addNode<GradientMapNode>("gradientmap", "Gradient Map",
                                  ":nodes/gradientmap.png");
    lib->addNode<HeightShiftNode>("heightshift", "Height Shift",
                                  ":nodes/heightshift.png");
    lib->addNode<HexagonNode>("hexagon", "Hexagon", ":nodes/hexagon.png");
    lib->addNode<InvertNode>("invert", "Invert", ":nodes/invert.png");
    lib->addNode<LineCellNode>("linecell", "Line Cell", ":nodes/linecell.png");
    lib->addNode<MapRangeNode>("maprange", "Map Range", ":nodes/maprange.png");
    lib->addNode<MaskNode>("mask", "Mask", ":nodes/mask.png");
    lib->addNode<MirrorNode>("mirror", "Mirror", ":nodes/mirror.png");
    lib->addNode<Perlin3DNode>("perlin3d", "Perlin 3D", ":nodes/perlin3d.png");
    lib->addNode<SolidCellNode>("solidcell", "Solid Cell",
                                ":nodes/solidcell.png");
    lib->addNode<SplatNode>("splat", "Splat", ":nodes/splat.png");
    lib->addNode<ThresholdNode>("threshold", "Threshold",
                                ":nodes/threshold.png");
    lib->addNode<TileNode>("tile", "Tile", ":nodes/tile.png");
    lib->addNode<Transform2DNode>("transform2d", "Transform2D",
                                  ":nodes/transform2d.png");
    lib->addNode<WarpNode>("warp", "Warp", ":nodes/warp.png");
    lib->addNode<WaveNode>("wave", "Wave", ":nodes/wave.png");

    // V2 NODES START HERE
    lib->addNode<AdvanceSplatterNode>("advancesplatter", "AdvanceSplatter",
                                      ":nodes/advancesplatter.png");
    lib->addNode<AnisotropicBlurNode>("anisotropicblur", "Anisotropic Blur",
                                      ":nodes/anisotropicblur.png");
    lib->addNode<BevelNode>("bevel", "Bevel", ":nodes/bevel.png");
    lib->addNode<BlurNodeV2>("blurv2", "Blur", ":nodes/blurv2.png");
    lib->addNode<CapsuleNode>("capsule", "Capsule", ":nodes/capsule.png");
    lib->addNode<CartesianToPolarNode>("cartesiantopolar", "Cartesian To Polar",
                                       ":nodes/cartesiantopolar.png");
    lib->addNode<CircularSplatterNode>("circularsplatter", "Circular Splatter",
                                       ":nodes/circularsplatter.png");
    lib->addNode<ClampNode>("clamp", "Clamp", ":nodes/clamp.png");
    lib->addNode<CombineNormalsNode>("combinenormals", "Combine Normals",
                                     ":nodes/combinenormals.png");
    lib->addNode<DirectionalBlurNode>("directionalblur", "Directional Blur",
                                      ":nodes/directionalblur.png");
    lib->addNode<DirectionalWarpV2Node>("directionalwarp", "Directional Warp",
                                        ":nodes/directionalwarp.png");
    lib->addNode<ExtractChannelNode>("extractchannel", "Extract Channel",
                                     ":nodes/extractchannel.png");
    lib->addNode<FloodFillNode>("floodfill", "Flood Fill",
                                ":nodes/floodfill.png");
    lib->addNode<FloodFillSamplerNode>("floodfillsampler", "FloodFill Sampler",
                                       ":nodes/floodfillsampler.png");
    lib->addNode<FloodFillToBBoxNode>("floodfilltobbox", "FloodFill to BBox",
                                      ":nodes/floodfilltobbox.png");
    lib->addNode<FloodFillToColorNode>("floodfilltocolor", "FloodFill to Color",
                                       ":nodes/floodfilltocolor.png");
    lib->addNode<FloodFillToGradientNode>("floodfilltogradient",
                                          "FloodFill to Gradient",
                                          ":nodes/floodfilltogradient.png");
    lib->addNode<FloodFillToRandomColorNode>(
        "floodfilltorandomcolor", "FloodFill to Random Color",
        ":nodes/floodfilltorandomcolor.png");
    lib->addNode<FloodFillToRandomIntensityNode>(
        "floodfilltorandomintensity", "FloodFill to Random Intensity",
        ":nodes/floodfilltorandomintensity.png");
    lib->addNode<GradientDynamicNode>("gradientdynamic", "Gradient Dynamic",
                                      ":nodes/gradientdynamic.png");
    lib->addNode<GradientNoiseNode>("gradientnoise", "Gradient Noise",
                                    ":nodes/gradientnoise.png");
    lib->addNode<GradientNoiseFractalSumNode>(
        "gradientnoisefractalsum", "Gradient Noise Fractal Sum",
        ":nodes/gradientnoisefractalsum.png");
    lib->addNode<GrayscaleNode>("grayscale", "Grayscale",
                                ":nodes/grayscale.png");
    lib->addNode<HistogramScanNode>("histogramscan", "Histogram Scan",
                                    ":nodes/histogramscan.png");
    lib->addNode<HistogramSelectNode>("histogramselect", "Histogram Select",
                                      ":nodes/histogramselect.png");
    lib->addNode<HistogramShiftNode>("histogramshift", "Histogram Shift",
                                     ":nodes/histogramshift.png");
    lib->addNode<HslNode>("hsl", "HSL", ":nodes/hsl.png");
    lib->addNode<HslExtractNode>("hslextract", "HSL Extract",
                                 ":nodes/hslextract.png");
    lib->addNode<ImageNode>("image", "Image", ":nodes/image.png");
    lib->addNode<InvertNormalNode>("invertnormal", "Node",
                                   ":nodes/invertnormal.png");
    lib->addNode<NormalMapV2Node>("normalmap", "Normal Map",
                                  ":nodes/normalmap.png");
    lib->addNode<PolarToCartesianNode>("polartocartesian", "Polar to Cartesian",
                                       ":nodes/polartocartesian.png");
    lib->addNode<PowNode>("pow", "Pow", ":nodes/pow.png");
    lib->addNode<QuantizeNode>("quantize", "Quantize", ":nodes/quantize.png");
    lib->addNode<RgbaMergeNode>("rgbamerge", "RGA Merge",
                                ":nodes/rgbamerge.png");
    lib->addNode<RgbaShuffleNode>("rgbashuffle", "RGBA Shuffle",
                                  ":nodes/rgbashuffle.png");
    lib->addNode<SimplexNoiseV2Node>("simplexnoise", "Simplex Noise",
                                     ":nodes/simplexnoise.png");
    lib->addNode<SkewNode>("skew", "Skew", ":nodes/skew.png");
    lib->addNode<SlopeBlurNode>("slopeblur", "Slope Blur",
                                ":nodes/slopeblur.png");
    lib->addNode<SoftFlowerNode>("softflower", "Soft Flower",
                                 ":nodes/softflower.png");
    lib->addNode<SplatV2Node>("splat", "Splat", ":nodes/splat.png");
    lib->addNode<StarNode>("star", "Star", ":nodes/star.png");
    lib->addNode<StripesNode>("stripes", "Stripes", ":nodes/stripes.png");
    lib->addNode<TileSamplerNode>("tilesampler", "Tile Sampler",
                                  ":nodes/tilesampler.png");
    lib->addNode<Transform2DV2Node>("transform2d", "Transform2D",
                                    ":nodes/transform2d.png");
    lib->addNode<ValueNoiseNode>("valuenoise", "Value Noise",
                                 ":nodes/valuenoise.png");
    lib->addNode<ValueNoiseFractalSumNode>("valuenoisefractalsum",
                                           "Value Noise Fractal Sum",
                                           ":nodes/valuenoisefractalsum.png");
    lib->addNode<WarpNodeV2>("warp", "Warp", ":nodes/warp.png");

    return lib;
}

Library* createLibraryV3()
{
    auto lib = createLibraryV2();

    // Remove V1/V2 nodes that are superseded by V3 equivalents
    lib->items.remove("floodfill");
    lib->items.remove("floodfillsampler");
    lib->items.remove("floodfilltobbox");
    lib->items.remove("floodfilltocolor");
    lib->items.remove("floodfilltogradient");
    lib->items.remove("floodfilltorandomcolor");
    lib->items.remove("floodfilltorandomintensity");
    lib->items.remove("bevel");
    lib->items.remove("perlin3d");
    lib->items.remove("blend");
    lib->items.remove("cell");
    lib->items.remove("linecell");
    lib->items.remove("solidcell");

    // V3 NODES
    lib->addNode<BevelV2Node>("bevelv2", "Bevel V2", ":nodes/bevel.png");
    lib->addNode<AmbientOcclusionNode>("ambientocclusion", "Ambient Occlusion",
                                       ":nodes/bevel.png");
    lib->addNode<CurvatureNode>("curvature", "Curvature", ":nodes/bevel.png");
    lib->addNode<MaskedBlurNode>("maskedblur", "Masked Blur",
                                 ":nodes/blurv2.png");
    lib->addNode<RaysNode>("rays", "Rays", ":nodes/bevel.png");
    lib->addNode<SwirlNode>("swirl", "Swirl", ":nodes/bevel.png");
    lib->addNode<FloodFillV2Node>("floodfillv2", "Flood Fill V2",
                                  ":nodes/floodfill.png");
    lib->addNode<FloodFillV2ToColorNode>("floodfillv2tocolor", "FF To Color V2",
                                         ":nodes/floodfilltocolor.png");
    lib->addNode<FloodFillV2ToRandomColorNode>(
        "floodfillv2torandomcolor", "FF To Random Color V2",
        ":nodes/floodfilltorandomcolor.png");
    lib->addNode<FloodFillV2ToRandomIntensityNode>(
        "floodfillv2torandomintensity", "FF To Random Intensity V2",
        ":nodes/floodfilltorandomintensity.png");
    lib->addNode<FloodFillV2ToBBoxNode>("floodfillv2tobbox", "FF To BBox V2",
                                        ":nodes/floodfilltobbox.png");
    lib->addNode<FloodFillV2ToGradientNode>("floodfillv2togradient",
                                            "FF To Gradient V2",
                                            ":nodes/floodfilltogradient.png");
    lib->addNode<FloodFillV2SamplerNode>("floodfillv2sampler", "FF Sampler V2",
                                         ":nodes/floodfillsampler.png");

    lib->addNode<CurveNode>("curve", "Curve", ":nodes/curve.png");

    // -----------------------------------------------------------------------
    // Phase 1 — Filters / Color
    // -----------------------------------------------------------------------
    lib->addNode<BlendV3Node>("blend", "Blend", ":nodes/blend.png");
    lib->addNode<EdgeDetectNode>("edgedetect", "Edge Detect",
                                 ":nodes/bevel.png");
    lib->addNode<HighpassNode>("highpass", "Highpass", ":nodes/blurv2.png");
    lib->addNode<EmbossNode>("emboss", "Emboss", ":nodes/normalmap.png");
    lib->addNode<VibranceNode>("vibrance", "Vibrance", ":nodes/hsl.png");
    lib->addNode<ColorToMaskNode>("colortomask", "Color To Mask",
                                  ":nodes/extractchannel.png");
    lib->addNode<ToonGradientNode>("toongradient", "Toon Gradient",
                                   ":nodes/gradientmap.png");
    lib->addNode<AutoLevelsNode>("autolevels", "Auto Levels",
                                 ":nodes/histogramscan.png");

    // -----------------------------------------------------------------------
    // Phase 2 — Generators
    // -----------------------------------------------------------------------
    lib->addNode<Bricks2Node>("bricks2", "Bricks 2",
                              ":nodes/brickgenerator.png");
    lib->addNode<DirectionalScratchesNode>(
        "directionalscratches", "Directional Scratches", ":nodes/cell.png");
    lib->addNode<RoughGrainNode>("roughgrain", "Rough Grain",
                                 ":nodes/cell.png");
    lib->addNode<VoronoiFractalNode>("voronoifractal", "Voronoi Fractal",
                                     ":nodes/cell.png");
    lib->addNode<TruchetNode>("truchet", "Truchet", ":nodes/hexagon.png");
    lib->addNode<FBMDomainWarpNode>("fbmdomainwarp", "FBM Domain Warp",
                                    ":nodes/fractalnoise.png");
    lib->addNode<PerlinNoiseNode>("perlinnoise", "Perlin Noise",
                                  ":nodes/fractalnoise.png");
    lib->addNode<PerlinNoise3DNode>("perlinnoise3d", "Perlin Noise 3D",
                                    ":nodes/fractalnoise.png");
    lib->addNode<CellV3Node>("cell", "Cell V3", ":nodes/cell.png");
    lib->addNode<LineCellV3Node>("linecell", "Line Cell V3",
                                 ":nodes/linecell.png");
    lib->addNode<SolidCellV3Node>("solidcell", "Solid Cell V3",
                                  ":nodes/solidcell.png");

    // -----------------------------------------------------------------------
    // Phase 3 — Multi-pass
    // -----------------------------------------------------------------------
    lib->addNode<BlurHQNode>("blurhq", "Blur HQ", ":nodes/blurv2.png");
    lib->addNode<DistanceTransformNode>(
        "distancetransform", "Distance Transform", ":nodes/bevel.png");
    lib->addNode<ColorSpreadNode>("spread", "Spread", ":nodes/bevel.png");
    lib->addNode<HeightBlendNode>("heightblend", "Height Blend",
                                  ":nodes/blend.png");
    // lib->addNode<MakeItTileNode>("makeittile", "Make It Tile",
    //                              ":nodes/tile.png");
    // lib->addNode<NormalMapV3Node>("normalmapv3", "Normal Map V3",
    //                               ":nodes/normalmap.png");

    return lib;
}