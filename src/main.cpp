#include "grids.h"
#include "renderer_context.h"
#include <openvdb/tools/VolumeToMesh.h>

int main(int argc, char *argv[]) {
  auto gr = VW::NoiseGrid::createNoiseGrid();

  auto rc = RendererContext();
  rc.init();

  while (true) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }
  }
  rc.clear();

  return 0;
}
