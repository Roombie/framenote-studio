#pragma once

#include <vector>
#include <cstdint>

namespace Framenote {

class Frame {
public:
    Frame(int width, int height);

    Frame(Frame&&) = default;
    Frame& operator=(Frame&&) = default;

    // Canvas-visible size (may be smaller than buffer)
    int width()  const { return m_width; }
    int height() const { return m_height; }

    // Full buffer size (always >= width/height)
    int bufferWidth()  const { return m_bufferWidth; }
    int bufferHeight() const { return m_bufferHeight; }

    std::vector<uint8_t>&       pixels()       { return m_pixels; }
    const std::vector<uint8_t>& pixels() const { return m_pixels; }

    uint32_t getPixel(int x, int y) const;
    void     setPixel(int x, int y, uint32_t rgba);
    void     clear(uint32_t rgba = 0x00000000);
    Frame    clone() const;

    // Resize visible area without destroying pixel data outside bounds
    void setVisibleSize(int w, int h);

    // Expand buffer to at least newW x newH, preserving all existing pixels
    void expandBuffer(int newW, int newH);

private:
    int                   m_width;        // visible canvas width
    int                   m_height;       // visible canvas height
    int                   m_bufferWidth;  // actual pixel buffer width
    int                   m_bufferHeight; // actual pixel buffer height
    std::vector<uint8_t>  m_pixels;
};

} // namespace Framenote
