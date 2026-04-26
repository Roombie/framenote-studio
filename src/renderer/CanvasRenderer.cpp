#include "renderer/CanvasRenderer.h"
#include <vector>
#include <cstring>

namespace Framenote {

CanvasRenderer::CanvasRenderer(SDL_Renderer* renderer, int canvasW, int canvasH)
    : m_renderer(renderer), m_canvasW(canvasW), m_canvasH(canvasH)
{
    createTextures();
    buildCheckerboard(2.0f); // 2 canvas pixels per checker square
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
    if (m_canvasTex)  { SDL_DestroyTexture(m_canvasTex);  m_canvasTex  = nullptr; }
    if (m_onionTex)   { SDL_DestroyTexture(m_onionTex);   m_onionTex   = nullptr; }
    if (m_checkerTex) { SDL_DestroyTexture(m_checkerTex); m_checkerTex = nullptr; }
}

void CanvasRenderer::resize(int newW, int newH) {
    m_canvasW = newW;
    m_canvasH = newH;
    destroyTextures();
    createTextures();
    buildCheckerboard(2.0f);
}

void CanvasRenderer::buildCheckerboard(float squareSizeCanvasPx) {
    // Pre-render the transparency grid into a static texture.
    // squareSizeCanvasPx = checker square size in canvas pixels.
    // We build it at canvas resolution and let zoom handle scaling.
    int sq = (int)squareSizeCanvasPx;
    if (sq < 1) sq = 1;

    // ARGB8888 pixel buffer for the checkerboard
    std::vector<uint32_t> pixels(
        static_cast<size_t>(m_canvasW * m_canvasH));

    const uint32_t light = 0xFFCCCCCC; // light grey
    const uint32_t dark  = 0xFF999999; // dark grey

    for (int y = 0; y < m_canvasH; ++y) {
        for (int x = 0; x < m_canvasW; ++x) {
            bool even = ((x / sq) + (y / sq)) % 2 == 0;
            pixels[y * m_canvasW + x] = even ? light : dark;
        }
    }

    if (m_checkerTex) SDL_DestroyTexture(m_checkerTex);
    m_checkerTex = SDL_CreateTexture(m_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        m_canvasW, m_canvasH);
    SDL_SetTextureScaleMode(m_checkerTex, SDL_SCALEMODE_NEAREST);
    SDL_UpdateTexture(m_checkerTex, nullptr,
                      pixels.data(), m_canvasW * sizeof(uint32_t));
}

void CanvasRenderer::uploadFrame(const Frame& frame) {
    if (frame.bufferWidth() == m_canvasW && frame.bufferHeight() == m_canvasH) {
        // Fast path: buffer matches canvas exactly
        SDL_UpdateTexture(m_canvasTex, nullptr,
                          frame.pixels().data(), m_canvasW * 4);
        return;
    }

    // Crop path: buffer is larger than canvas — copy only visible rows
    std::vector<uint8_t> visible(
        static_cast<size_t>(m_canvasW * m_canvasH * 4), 0);
    const auto& src = frame.pixels();
    int bufW  = frame.bufferWidth();
    int copyW = m_canvasW < bufW                ? m_canvasW                : bufW;
    int copyH = m_canvasH < frame.bufferHeight() ? m_canvasH : frame.bufferHeight();

    for (int y = 0; y < copyH; ++y) {
        std::memcpy(visible.data()   + y * m_canvasW * 4,
                    src.data()       + y * bufW * 4,
                    static_cast<size_t>(copyW * 4));
    }

    SDL_UpdateTexture(m_canvasTex, nullptr, visible.data(), m_canvasW * 4);
}

void CanvasRenderer::uploadOnionFrame(const Frame& frame, float opacity) {
    if (frame.bufferWidth() == m_canvasW && frame.bufferHeight() == m_canvasH) {
        SDL_UpdateTexture(m_onionTex, nullptr,
                          frame.pixels().data(), m_canvasW * 4);
    } else {
        std::vector<uint8_t> visible(
            static_cast<size_t>(m_canvasW * m_canvasH * 4), 0);
        const auto& src = frame.pixels();
        int bufW  = frame.bufferWidth();
        int copyW = m_canvasW < bufW                 ? m_canvasW                 : bufW;
        int copyH = m_canvasH < frame.bufferHeight() ? m_canvasH : frame.bufferHeight();
        for (int y = 0; y < copyH; ++y) {
            std::memcpy(visible.data() + y * m_canvasW * 4,
                        src.data()     + y * bufW * 4,
                        static_cast<size_t>(copyW * 4));
        }
        SDL_UpdateTexture(m_onionTex, nullptr, visible.data(), m_canvasW * 4);
    }
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
