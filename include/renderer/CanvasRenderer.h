#pragma once

#include <SDL3/SDL.h>
#include "core/Frame.h"

namespace Framenote {

// CanvasRenderer owns the SDL textures for the canvas and onion skin.
// It is created per-tab and lives in DocumentTab.
//
// Pixel format: SDL_PIXELFORMAT_ARGB8888, SDL_SCALEMODE_NEAREST.
// uploadFrame() crops the visible canvas region from the frame buffer
// so that buffer-larger-than-canvas is handled correctly.
class CanvasRenderer {
public:
    CanvasRenderer(SDL_Renderer* renderer, int canvasW, int canvasH);
    ~CanvasRenderer();

    // Non-copyable — owns GPU textures
    CanvasRenderer(const CanvasRenderer&) = delete;
    CanvasRenderer& operator=(const CanvasRenderer&) = delete;

    // Resize canvas (destroys and recreates textures + checkerboard)
    void resize(int newW, int newH);

    // Upload pixel data from frame into the canvas texture.
    // Handles the case where frame.bufferWidth() > canvasW by cropping.
    void uploadFrame(const Frame& frame);

    // Upload a previous frame for onion skin ghost rendering.
    void uploadOnionFrame(const Frame& frame, float opacity);

    SDL_Texture* canvasTexture() const { return m_canvasTex; }
    SDL_Texture* onionTexture()  const { return m_onionTex; }

    // Rebuild the checkerboard texture at the current canvas size.
    // Called automatically on construction and resize.
    void buildCheckerboard(float squareSize);
    SDL_Texture* checkerboardTexture() const { return m_checkerTex; }

    // Convert screen coordinates to canvas pixel coordinates.
    // Returns false if the point is outside the canvas.
    bool screenToCanvas(float sx, float sy,
                        SDL_FPoint canvasPos, SDL_FPoint canvasSize,
                        int& outX, int& outY) const;

    int canvasW() const { return m_canvasW; }
    int canvasH() const { return m_canvasH; }

private:
    void createTextures();
    void destroyTextures();

    SDL_Renderer* m_renderer   = nullptr;
    SDL_Texture*  m_canvasTex  = nullptr;
    SDL_Texture*  m_onionTex   = nullptr;
    SDL_Texture*  m_checkerTex = nullptr; // pre-rendered transparency grid
    int           m_canvasW    = 0;
    int           m_canvasH    = 0;
};

} // namespace Framenote