#include "renderer/CanvasRenderer.h"

namespace Framenote {

CanvasRenderer::CanvasRenderer(SDL_Renderer* renderer, int canvasW, int canvasH)
    : m_renderer(renderer), m_canvasW(canvasW), m_canvasH(canvasH)
{
    createTextures();
}

CanvasRenderer::~CanvasRenderer() {
    destroyTextures();
}

void CanvasRenderer::createTextures() {
    m_canvasTex = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        m_canvasW, m_canvasH);
    SDL_SetTextureBlendMode(m_canvasTex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(m_canvasTex, SDL_SCALEMODE_NEAREST);

    m_onionTex = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        m_canvasW, m_canvasH);
    SDL_SetTextureBlendMode(m_onionTex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(m_onionTex, SDL_SCALEMODE_NEAREST);
}

void CanvasRenderer::destroyTextures() {
    if (m_canvasTex) { SDL_DestroyTexture(m_canvasTex); m_canvasTex = nullptr; }
    if (m_onionTex)  { SDL_DestroyTexture(m_onionTex);  m_onionTex  = nullptr; }
}

void CanvasRenderer::resize(int newW, int newH) {
    m_canvasW = newW;
    m_canvasH = newH;
    destroyTextures();
    createTextures();
}

void CanvasRenderer::uploadFrame(const Frame& frame) {
    const auto& src = frame.pixels();
    // Use buffer width as pitch — buffer may be larger than visible canvas
    SDL_UpdateTexture(m_canvasTex, nullptr, src.data(), frame.bufferWidth() * 4);
}

void CanvasRenderer::uploadOnionFrame(const Frame& frame, float opacity) {
    const auto& src = frame.pixels();
    SDL_UpdateTexture(m_onionTex, nullptr, src.data(), frame.bufferWidth() * 4);
    SDL_SetTextureAlphaMod(m_onionTex, static_cast<uint8_t>(opacity * 255.0f));
    SDL_SetTextureColorMod(m_onionTex, 100, 180, 255);
}

bool CanvasRenderer::screenToCanvas(float sx, float sy,
                                    SDL_FPoint canvasPos, SDL_FPoint canvasSize,
                                    int& outX, int& outY) const {
    float relX = (sx - canvasPos.x) / canvasSize.x;
    float relY = (sy - canvasPos.y) / canvasSize.y;
    if (relX < 0.0f || relX >= 1.0f || relY < 0.0f || relY >= 1.0f)
        return false;
    outX = static_cast<int>(relX * m_canvasW);
    outY = static_cast<int>(relY * m_canvasH);
    return true;
}

} // namespace Framenote
