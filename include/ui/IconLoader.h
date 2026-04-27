#pragma once
#include <SDL3/SDL.h>

namespace Framenote {

// Holds SDL textures for all tool icons.
// Loaded once at startup from assets/icons/*.png
struct ToolIcons {
    SDL_Texture* pencil     = nullptr;
    SDL_Texture* eraser     = nullptr;
    SDL_Texture* fill       = nullptr;
    SDL_Texture* eyedropper = nullptr;

    bool load(SDL_Renderer* renderer);
    void destroy();

    bool ok() const {
        return pencil && eraser && fill && eyedropper;
    }
};

} // namespace Framenote
