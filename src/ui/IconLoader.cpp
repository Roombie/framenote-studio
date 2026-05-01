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
    // Drawing tools
    pencil     = loadPNG(renderer, "assets/icons/pencil_icon.png");
    eraser     = loadPNG(renderer, "assets/icons/eraser_icon.png");
    fill       = loadPNG(renderer, "assets/icons/fill_icon.png");
    eyedropper = loadPNG(renderer, "assets/icons/eyedropper_icon.png");
    line       = loadPNG(renderer, "assets/icons/line_icon.png");
    rectangle  = loadPNG(renderer, "assets/icons/rectangle_icon.png");
    ellipse    = loadPNG(renderer, "assets/icons/ellipse_icon.png");
    select     = loadPNG(renderer, "assets/icons/select_icon.png");
    move       = loadPNG(renderer, "assets/icons/move_icon.png");

    // Timeline controls
    firstFrame = loadPNG(renderer, "assets/icons/first_frame_icon.png");
    prevFrame  = loadPNG(renderer, "assets/icons/prev_frame_icon.png");
    play       = loadPNG(renderer, "assets/icons/play_icon.png");
    pause      = loadPNG(renderer, "assets/icons/pause_icon.png");
    nextFrame  = loadPNG(renderer, "assets/icons/next_frame_icon.png");
    lastFrame  = loadPNG(renderer, "assets/icons/last_frame_icon.png");

    return ok();
}

void ToolIcons::destroy() {
    auto destroyTex = [](SDL_Texture*& t) {
        if (t) { SDL_DestroyTexture(t); t = nullptr; }
    };
    destroyTex(pencil);
    destroyTex(eraser);
    destroyTex(fill);
    destroyTex(eyedropper);
    destroyTex(line);
    destroyTex(rectangle);
    destroyTex(ellipse);
    destroyTex(select);
    destroyTex(move);
    destroyTex(firstFrame);
    destroyTex(prevFrame);
    destroyTex(play);
    destroyTex(pause);
    destroyTex(nextFrame);
    destroyTex(lastFrame);
}

} // namespace Framenote