#include "renderer_context.h"
#include "logging.h"
#include "settings_global.h"
#include <SDL.h>
#include <SDL_vulkan.h>

namespace VW_RendererContext {

SDL_Window *window;

void init() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI // SDL_WINDOW_OPENGL
      );

  window = SDL_CreateWindow("voxel_world", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, GameSettings::SCR_WIDTH,
                            GameSettings::SCR_HEIGHT,
                            window_flags
  );
  if (!window)
    LOG_ERROR("Couldn't create window");

  // hide cursor, report only mouse motion events
  // SDL_SetRelativeMouseMode(SDL_TRUE);
  
  // Gui::InitGuiContext(window, context);
}

void clear() {
  SDL_DestroyWindow(VW_RendererContext::window);
  SDL_Quit();
}

} // namespace VW_RendererContext
