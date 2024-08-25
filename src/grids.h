#ifndef GRIDS_H
#define GRIDS_H
#include <FastNoise/FastNoise.h>
#include <openvdb/openvdb.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/points/PointCount.h>
#include <openvdb/tools/SignedFloodFill.h>
#include <openvdb/tools/VolumeToMesh.h>

namespace VW {
openvdb::FloatGrid::Ptr createNoiseGrid();

template <class GridType>
void makeSphere(GridType &grid, float radius, const openvdb::Vec3f &c) {
  using ValueT = typename GridType::ValueType;
  // Distance value for the constant region exterior to the narrow band
  const ValueT outside = grid.background();
  // Distance value for the constant region interior to the narrow band
  // (by convention, the signed distance is negative in the interior of
  // a level set)
  const ValueT inside = -outside;
  // Use the background value as the width in voxels of the narrow band.
  // (The narrow band is centered on the surface of the sphere, which
  // has distance 0.)
  int padding = int(openvdb::math::RoundUp(openvdb::math::Abs(outside)));
  // The bounding box of the narrow band is 2*dim voxels on a side.
  int dim = int(radius + padding);
  // Get a voxel accessor.
  typename GridType::Accessor accessor = grid.getAccessor();
  // Compute the signed distance from the surface of the sphere of each
  // voxel within the bounding box and insert the value into the grid
  // if it is smaller in magnitude than the background value.
  openvdb::Coord ijk;
  int &i = ijk[0], &j = ijk[1], &k = ijk[2];
  for (i = c[0] - dim; i < c[0] + dim; ++i) {
    const float x2 = openvdb::math::Pow2(i - c[0]);
    for (j = c[1] - dim; j < c[1] + dim; ++j) {
      const float x2y2 = openvdb::math::Pow2(j - c[1]) + x2;
      for (k = c[2] - dim; k < c[2] + dim; ++k) {
        // The distance from the sphere surface in voxels
        const float dist =
            openvdb::math::Sqrt(x2y2 + openvdb::math::Pow2(k - c[2])) - radius;
        // Convert the floating-point distance to the grid's value type.
        ValueT val = ValueT(dist);
        // Only insert distances that are smaller in magnitude than
        // the background value.
        if (val < inside || outside < val)
          continue;
        // Set the distance for voxel (i,j,k).
        accessor.setValue(ijk, val);
      }
    }
  }
  // Propagate the outside/inside sign information from the narrow band
  // throughout the grid.
  openvdb::tools::signedFloodFill(grid.tree());
}
openvdb::FloatGrid::Ptr get_sphere_example_0();

// https://www.openvdb.org/documentation/doxygen/codeExamples.html#openvdbPointsHelloWorld
openvdb::points::PointDataGrid::Ptr get_grid_example_0();

} // namespace VW
#endif
