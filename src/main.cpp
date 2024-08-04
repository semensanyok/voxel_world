#include "logging.h"
#include "grids.h"
#include <openvdb/tools/VolumeToMesh.h>
#include "renderer_context.h"

int main(int argc, char *argv[]) {
  auto gr = VW::NoiseGrid::createNoiseGrid();

  VW_RendererContext::init();

  while (true) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }
  }

  return 0;
}
