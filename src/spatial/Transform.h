#pragma once

#include "sausage.h"
#include "Interfaces.h"

using namespace glm;
using namespace std;

template<typename TRANFORM_TYPE>
class Transform : public UserPointer {
private:
  TRANFORM_TYPE transform;
  // true to buffer initial transform
public:
  Transform(TRANFORM_TYPE& transform) : transform(transform) {};
  bool is_dirty = true;
  virtual ~Transform() {};
  const TRANFORM_TYPE& ReadTransform() {
    return transform;
  };
  TRANFORM_TYPE& GetOutTransform() {
    is_dirty = true;
    return transform;
  };
  virtual void OnTransformUpdate() = 0;
};
