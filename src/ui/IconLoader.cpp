#include "ui/IconLoader.h"
#include <stb_image.h>

namespace Framenote {

static SDL_Texture* loadPNG(SDL_Renderer* renderer, const char* path) {
    int w, h, channels;
    unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) return nullptr;

    SDL_Texture* tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        w, h);

    if (tex) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        SDL_UpdateTexture(tex, nullptr, data, w * 4);
    }

    stbi_image_free(data);
    return tex;
}

bool ToolIcons::load(SDL_Renderer* renderer) {
    pencil     = loadPNG(renderer, "assets/icons/pencil_icon.png");
    eraser     = loadPNG(renderer, "assets/icons/eraser_icon.png");
    fill       = loadPNG(renderer, "assets/icons/fill_icon.png");
    eyedropper = loadPNG(renderer, "assets/icons/eyedropper_icon.png");
    return ok();
}

void ToolIcons::destroy() {
    if (pencil)     { SDL_DestroyTexture(pencil);     pencil     = nullptr; }
    if (eraser)     { SDL_DestroyTexture(eraser);     eraser     = nullptr; }
    if (fill)       { SDL_DestroyTexture(fill);       fill       = nullptr; }
    if (eyedropper) { SDL_DestroyTexture(eyedropper); eyedropper = nullptr; }
}

} // namespace Framenote