#include "renderer_context.h"
#include "vw_utils.h"
#include <SDL.h>
#include <algorithm>
#include <cstddef>
#include <openvdb/tools/VolumeToMesh.h>
#include <string>

RendererContext *rc;

int resizeWindowCallback(void *userdata, SDL_Event *event) {
  if (event->type == SDL_WINDOWEVENT) {
    rc->windowCallback(event);
  }
  // RendererContext *rc = reinterpret_cast<RendererContext *>(userdata);
  // Do things with userdata and SDL_Event
  return 0; // Value will be ignored
}

int main(int argc, char *argv[]) {
  auto args_vw = parse_arg(argc, argv);

  rc = new RendererContext();
  rc->init();

  SDL_AddEventWatch(resizeWindowCallback, // &rc
                    nullptr);
  while (true) {
    SDL_Event e;
    if (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        break;
      }
    }
    rc->drawFrame();
  }
  rc->clear();
  return 0;
}
