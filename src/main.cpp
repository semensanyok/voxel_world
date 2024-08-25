#include "renderer_context.h"
#include "vw_utils.h"
#include <algorithm>
#include <openvdb/tools/VolumeToMesh.h>
#include <string>

int main(int argc, char *argv[]) {
  auto args_vw = parse_arg(argc, argv);

  auto rc = RendererContext();
  rc.init();

  while (true) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }
    rc.drawFrame();
  }
  rc.clear();
  return 0;
}
