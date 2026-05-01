#pragma once
#include <SDL3/SDL.h>

namespace Framenote {

// Holds SDL textures for all tool and timeline icons.
// Loaded once at startup from assets/icons/*.png
struct ToolIcons {
    // Drawing tools
    SDL_Texture* pencil     = nullptr;
    SDL_Texture* eraser     = nullptr;
    SDL_Texture* fill       = nullptr;
    SDL_Texture* eyedropper = nullptr;
    SDL_Texture* line       = nullptr;
    SDL_Texture* rectangle  = nullptr;
    SDL_Texture* ellipse    = nullptr;
    SDL_Texture* select     = nullptr;
    SDL_Texture* move       = nullptr;

    // Timeline controls
    SDL_Texture* firstFrame = nullptr;
    SDL_Texture* prevFrame  = nullptr;
    SDL_Texture* play       = nullptr;
    SDL_Texture* pause      = nullptr;
    SDL_Texture* nextFrame  = nullptr;
    SDL_Texture* lastFrame  = nullptr;

    bool load(SDL_Renderer* renderer);
    void destroy();

    // Returns true if at minimum the drawing tools loaded
    bool ok() const {
        return pencil && eraser && fill && eyedropper;
    }
};

} // namespace Framenote