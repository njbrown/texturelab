#pragma once
#include <math.h>

class Mesh;
class QOpenGLFunctions;

enum class PlaneOrientation {
    XY, // Normal facing Z
    YZ, // Normal facing X
    XZ  // Normal facing Y
};

// https://github.com/mrdoob/three.js/blob/master/src/geometries/SphereGeometry.js
Mesh* createSphere(QOpenGLFunctions* gl, float radius = 1,
                   int widthSegments = 32, int heightSegments = 16,
                   float phiStart = 0, float phiLength = M_PI * 2,
                   float thetaStart = 0, float thetaLength = M_PI);

// Create a subdivided plane mesh with normals, tangents, and UVs
// https://github.com/mrdoob/three.js/blob/master/src/geometries/PlaneGeometry.js
Mesh* createPlane(QOpenGLFunctions* gl, float width = 1, float height = 1,
                  int widthSegments = 1, int heightSegments = 1,
                  PlaneOrientation orientation = PlaneOrientation::XY);

// Create a cylinder mesh with normals, tangents, and UVs
// https://github.com/mrdoob/three.js/blob/master/src/geometries/CylinderGeometry.js
Mesh* createCylinder(QOpenGLFunctions* gl, float radiusTop = 1,
                     float radiusBottom = 1, float height = 1,
                     int radialSegments = 32, int heightSegments = 1,
                     bool openEnded = false);

// Create a subdivided cube mesh with normals, tangents, and UVs
// https://github.com/mrdoob/three.js/blob/master/src/geometries/BoxGeometry.js
Mesh* createCube(QOpenGLFunctions* gl, float width = 1, float height = 1,
                 float depth = 1, int widthSegments = 1, int heightSegments = 1,
                 int depthSegments = 1);

// Create a skydome (inverted sphere) for rendering environment maps
Mesh* createSkydome(QOpenGLFunctions* gl, float radius = 100,
                    int widthSegments = 32, int heightSegments = 16);

// other sources:
// https://www.danielsieger.com/blog/2021/03/27/generating-spheres.html
// https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/
// https://github.com/caosdoar/spheres/blob/master/src/spheres.cpp
// VERY GOOD:
// http://www.songho.ca/opengl/gl_sphere.html