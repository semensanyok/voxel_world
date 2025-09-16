#pragma once

#include "BoundingVolume.h"
#include "Frustum.h"
#include "Spatial.h"
#include "ThreadSafeVector.h"
#include "sausage.h"

/**
 * @brief octree of bounding boxes, acceleration of frustum culling and other
 * spatial checks
 *
 * to not have thread safe vectors, allow only one thread to traverse tree at a
 * time. other threads only setting `is_dirty` flag on Spatial, marking its to
 * be reinserted to Octree during frustum or raytest
 *
 * TODO:
 *  one mutex access, on insertion?
 *  currently just keep same thread insert/iterate tree
 */
class Octree {
public:
  Octree *parent;

  BoundingBox *bv;
  // ThreadSafeVector<Spatial*> objects;
  vector<Spatial *> objects;
  bool is_leaf;

  mutex insert_mtx;
  // clockwise start from down south west
  vector<Octree *> children;

  Octree(BoundingBox *bv, unsigned long num_levels)
      : Octree(nullptr, bv, num_levels) {}

  Octree(Octree *parent, BoundingBox *bv, unsigned long num_levels)
      : parent{parent}, bv{bv} {
    is_leaf = num_levels <= 1;
    Split(num_levels);
  };

  void FrustrumCull(Frustum *frustum, vec3 &camera_pos,
                    vector<Spatial *> &out_inside_frustum) {
    ProcessNode(frustum, camera_pos, this, out_inside_frustum);
  }

  // TODO: dont insert parent largest frustum somehow
  // if frustum includes intersection - insert parent node objects.
  bool Insert(vector<Spatial *> &in_objects) {
    for (auto object : in_objects) {
      Insert(object);
    }
  }
  /**
   * @return if object is fully inside node -> inserted.
   *         return value used to check if containing parent node can be
   narrowed to any children.
   *         to stop recurse when none of children returned `true`
   *
   TODO: problem - on rotation/scale, objects AABB changes. need to reinsert
   into the tree.
  */
  bool Insert(Spatial *object) { return Insert(this, object); }

  bool Insert(Octree *node, Spatial *object) {
    if (!node->bv->IsInside(object->bv)) {
      return false;
    }
    for (auto child : node->children) {
      if (Insert(child, object)) {
        return true;
      }
    }
    objects.push_back(object);
    object->SetParentIndex(objects.size() - 1);
    return true;
  }

  void ProcessNode(Frustum *frustum, vec3 &camera_pos, Octree *node,
                   vector<Spatial *> &out_inside_frustum) {
    if (node->bv->IsCulledByDistance(camera_pos)) {
      return;
    }
    // TODO: 1) exit only if fully inside. guarantying childs also inside.
    //       2) if intersects and not fully inside - check childs.
    bool is_fully_inside = !node->bv->IsInside(frustum);

    bool is_culled = !node->bv->IsCross(frustum);
    if (is_culled) {
      return;
    }
    // TODO: maybe narrower phase, frustum per object.
    out_inside_frustum.insert(out_inside_frustum.end(), objects.begin(),
                              objects.end());
    for (auto child : node->children) {
      ProcessNode(frustum, camera_pos, child, out_inside_frustum);
    }
  }

  void Split(int num_levels) {
    if (is_leaf) {
      return;
    }
    // Bounds.Extens.x is the same as y and z, all nodes are square
    // We want each child node to be half as big as this node
    vec3 child_extents = bv->half_extents / 2.0f;

    children.resize(8, nullptr);

    // order matters
    // clockwise start from down south west
    vector<vec3> childs_centers = {
        bv->center + vec3(-child_extents.x, -child_extents.y, -child_extents.z),
        bv->center + vec3(-child_extents.x, -child_extents.y, child_extents.z),
        bv->center + vec3(child_extents.x, -child_extents.y, child_extents.z),
        bv->center + vec3(child_extents.x, -child_extents.y, -child_extents.z),

        bv->center + vec3(-child_extents.x, child_extents.y, -child_extents.z),
        bv->center + vec3(-child_extents.x, child_extents.y, child_extents.z),
        bv->center + vec3(child_extents.x, child_extents.y, child_extents.z),
        bv->center + vec3(child_extents.x, child_extents.y, -child_extents.z),
    };

    for (int i = 0; i < 8; i++) {
      auto center = childs_centers[i];
      auto max_AABB = center + child_extents;
      auto min_AABB = center - child_extents;
      auto half_extents = max_AABB - min_AABB;
      half_extents[0] *= 0.5;
      half_extents[1] *= 0.5;
      half_extents[2] *= 0.5;

      auto child_bv = new BoundingBox(childs_centers[i], half_extents, true);
      auto child = new Octree(this, child_bv, num_levels - 1);
      children[i] = child;
    }
  }
};
