import {
	TestShapeNode,
	TestGradientNode,
	TestSimplexNode,
	TestWorleyNode,
	TestMultiplyNode,
	TestInvertNode,
	TestNormalNode,
	TestBrickNode,
	TestWarpNode
} from "./v0/nodes";

import { DesignerLibrary } from "../designer/library";

export function createLibrary() {
	const lib = new DesignerLibrary();
	lib.versionName = "v0";
	lib.addNode("shape", "Shape", TestShapeNode);
	lib.addNode("gradient", "Gradient", TestGradientNode);
	lib.addNode("multiply", "Multiply", TestMultiplyNode);
	lib.addNode("invert", "Invert", TestInvertNode);
	lib.addNode("worley", "Worley", TestWorleyNode);
	lib.addNode("normal", "Normal", TestNormalNode);
	lib.addNode("brick", "Brick", TestBrickNode);
	lib.addNode("warp", "Warp", TestWarpNode);
	lib.addNode("simplex", "Simpled Noise", TestSimplexNode);

	return lib;
}
