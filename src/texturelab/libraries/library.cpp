#include "library.h"
#include "../models.h"
#include "libv1.h"
#include "libv2.h"

#include <QMap>

TextureNodePtr Library::createNode(QString name)
{
    if (this->items.contains(name)) {
        auto& item = items[name];
        if (item.name == name) {
            auto node = item.factoryFunction();

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
    // lib->addNode<PolygonNode>("polygon", "Polygon", ":nodes/bevel.png");
    lib->addNode<CircleNode>("circle", "Circle", ":nodes/circle.png");
    lib->addNode<ColorNode>("color", "Color", ":nodes/color.png");
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
    // lib->addNode<DirectionalWarpNode>("directionalwarp", "Directional Warp",
    //                                   ":nodes/directionalwarp.png");
    lib->addNode<FractalNoiseNode>("fractalnoise", "Fractal Noise",
                                   ":nodes/fractalnoise.png");

    lib->addNode<GradientNode>("gradient", "Gradient", ":nodes/gradient.png");
    // lib->addNode<GradientMapNode>("gradientmap", "Gradient Map",
    //                               ":nodes/gradientmap.png");
    lib->addNode<HeightShiftNode>("heightshift", "Height Shift",
                                  ":nodes/heightshift.png");
    lib->addNode<HexagonNode>("hexagon", "Hexagon", ":nodes/hexagon.png");
    lib->addNode<InvertNode>("invert", "Invert", ":nodes/invert.png");
    lib->addNode<LineCellNode>("linecell", "Line Cell", ":nodes/linecell.png");
    lib->addNode<MapRangeNode>("maprange", "Map Range", ":nodes/maprange.png");
    lib->addNode<MaskNode>("mask", "Mask", ":nodes/mask.png");
    lib->addNode<MirrorNode>("mirror", "Mirror", ":nodes/mirror.png");
    // lib->addNode<BlendNode>("normalmap", "Normal Map",
    // ":nodes/normalmap.png");
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
    lib->addNode<GradientNoiseNode>("gradientnode", "Gradient Noise",
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