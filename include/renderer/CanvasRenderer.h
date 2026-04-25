#pragma once

#include <SDL3/SDL.h>
#include "core/Document.h"
#include "core/Timeline.h"

namespace Framenote {

class CanvasRenderer {
public:
    CanvasRenderer(SDL_Renderer* renderer, int canvasW, int canvasH);
    ~CanvasRenderer();

    void uploadFrame(const Frame& frame);
    void uploadOnionFrame(const Frame& frame, float opacity);

    // Recreate textures at a new size (called after canvas resize)
    void resize(int newW, int newH);

    bool screenToCanvas(float sx, float sy,
                        SDL_FPoint canvasPos, SDL_FPoint canvasSize,
                        int& outX, int& outY) const;

    SDL_Texture* canvasTexture() const { return m_canvasTex; }

private:
    void createTextures();
    void destroyTextures();

    SDL_Renderer* m_renderer  = nullptr;
    SDL_Texture*  m_canvasTex = nullptr;
    SDL_Texture*  m_onionTex  = nullptr;
    int           m_canvasW   = 0;
    int           m_canvasH   = 0;
};

} // namespace Framenote
