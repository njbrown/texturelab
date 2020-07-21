// snatched from threejs
/**
 * @author mrdoob / http://mrdoob.com/
 * @author benaadams / https://twitter.com/ben_a_adams
 * @author Mugen87 / https://github.com/Mugen87
 * @author njbrown /  https://twitter.com/njbrown
 */

import { Geometry } from "three";
import { BufferGeometry } from "three";
import { Float32BufferAttribute } from "three";
import { Vector3 } from "three";

class SphereGeometry extends Geometry {
	parameters = {};
	constructor(
		radius?,
		widthSegments?,
		heightSegments?,
		phiStart?,
		phiLength?,
		thetaStart?,
		thetaLength?
	) {
		super();

		this.type = "SphereGeometry";

		this.parameters = {
			radius: radius,
			widthSegments: widthSegments,
			heightSegments: heightSegments,
			phiStart: phiStart,
			phiLength: phiLength,
			thetaStart: thetaStart,
			thetaLength: thetaLength
		};

		this.fromBufferGeometry(
			new SphereBufferGeometry(
				radius,
				widthSegments,
				heightSegments,
				phiStart,
				phiLength,
				thetaStart,
				thetaLength
			)
		);
		//this.mergeVertices();
	}
}

// SphereBufferGeometry

class SphereBufferGeometry extends BufferGeometry {
	parameters = {};
	constructor(
		radius,
		widthSegments,
		heightSegments,
		phiStart,
		phiLength,
		thetaStart,
		thetaLength
	) {
		super();

		this.type = "SphereBufferGeometry";

		this.parameters = {
			radius: radius,
			widthSegments: widthSegments,
			heightSegments: heightSegments,
			phiStart: phiStart,
			phiLength: phiLength,
			thetaStart: thetaStart,
			thetaLength: thetaLength
		};

		radius = radius || 1;

		widthSegments = Math.max(3, Math.floor(widthSegments) || 8);
		heightSegments = Math.max(2, Math.floor(heightSegments) || 6);

		phiStart = phiStart !== undefined ? phiStart : 0;
		phiLength = phiLength !== undefined ? phiLength : Math.PI * 2;

		thetaStart = thetaStart !== undefined ? thetaStart : 0;
		thetaLength = thetaLength !== undefined ? thetaLength : Math.PI;

		const thetaEnd = Math.min(thetaStart + thetaLength, Math.PI);

		let ix, iy;

		let index = 0;
		const grid = [];

		const vertex = new Vector3();
		const normal = new Vector3();

		// buffers

		const indices = [];
		const vertices = [];
		const normals = [];
		const uvs = [];

		// generate vertices, normals and uvs

		for (iy = 0; iy <= heightSegments; iy++) {
			const verticesRow = [];

			const v = iy / heightSegments;

			// special case for the poles

			let uOffset = 0;

			if (iy == 0 && thetaStart == 0) {
				uOffset = 0.5 / widthSegments;
			} else if (iy == heightSegments && thetaEnd == Math.PI) {
				uOffset = -0.5 / widthSegments;
			}

			for (ix = 0; ix <= widthSegments; ix++) {
				const u = ix / widthSegments;

				// vertex

				vertex.x =
					-radius *
					Math.cos(phiStart + u * phiLength) *
					Math.sin(thetaStart + v * thetaLength);
				vertex.y = radius * Math.cos(thetaStart + v * thetaLength);
				vertex.z =
					radius *
					Math.sin(phiStart + u * phiLength) *
					Math.sin(thetaStart + v * thetaLength);

				vertices.push(vertex.x, vertex.y, vertex.z);

				// normal

				normal.copy(vertex).normalize();
				normals.push(normal.x, normal.y, normal.z);

				// uv

				uvs.push((u + uOffset) * 2, 1 - v);

				verticesRow.push(index++);
			}

			grid.push(verticesRow);
		}

		// indices

		for (iy = 0; iy < heightSegments; iy++) {
			for (ix = 0; ix < widthSegments; ix++) {
				const a = grid[iy][ix + 1];
				const b = grid[iy][ix];
				const c = grid[iy + 1][ix];
				const d = grid[iy + 1][ix + 1];

				if (iy !== 0 || thetaStart > 0) indices.push(a, b, d);
				if (iy !== heightSegments - 1 || thetaEnd < Math.PI)
					indices.push(b, c, d);
			}
		}

		// build geometry

		this.setIndex(indices);
		this.addAttribute("position", new Float32BufferAttribute(vertices, 3));
		this.addAttribute("normal", new Float32BufferAttribute(normals, 3));
		this.addAttribute("uv", new Float32BufferAttribute(uvs, 2));
		this.addAttribute("uv2", new Float32BufferAttribute(uvs, 2));
	}
}

export { SphereGeometry, SphereBufferGeometry };
