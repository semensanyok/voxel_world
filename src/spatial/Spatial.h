#pragma once

#include "sausage.h"
#include "BoundingVolume.h"
#include "Interfaces.h"

using namespace std;

// hierarchical bounding volume
// used in broad phase culling
class Spatial : UserPointer {
private:
  Spatial* parent;
  // if parent == null && assigned to Octree node (box), it is id for octree children map.
  unsigned int parent_idx;
  //int children_ids;
public:
  // children meshes
  // make ThreadSafeArray???
  //ThreadSafeVector<Spatial*> children;
  BoundingBox* bv;

  Spatial(BoundingBox* bv) :
    bv{ bv },
    parent{ nullptr },
    parent_idx{ 0 }{

  }
  /**
   * new transformed BoundingBox
  */
  Spatial(BoundingBox* base, mat4& transform) :
    bv{ new BoundingBox(transform, base->half_extents, base->is_cull_by_distance) },
    parent{ nullptr },
    parent_idx{ 0 } {
  }

  //Spatial* AddChild(BoundingBox* node) {
  //  Spatial* child = new Spatial(node, this, children_ids++);
  //  children[child->parent_idx] = child;
  //  return child;
  //}
  ///**
  // * @brief return number of elements erased. (expected 1)
  //*/
  //size_t RemoveChild(Spatial* node) {
  //  return children.erase(node->parent_idx);
  //}
  /**
   * this method is used on topmost mesh, containing child meshes.
   * i.e. table -> plate
   * table's parent_idx in that case is index in Octree's node children.
   * plate's index is index into table children array
   * meshes assigned to Octree after initialization, and can move between Octree nodes
  */
  void SetParentIndex(unsigned int parent_idx) {
    this->parent_idx = parent_idx;
  }
private:
  Spatial(BoundingBox* bv, Spatial* parent, unsigned int parent_idx) :
    bv{ bv },
    parent{ parent },
    parent_idx{ parent_idx } {

  }
};
