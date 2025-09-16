#ifndef SAUSAGE_SPATIAL_FRUSTUM_H
#define SAUSAGE_SPATIAL_FRUSTUM_H

using namespace std;
using namespace glm;

class Camera;

struct Plane {
  // distance to closest point on the plane
  float distance;
  vec3 normal;
};

struct Frustum {
  Plane near;
  Plane far;
  Plane down;
  Plane up;
  Plane left;
  Plane right;
};

namespace GameDebug {
inline Frustum *GetCentered45DownFrustum() {
  return new Frustum{{56.7659798, {0, -0.707107, -0.707107}},
                     {977.861145, {-0, 0.707107, 0.707107}},
                     {56.8072853, {-0.433013, 0.435596, -0.789149}},
                     {56.8072853, {-0.433013, -0.789149, 0.435596}},
                     {56.8072853, {-0.866025, 0.482963, -0.129409}},
                     {56.8072853, {-0.866025, -0.482963, 0.129409}}};
}
struct FrustumVertices {
  vector<vec3> vertices;
  vector<unsigned int> indices;
};

// for debug draw

void GetScaledCameraFrustum(Camera *c, Frustum *out_frustum, float scale);
FrustumVertices GetScaledCameraFrustumVertices(float scale);
} // namespace SausageDebug

#endif // SAUSAGE_SPATIAL_FRUSTUM_H