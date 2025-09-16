#pragma once

#include "Camera.h"
#include "Frustum.h"
#include "Settings.h"
#include "sausage.h"

using namespace std;
using namespace glm;

class BoundingBox;

class BoundingVolume {
public:
  vec3 center;
  bool is_cull_by_distance;
  BoundingVolume(mat4 &transform,
                 // in cases of large area meshes like terrain, or Octree
                 // topmost box cull by distance must be disabled
                 bool is_cull_by_distance = false)
      : center{transform[3]}, is_cull_by_distance{is_cull_by_distance} {}
  BoundingVolume(vec3 &center, bool is_cull_by_distance = false)
      : center{center}, is_cull_by_distance{is_cull_by_distance} {}

  virtual bool IsCross(const Frustum *frustum) = 0;

  bool IsCulledByDistance(const vec3 &pos,
                          int draw_distance = GameSettings::MAX_DRAW_DISTANCE) {
    return is_cull_by_distance || distance(pos, center) > draw_distance;
  };

  virtual void Transform(const mat4 &transform) = 0;
  /**
   * @return if inside this BV
   */
  virtual bool IsInside(const vec3 &pos) = 0;
  /**
   * @return if inside this BV
   */
  virtual bool IsInside(const BoundingBox *other) = 0;
  /**
   * @return if inside this BV
   */
  virtual bool Intersects(const BoundingBox *other) = 0;
};

class BoundingBox : public BoundingVolume {
public:
  vec3 min_AABB;
  vec3 max_AABB;
  vec3 half_extents;

  BoundingBox(mat4 &transform, vec3 &min_AABB, vec3 &max_AABB,
              bool is_cull_by_distance = false)
      : BoundingVolume(transform, is_cull_by_distance), min_AABB{min_AABB},
        max_AABB{max_AABB}, half_extents{abs(max_AABB - min_AABB)} {
    half_extents[0] *= 0.5;
    half_extents[1] *= 0.5;
    half_extents[2] *= 0.5;

    TransformExtents(transform);
  }
  BoundingBox(mat4 &transform, vec3 &half_extents,
              bool is_cull_by_distance = false)
      : BoundingVolume(transform, is_cull_by_distance),
        min_AABB{center - half_extents}, max_AABB{center + half_extents},
        half_extents{half_extents} {}

  BoundingBox(vec3 &center, vec3 &half_extents,
              bool is_cull_by_distance = false)
      : BoundingVolume(center, is_cull_by_distance),
        min_AABB{center - half_extents}, max_AABB{center + half_extents},
        half_extents{half_extents} {}

  void Transform(const mat4 &transform) override {
    center = transform[3];
    TransformExtents(transform);
  }

  void TransformExtents(const mat4 &transform) {
    // local unit directions vectors
    auto right = vec3(transform[0][0], transform[1][0], transform[2][0]) *
                 half_extents.x;
    auto up = vec3(transform[0][1], transform[1][1], transform[2][1]) *
              half_extents.y;
    auto forward = vec3(transform[0][2], transform[1][2], transform[2][2]) *
                   half_extents.z;
    // projection of rotated/scaled extents of direction vectors onto world axis
    half_extents.x = abs(dot(right, GameSettings::world_right)) +
                     abs(dot(up, GameSettings::world_right)) +
                     abs(dot(forward, GameSettings::world_right));
    half_extents.y = abs(dot(right, GameSettings::world_up)) +
                     abs(dot(up, GameSettings::world_up)) +
                     abs(dot(forward, GameSettings::world_up));
    half_extents.z = abs(dot(right, GameSettings::world_forward)) +
                     abs(dot(up, GameSettings::world_forward)) +
                     abs(dot(forward, GameSettings::world_forward));

    max_AABB = center + half_extents;
    min_AABB = center - half_extents;
  }

  // given:
  //
  //   normal vector:
  //
  //     n
  //
  //   point on a plane:
  //
  //     x
  //
  //   direction from world origin:
  //
  //     p
  //
  //
  // plane equation:
  //
  //     n * x = -p,
  //
  // where `p > 0` means distance vector origin to plane cooriented with normal.
  // (pointing to same part of space, divided by hyperplane)
  //
  //
  // distance `d` of point `c` to plane:
  //
  //   d = n * c + p,
  //
  // where `d > 0` means distance vector point to plane cooriented with normal.
  //
  // test AABB on positive side of frustum:
  //   1. project extent onto normal (find lenght of AABB projection onto axis,
  //   || plane normal)
  //   2. project AABB center 'c' on 'n', sub 'p'.
  //
  //   3. find distance from `c` to plane == `d`.
  //      if 'd' is positive, it lies on plane positive side.
  //      if 'd' is negative - exit.
  //
  bool IsCross(const Frustum *frustum) override {
    // optimal order. first check sides, then depth, since sides cuts off most
    // space
    return IsCross(frustum->left) && IsCross(frustum->right) &&
           IsCross(frustum->up) && IsCross(frustum->down) &&
           IsCross(frustum->near) && IsCross(frustum->far);
  };

  bool IsCross(const Plane &plane) {
    // because rotation of normal doesnt change length of AABB diagonal
    // projection
    float r = dot(half_extents, abs(plane.normal));

    // Compute distance of box center from plane
    // if sides are opposite - sign will be reversed
    auto distance_center_to_plane = dot(center, plane.normal) - plane.distance;

    // >= -r means - AABB side touches Frustum side.
    return distance_center_to_plane >= -r;
  }

  bool IsInside(const vec3 &pos) override {
    return pos.x <= max_AABB.x && pos.y <= max_AABB.y && pos.z <= max_AABB.z &&
           pos.x >= min_AABB.x && pos.y >= min_AABB.y && pos.z >= min_AABB.z;
  }

  /**
   * if inside this BV
   */
  bool IsInside(const BoundingBox *other) override {
    return other->max_AABB.x <= max_AABB.x && other->max_AABB.y <= max_AABB.y &&
           other->max_AABB.z <= max_AABB.z && other->min_AABB.x >= min_AABB.x &&
           other->min_AABB.y >= min_AABB.y && other->min_AABB.z >= min_AABB.z;
  }

  bool Intersects(const BoundingBox *other) override {
    return other->min_AABB.x <= max_AABB.x && other->max_AABB.x >= min_AABB.x &&
           other->min_AABB.y <= max_AABB.y && other->max_AABB.y >= min_AABB.y &&
           other->min_AABB.z <= max_AABB.z && other->max_AABB.z >= min_AABB.z;
  }
};
// class BoundingSphere {
// public:
//  virtual bool IsCulled(frustum) = 0;
//};
