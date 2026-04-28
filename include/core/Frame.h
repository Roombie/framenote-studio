#pragma once

#include <vector>
#include <cstdint>

namespace Framenote {

// ── Pixel format note ─────────────────────────────────────────────────────────
// All Frame pixel data uses SDL_PIXELFORMAT_ARGB8888 on little-endian x86/x64.
// This means bytes in memory are stored as [B, G, R, A] at each pixel offset.
//
// When we talk about a "color value" (uint32_t), the bit layout is:
//   bits 31-24: A   bits 23-16: R   bits 15-8: G   bits 7-0: B
// So toRGBA() / getPixel() return (A<<24)|(R<<16)|(G<<8)|B.
// And setPixel() decomposes that back to [B, G, R, A] bytes in memory.
//
// This matches SDL_PIXELFORMAT_ARGB8888 exactly and what SDL_UpdateTexture expects.
// ──────────────────────────────────────────────────────────────────────────────

class Frame {
public:
    Frame(int width, int height);

    Frame(Frame&&) = default;
    Frame& operator=(Frame&&) = default;

    // Canvas-visible size (may be smaller than buffer after resize-down)
    int width()  const { return m_width; }
    int height() const { return m_height; }

    // Full buffer size — always >= visible size
    // Buffer may be larger when canvas was previously resized to a bigger size
    int bufferWidth()  const { return m_bufferWidth; }
    int bufferHeight() const { return m_bufferHeight; }

    std::vector<uint8_t>&       pixels()       { return m_pixels; }
    const std::vector<uint8_t>& pixels() const { return m_pixels; }

    uint32_t getPixel(int x, int y) const;
    void     setPixel(int x, int y, uint32_t argb);

    // Clear to color (transparent black by default). Uses memset for the fast path.
    void  clear(uint32_t argb = 0x00000000);

    Frame clone() const;

    // Update visible bounds without touching pixel data
    void setVisibleSize(int w, int h);

    // Grow buffer to at least newW×newH, preserving all existing pixels.
    // Pixels outside the old buffer area are zeroed (transparent).
    void expandBuffer(int newW, int newH);

    // Fully replace pixel data and buffer dimensions — used by undo/redo
    // to restore a frame to an exact previous state including its buffer size.
    void restoreBuffer(std::vector<uint8_t> pixels, int bufW, int bufH);

private:
    int                  m_width;         // visible canvas width
    int                  m_height;        // visible canvas height
    int                  m_bufferWidth;   // actual pixel buffer width
    int                  m_bufferHeight;  // actual pixel buffer height
    std::vector<uint8_t> m_pixels;        // raw BGRA bytes, bufferWidth*bufferHeight*4
};

} // namespace Framenote