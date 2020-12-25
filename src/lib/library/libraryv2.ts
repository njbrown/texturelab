import { BrickGeneratorNode } from "./v1/brickgenerator";
import { NormalMapV2 } from "./v2/normalmapv2";
import { CellNode } from "./v1/cellnode";
import { SolidCellNode } from "./v1/solidcell";
import { LineCellNode } from "./v1/linecell";
import { PolygonNode, CircleNode } from "./v1/shapes";
import { BlendNode } from "./v1/blend";
import { InvertNode } from "./v1/invert";
import { WarpNodeV2 } from "./v2/warpv2";
import { ColorNode, ColorizeNode } from "./v1/color";
import { SimplexNoiseNode } from "./v1/simplexnoise";
import { MaskNode } from "./v1/mask";
import { Transform2DNodeV2 } from "./v2/transform2dv2";
import { MapRangeNode } from "./v1/maprange";
import { Perlin3DNode } from "./v1/perlin3d";
import { HexagonNode } from "./v1/hexagon";
import { FractalNoiseNode } from "./v1/fractalnoise";
import { TileNode } from "./v1/tile";
import { ThresholdNode } from "./v1/threshold";
import { HeightShiftNode } from "./v1/heightshift";
import { CheckerBoardNode } from "./v1/checkerboard";
import { DirectionalWarpNode } from "./v1/directionalwarp";
import { MirrorNode } from "./v1/mirror";
import { BrightnessContrastNode } from "./v1/brightnesscontrast";
import { WaveNode } from "./v1/wave";
import { CopyNode } from "./v1/copy";
import { GradientNode, TriGradientNode } from "./v1/gradient";
import { DesignerLibrary } from "../designer/library";
import { OutputNode } from "./v1/output";
import { GradientMapNode } from "./v1/gradientmap";
import { SplatNodeV2 } from "./v2/splat";
import { BlurV2 } from "./v2/blur";
import { AdvanceSplatterV2 } from "./v2/advancesplatter";
import { SlopeBlur } from "./v2/slopeblur";
import { TileSampler } from "./v2/tilesampler";
import { GradientDynamic } from "./v2/gradientdynamic";
import { Clamp } from "./v2/clamp";
import { Pow } from "./v2/pow";
import { Quantize } from "./v2/quantize";
import { HistogramShift } from "./v2/histogramshift";
import { HistogramScan } from "./v2/histogramscan";
import { HistogramSelect } from "./v2/histogramselect";
import { Skew } from "./v2/skew";
import { Bevel } from "./v2/bevel";
import { FloodFill } from "./v2/floodfill";
import { FloodFillToColor } from "./v2/floodfilltocolor";
import { FloodFillToBBox } from "./v2/floodfilltobbox";
import { FloodFillToRandomColor } from "./v2/floodfilltorandomcolor";
import { FloodFillToRandomIntensity } from "./v2/floodfilltorandomintensity";
import { FloodFillSampler } from "./v2/floodfillsampler";
import { FloodFillToGradient } from "./v2/floodfilltogradient";
import { StripesNode } from "./v2/stripes";
import { RgbaMerge } from "./v2/rgbamerge";
import { RgbaShuffle } from "./v2/rgbashuffle";
import { ExtractChannel } from "./v2/extractchannel";
import { Grayscale } from "./v2/grayscale";
import { InvertNormal } from "./v2/invertnormal";
import { CapsuleNode } from "./v2/capsule";
import { CartesianToPolar } from "./v2/cartesiantopolar";
import { PolarToCartesian } from "./v2/polartocartesian";
import { CombineNormals } from "./v2/combinenormals";
import { SoftFlower } from "./v2/softflower";
import { Star } from "./v2/star";
import { HslExtract } from "./v2/hslextract";
import { Hsl } from "./v2/hsl";
import { DirectionalBlur } from "./v2/directionalblur";
import { AnisotropicBlur } from "./v2/anisotropicblur";

export function createLibrary() {
	const lib = new DesignerLibrary();
	lib.versionName = "v2";
	lib.addNode("brickgenerator", "Brick Generator", BrickGeneratorNode);
	lib.addNode("normalmap", "Normal Map", NormalMapV2);
	lib.addNode("cell", "Cell", CellNode);
	lib.addNode("solidcell", "Solid Cell", SolidCellNode);
	lib.addNode("linecell", "Line Cell", LineCellNode);
	lib.addNode("circle", "Circle", CircleNode);
	lib.addNode("polygon", "Polygon", PolygonNode);
	lib.addNode("blend", "Blend", BlendNode);
	lib.addNode("invert", "Invert", InvertNode);
	lib.addNode("warp", "Warp", WarpNodeV2);
	lib.addNode("color", "Color", ColorNode);
	lib.addNode("colorize", "Colorize", ColorizeNode);
	lib.addNode("simplexnoise", "Simplex Noise", SimplexNoiseNode);
	lib.addNode("mask", "Mask", MaskNode);
	lib.addNode("transform2d", "Transform2D", Transform2DNodeV2);
	lib.addNode("maprange", "Map Range", MapRangeNode);
	lib.addNode("splat", "Splat", SplatNodeV2);
	lib.addNode("perlin3d", "Perlin 3D", Perlin3DNode);
	lib.addNode("hexagon", "Hexagon", HexagonNode);
	lib.addNode("fractalnoise", "Fractal Noise", FractalNoiseNode);
	lib.addNode("tile", "Tile", TileNode);
	lib.addNode("threshold", "Threshold", ThresholdNode);
	lib.addNode("heightshift", "Height Shift", HeightShiftNode);
	lib.addNode("checkerboard", "CheckerBoard", CheckerBoardNode);
	lib.addNode("directionalwarp", "Directional Warp", DirectionalWarpNode);
	lib.addNode("mirror", "Mirror", MirrorNode);
	lib.addNode(
		"brightnesscontrast",
		"Brightness Contrast",
		BrightnessContrastNode
	);
	lib.addNode("wave", "Wave", WaveNode);
	lib.addNode("copy", "Copy", CopyNode);
	lib.addNode("gradient", "Gradient", GradientNode);
	lib.addNode("trigradient", "TriGradient", TriGradientNode);
	lib.addNode("output", "Output", OutputNode);
	lib.addNode("gradientmap", "Gradient Map", GradientMapNode);
	lib.addNode("blurv2", "Blur", BlurV2);
	lib.addNode("slopeblur", "Slope Blur", SlopeBlur);
	lib.addNode("advancesplatter", "Advance Splatter", AdvanceSplatterV2);
	lib.addNode("tilesampler", "Tile Sampler", TileSampler);
	lib.addNode("gradientdynamic", "Gradient Dynamic", GradientDynamic);
	lib.addNode("clamp", "Clamp", Clamp);
	lib.addNode("pow", "Pow", Pow);
	lib.addNode("quantize", "Quantize", Quantize);
	lib.addNode("histogramshift", "Histogram Shift", HistogramShift);
	lib.addNode("histogramscan", "Histogram Scan", HistogramScan);
	lib.addNode("histogramselect", "Histogram Select", HistogramSelect);
	lib.addNode("skew", "Skew", Skew);
	lib.addNode("bevel", "Bevel", Bevel);
	lib.addNode("floodfill", "Flood Fill", FloodFill);
	lib.addNode("floodfilltocolor", "Flood Fill To Color", FloodFillToColor);
	lib.addNode("floodfilltobbox", "Flood Fill To BBox", FloodFillToBBox);
	lib.addNode(
		"floodfilltorandomcolor",
		"Flood Fill To Random Color",
		FloodFillToRandomColor
	);
	lib.addNode(
		"floodfilltorandomintensity",
		"Flood Fill To Random Intensity",
		FloodFillToRandomIntensity
	);
	lib.addNode("floodfillsampler", "Flood Fill Sampler", FloodFillSampler);
	lib.addNode(
		"floodfilltogradient",
		"Flood Fill To Gradient",
		FloodFillToGradient
	);
	lib.addNode("stripes", "Stripes", StripesNode);
	lib.addNode("rgbamerge", "RGBA Merge", RgbaMerge);
	lib.addNode("rgbashuffle", "RGBA Shuffle", RgbaShuffle);
	lib.addNode("extractchannel", "Extract Channel", ExtractChannel);
	lib.addNode("grayscale", "Grayscale", Grayscale);
	lib.addNode("invertnormal", "Invert Normal", InvertNormal);
	lib.addNode("capsule", "Capsule", CapsuleNode);
	lib.addNode("cartesiantopolar", "Cartesian to Polar", CartesianToPolar);
	lib.addNode("polartocartesian", "Polar to Cartesian", PolarToCartesian);
	lib.addNode("combinenormals", "Combine Normals", CombineNormals);
	lib.addNode("softflower", "Soft Flower", SoftFlower);
	lib.addNode("star", "Star", Star);
	lib.addNode("hslextract", "HSL Extract", HslExtract);
	lib.addNode("hsl", "HSL", Hsl);
	lib.addNode("directionalblur", "Directional Blur", DirectionalBlur);
	lib.addNode("anisotropic", "Anisotropic Blur", AnisotropicBlur);
	// lib.addNode("betterwarp", "Better Warp", BetterWarpNode);

	return lib;
}
