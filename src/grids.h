#ifndef GRIDS_H
#define GRIDS_H
#include <openvdb/tools/SignedFloodFill.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/openvdb.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/points/PointCount.h>
#include <FastNoise/FastNoise.h>

namespace VW {
namespace NoiseGrid {
openvdb::FloatGrid::Ptr createNoiseGrid() {
  auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
  FastNoise::SmartNode<FastNoise::FractalFBm> fnGenerator =
      FastNoise::New<FastNoise::FractalFBm>();

  fnGenerator->SetSource(fnSimplex);
  // Create an array of floats to store the noise output in
  fnGenerator->SetOctaveCount(5);
  std::vector<float> noiseOutput(16 * 16 * 16);

  // Generate a 16 x 16 x 16 area of noise
  fnGenerator->GenUniformGrid3D(noiseOutput.data(), 0, 0, 0, 16, 16, 16, 0.2f,
                                1337);

  openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0f);
  openvdb::FloatGrid::Accessor accessor = grid->getAccessor();
  int scale = 10;
  int index = 0;
  float threshold = 0.0f;
  for (int z = 0; z < 16; z++) {
    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        openvdb::Coord xyz(x * scale, y * scale, z * scale);
        auto val = noiseOutput[index++];
        if (val > threshold) {
          accessor.setValue(xyz, val);
        }
      }
    }
  }
  return grid;
}
} // namespace NoiseGrid

template<class GridType>
void
makeSphere(GridType& grid, float radius, const openvdb::Vec3f& c)
{
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
                const float dist = openvdb::math::Sqrt(x2y2
                    + openvdb::math::Pow2(k - c[2])) - radius;
                // Convert the floating-point distance to the grid's value type.
                ValueT val = ValueT(dist);
                // Only insert distances that are smaller in magnitude than
                // the background value.
                if (val < inside || outside < val) continue;
                // Set the distance for voxel (i,j,k).
                accessor.setValue(ijk, val);
            }
        }
    }
    // Propagate the outside/inside sign information from the narrow band
    // throughout the grid.
    openvdb::tools::signedFloodFill(grid.tree());
}
openvdb::FloatGrid::Ptr get_sphere_example_0() {
    openvdb::initialize();
    // Create a shared pointer to a newly-allocated grid of a built-in type:
    // in this case, a FloatGrid, which stores one single-precision floating point
    // value per voxel.  Other built-in grid types include BoolGrid, DoubleGrid,
    // Int32Grid and Vec3SGrid (see openvdb.h for the complete list).
    // The grid comprises a sparse tree representation of voxel data,
    // user-supplied metadata and a voxel space to world space transform,
    // which defaults to the identity transform.
    openvdb::FloatGrid::Ptr grid =
        openvdb::FloatGrid::create(/*background value=*/2.0);
    // Populate the grid with a sparse, narrow-band level set representation
    // of a sphere with radius 50 voxels, located at (1.5, 2, 3) in index space.
    makeSphere(*grid, /*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3));
    // Associate some metadata with the grid.
    grid->insertMeta("radius", openvdb::FloatMetadata(50.0));
    // Associate a scaling transform with the grid that sets the voxel size
    // to 0.5 units in world space.
    grid->setTransform(
        openvdb::math::Transform::createLinearTransform(/*voxel size=*/0.5));
    // Identify the grid as a level set.
    grid->setGridClass(openvdb::GRID_LEVEL_SET);
    // Name the grid "LevelSetSphere".
    grid->setName("LevelSetSphere");

    return grid;

    // // Create a VDB file object.
    // openvdb::io::File file("mygrids.vdb");
    // // Add the grid pointer to a container.
    // openvdb::GridPtrVec grids;
    // grids.push_back(grid);
    // // Write out the contents of the container.
    // file.write(grids);
    // file.close();
}

// https://www.openvdb.org/documentation/doxygen/codeExamples.html#openvdbPointsHelloWorld
openvdb::points::PointDataGrid::Ptr get_grid_example_0() {
  // Initialize grid types and point attributes types.
  openvdb::initialize();
  // Create a vector with four point positions.
  std::vector<openvdb::Vec3R> positions;
  positions.push_back(openvdb::Vec3R(0, 1, 0));
  positions.push_back(openvdb::Vec3R(1.5, 3.5, 1));
  positions.push_back(openvdb::Vec3R(-1, 6, -2));
  positions.push_back(openvdb::Vec3R(1.1, 1.25, 0.06));
  // The VDB Point-Partioner is used when bucketing points and requires a
  // specific interface. For convenience, we use the PointAttributeVector
  // wrapper around an stl vector wrapper here, however it is also possible to
  // write one for a custom data structure in order to match the interface
  // required.
  openvdb::points::PointAttributeVector<openvdb::Vec3R> positionsWrapper(
      positions);
  // This method computes a voxel-size to match the number of
  // points / voxel requested. Although it won't be exact, it typically offers
  // a good balance of memory against performance.
  int pointsPerVoxel = 8;
  float voxelSize =
      openvdb::points::computeVoxelSize(positionsWrapper, pointsPerVoxel);
  // Print the voxel-size to cout
  std::cout << "VoxelSize=" << voxelSize << std::endl;
  // Create a transform using this voxel-size.
  openvdb::math::Transform::Ptr transform =
      openvdb::math::Transform::createLinearTransform(voxelSize);
  // Create a PointDataGrid containing these four points and using the
  // transform given. This function has two template parameters, (1) the codec
  // to use for storing the position, (2) the grid we want to create
  // (ie a PointDataGrid).
  // We use no compression here for the positions.
  openvdb::points::PointDataGrid::Ptr grid =
      openvdb::points::createPointDataGrid<openvdb::points::NullCodec,
                                           openvdb::points::PointDataGrid>(
          positions, *transform);
  // Set the name of the grid
  grid->setName("Points");
  openvdb::Index64 count = openvdb::points::pointCount(grid->tree());
  std::cout << "PointCount=" << count << std::endl;
  // Iterate over all the leaf nodes in the grid.
  for (auto leafIter = grid->tree().cbeginLeaf(); leafIter; ++leafIter) {
    // Verify the leaf origin.
    std::cout << "Leaf" << leafIter->origin() << std::endl;
    // Extract the position attribute from the leaf by name (P is position).
    const openvdb::points::AttributeArray &array =
        leafIter->constAttributeArray("P");
    // Create a read-only AttributeHandle. Position always uses Vec3f.
    openvdb::points::AttributeHandle<openvdb::Vec3f> positionHandle(array);
    // Iterate over the point indices in the leaf.
    for (auto indexIter = leafIter->beginIndexOn(); indexIter; ++indexIter) {
      // Extract the voxel-space position of the point.
      openvdb::Vec3f voxelPosition = positionHandle.get(*indexIter);
      // Extract the index-space position of the voxel.
      const openvdb::Vec3d xyz = indexIter.getCoord().asVec3d();
      // Compute the world-space position of the point.
      openvdb::Vec3f worldPosition =
          grid->transform().indexToWorld(voxelPosition + xyz);
      // Verify the index and world-space position of the point
      std::cout << "* PointIndex=[" << *indexIter << "] ";
      std::cout << "WorldPosition=" << worldPosition << std::endl;
    }
  }
    return grid;
}

} // namespace VW
#endif 

