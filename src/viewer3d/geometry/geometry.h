#pragma once
#include <math.h>

class Mesh;
class QOpenGLFunctions;

// https://github.com/mrdoob/three.js/blob/master/src/geometries/SphereGeometry.js
Mesh* createSphere(QOpenGLFunctions* gl, float radius = 1,
                   int widthSegments = 32, int heightSegments = 16,
                   float phiStart = 0, float phiLength = M_PI * 2,
                   float thetaStart = 0, float thetaLength = M_PI);

// other sources:
// https://www.danielsieger.com/blog/2021/03/27/generating-spheres.html
// https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/
// https://github.com/caosdoar/spheres/blob/master/src/spheres.cpp
// VERY GOOD:
// http://www.songho.ca/opengl/gl_sphere.html