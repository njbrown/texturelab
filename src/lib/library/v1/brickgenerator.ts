import { GpuDesignerNode } from "../../designer/gpudesignernode";

export class BrickGeneratorNode extends GpuDesignerNode {
	public init() {
		this.title = "Brick Generator";

		this.addFloatProperty("offset", "Offset", 0.5, 0, 1, 0.1);

		// brick size
		this.addFloatProperty("brickWidth", "Brick Width", 0.9, 0, 1, 0.01);
		this.addFloatProperty("brickHeight", "Brick Height", 0.9, 0, 1, 0.01);

		// height
		this.addFloatProperty("heightMin", "Height Min", 0.0, 0, 1, 0.05);
		this.addFloatProperty("heightMax", "Height Max", 1.0, 0, 1, 0.05);
		this.addFloatProperty("heightBalance", "Height Balance", 1.0, 0, 1, 0.05);
		this.addFloatProperty("heightVariance", "Height Variance", 0, 0, 1, 0.05);

		this.addFloatProperty("rows", "Rows", 6, 1, 20, 1);
		this.addFloatProperty("columns", "Columns", 6, 1, 20, 1);

		const source = `
        //vec2 brickSize = vec2(prop_brickWidth, prop_brickHeight);
        //vec2 tileSize = vec2(prop_rows, prop_columns);

        // float shiftX = 0.5;
        // float shiftY = 0.0;

        // offset for alternating rows
        //float offset = prop_offset;

        // HEIGHT FUNCTIONS
        float calculateHeight(vec2 brickId)
        {
            // height
            float heightMin = prop_heightMin;
            float heightMax = prop_heightMax;
            float heightBalance= prop_heightBalance; // threshold that decides whether to use height variance or not
            float heightVariance = prop_heightVariance; // multiplies the heightMax-heightMin range


            // check whether or not there should be a height range in the first place
            float balRand = _rand(vec2(_seed) + brickId * vec2(0.01));
            
            // if balRand is less than heightBalance it means it qualifies for a random
            // height. This way if heightBalance is 0 then we only use the min luminance
            if( balRand > heightBalance) {
                return 1.0;
            }
            
            // calculate height variance
            // need to offset brickId to give new random result
            float randVariance = _rand(vec2(_seed) + (brickId + vec2(1) ) * vec2(0.01));
            randVariance *= heightVariance;
            
            float range = (heightMax - heightMin);
            float height = heightMax - 
                        range * randVariance;
            
            return height;
        }

        // slope
        // float slopeX;// slope x direction
        // float slopeY;// slope y direction
        // float slopeBalance;// threshold that determines whether or not to use slope
        // float slopeVariation;// decreases the range of the slope
            
        vec2 is_brick(vec2 pos)
        {
            vec2 brickSize = vec2(prop_brickWidth, prop_brickHeight);

            vec2 edgeSize = (vec2(1.0) - brickSize) * vec2(0.5);
            vec2 brick = vec2(0.0);
            
            if (pos.x > edgeSize.x && pos.x < (1.0 - edgeSize.x))
                brick.x = 1.0;
                
            if (pos.y > edgeSize.y && pos.y < (1.0 - edgeSize.y))
                brick.y = 1.0;
                
            return brick;
        }

        vec4 process(vec2 uv)
        {
            vec2 tileSize = vec2(prop_rows, prop_columns);
            float offset = prop_offset;

            //vec2 pos = uv * vec2(5);
            vec2 pos = uv * tileSize;
            
            float xOffset = 0.0;
            if (fract(pos.y * 0.5) > 0.5) {
                xOffset = offset;
            }
            pos.x += xOffset;
            
            // a brick's id would be floor(pos)
            // this gives us its origin
            // this can act as a random seed for the entire brick
            vec2 brickId = floor(pos);// - vec2(xOffset, 0);

            // wrap around x
            if (brickId.x > tileSize.x-1.0)
                brickId.x = 0.0;

            //float lum = _rand(vec2(_seed) + brickId * vec2(0.01));
            //float lum = randomFloat(0);
            float lum = calculateHeight(brickId);
            pos = fract(pos);
            
            //vec2 isBrick = step(pos,vec2(0.95,0.9));
            vec2 isBrick =is_brick(pos);

            
            return vec4(vec3(isBrick.x * isBrick.y * lum),1.0);
        }
        `;

		this.buildShader(source);
	}
}
