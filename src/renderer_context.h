#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include "SDL.h"

namespace RendererContext {
	SDL_Window* window;
	SDL_GLContext context;
	void init();
	void clear_context();
};

#endif
