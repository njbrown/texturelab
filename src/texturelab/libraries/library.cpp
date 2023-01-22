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
    lib->addNode<CircleNode>("circle", "Circle", ":nodes/bevel.png");
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
    lib->addNode<DirectionalWarpNode>("directionalwarp", "Directional Warp",
                                      ":nodes/directionalwarp.png");
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

    return lib;
}