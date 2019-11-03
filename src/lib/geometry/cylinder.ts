// snatched from three.js

/**
 * @author mrdoob / http://mrdoob.com/
 * @author Mugen87 / https://github.com/Mugen87
 * @author njbrown /  https://twitter.com/njbrown
 */

import { Geometry } from "three";
import { BufferGeometry } from "three";
import { Float32BufferAttribute } from "three";
import { Vector3 } from "three";
import { Vector2 } from "three";

class CylinderGeometry extends Geometry {
	parameters = {};

	constructor(
		radiusTop?,
		radiusBottom?,
		height?,
		radialSegments?,
		heightSegments?,
		openEnded?,
		thetaStart?,
		thetaLength?
	) {
		super();

		this.type = "CylinderGeometry";

		this.parameters = {
			radiusTop: radiusTop,
			radiusBottom: radiusBottom,
			height: height,
			radialSegments: radialSegments,
			heightSegments: heightSegments,
			openEnded: openEnded,
			thetaStart: thetaStart,
			thetaLength: thetaLength
		};

		this.fromBufferGeometry(
			new CylinderBufferGeometry(
				radiusTop,
				radiusBottom,
				height,
				radialSegments,
				heightSegments,
				openEnded,
				thetaStart,
				thetaLength
			)
		);
		this.mergeVertices();
	}
}

class CylinderBufferGeometry extends BufferGeometry {
	parameters = {};
	constructor(
		radiusTop?,
		radiusBottom?,
		height?,
		radialSegments?,
		heightSegments?,
		openEnded?,
		thetaStart?,
		thetaLength?
	) {
		super();

		this.type = "CylinderBufferGeometry";

		this.parameters = {
			radiusTop: radiusTop,
			radiusBottom: radiusBottom,
			height: height,
			radialSegments: radialSegments,
			heightSegments: heightSegments,
			openEnded: openEnded,
			thetaStart: thetaStart,
			thetaLength: thetaLength
		};

		var scope = this;

		radiusTop = radiusTop !== undefined ? radiusTop : 1;
		radiusBottom = radiusBottom !== undefined ? radiusBottom : 1;
		height = height || 1;

		radialSegments = Math.floor(radialSegments) || 8;
		heightSegments = Math.floor(heightSegments) || 1;

		openEnded = openEnded !== undefined ? openEnded : false;
		thetaStart = thetaStart !== undefined ? thetaStart : 0.0;
		thetaLength = thetaLength !== undefined ? thetaLength : Math.PI * 2;

		// buffers

		var indices = [];
		var vertices = [];
		var normals = [];
		var uvs = [];

		// helper variables

		var index = 0;
		var indexArray = [];
		var halfHeight = height / 2;
		var groupStart = 0;

		// generate geometry

		generateTorso();

		// open-ended by default
		// if (openEnded === false) {
		// 	if (radiusTop > 0) generateCap(true);
		// 	if (radiusBottom > 0) generateCap(false);
		// }

		// build geometry

		this.setIndex(indices);
		this.addAttribute("position", new Float32BufferAttribute(vertices, 3));
		this.addAttribute("normal", new Float32BufferAttribute(normals, 3));
		this.addAttribute("uv", new Float32BufferAttribute(uvs, 2));

		function generateTorso() {
			var x, y;
			var normal = new Vector3();
			var vertex = new Vector3();

			var groupCount = 0;

			// this will be used to calculate the normal
			var slope = (radiusBottom - radiusTop) / height;

			// generate vertices, normals and uvs

			for (y = 0; y <= heightSegments; y++) {
				var indexRow = [];

				var v = y / heightSegments;

				// calculate the radius of the current row

				var radius = v * (radiusBottom - radiusTop) + radiusTop;

				for (x = 0; x <= radialSegments; x++) {
					var u = x / radialSegments;

					var theta = u * thetaLength + thetaStart;

					var sinTheta = Math.sin(theta);
					var cosTheta = Math.cos(theta);

					// vertex

					vertex.x = radius * sinTheta;
					vertex.y = -v * height + halfHeight;
					vertex.z = radius * cosTheta;
					vertices.push(vertex.x, vertex.y, vertex.z);

					// normal

					normal.set(sinTheta, slope, cosTheta).normalize();
					normals.push(normal.x, normal.y, normal.z);

					// uv

					uvs.push(u * 4, 1 - v);

					// save index of vertex in respective row

					indexRow.push(index++);
				}

				// now save vertices of the row in our index array

				indexArray.push(indexRow);
			}

			// generate indices

			for (x = 0; x < radialSegments; x++) {
				for (y = 0; y < heightSegments; y++) {
					// we use the index array to access the correct indices

					var a = indexArray[y][x];
					var b = indexArray[y + 1][x];
					var c = indexArray[y + 1][x + 1];
					var d = indexArray[y][x + 1];

					// faces

					indices.push(a, b, d);
					indices.push(b, c, d);

					// update group counter

					groupCount += 6;
				}
			}

			// add a group to the geometry. this will ensure multi material support

			scope.addGroup(groupStart, groupCount, 0);

			// calculate new start value for groups

			groupStart += groupCount;
		}
	}
}

export { CylinderGeometry, CylinderBufferGeometry };
