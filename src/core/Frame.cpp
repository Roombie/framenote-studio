#include "core/Frame.h"
#include <cstring>

namespace Framenote {

Frame::Frame(int width, int height)
    : m_width(width), m_height(height)
    , m_bufferWidth(width), m_bufferHeight(height)
{
    m_pixels.assign(static_cast<size_t>(width * height * 4), 0);
}

uint32_t Frame::getPixel(int x, int y) const {
    if (x < 0 || x >= m_bufferWidth || y < 0 || y >= m_bufferHeight) return 0;
    size_t idx = static_cast<size_t>((y * m_bufferWidth + x) * 4);
    // Memory layout is [B, G, R, A] (little-endian ARGB8888)
    // We reconstruct as (A<<24)|(R<<16)|(G<<8)|B
    return (static_cast<uint32_t>(m_pixels[idx + 3]) << 24) |
           (static_cast<uint32_t>(m_pixels[idx + 2]) << 16) |
           (static_cast<uint32_t>(m_pixels[idx + 1]) <<  8) |
           (static_cast<uint32_t>(m_pixels[idx    ]));
}

void Frame::setPixel(int x, int y, uint32_t argb) {
    if (x < 0 || x >= m_bufferWidth || y < 0 || y >= m_bufferHeight) return;
    size_t idx = static_cast<size_t>((y * m_bufferWidth + x) * 4);
    // SDL_PIXELFORMAT_ARGB8888 on little-endian stores bytes as [B, G, R, A]
    m_pixels[idx    ] =  argb        & 0xFF;  // B
    m_pixels[idx + 1] = (argb >>  8) & 0xFF;  // G
    m_pixels[idx + 2] = (argb >> 16) & 0xFF;  // R
    m_pixels[idx + 3] = (argb >> 24) & 0xFF;  // A
}

void Frame::clear(uint32_t argb) {
    if (argb == 0x00000000) {
        // Fast path: transparent black — zero the whole buffer in one call
        std::memset(m_pixels.data(), 0,
                    static_cast<size_t>(m_bufferWidth * m_bufferHeight * 4));
        return;
    }
    // General path: decompose color and fill each pixel
    // (std::fill on uint32_t would misalign on some platforms so we write bytes)
    uint8_t b =  argb        & 0xFF;
    uint8_t g = (argb >>  8) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t a = (argb >> 24) & 0xFF;
    size_t total = static_cast<size_t>(m_bufferWidth * m_bufferHeight);
    for (size_t i = 0; i < total; ++i) {
        m_pixels[i * 4    ] = b;
        m_pixels[i * 4 + 1] = g;
        m_pixels[i * 4 + 2] = r;
        m_pixels[i * 4 + 3] = a;
    }
}

void Frame::setVisibleSize(int w, int h) {
    // Only updates visible bounds — buffer data is untouched
    m_width  = w;
    m_height = h;
}

void Frame::expandBuffer(int newW, int newH) {
    if (newW <= m_bufferWidth && newH <= m_bufferHeight) return;

    int finalW = newW > m_bufferWidth  ? newW : m_bufferWidth;
    int finalH = newH > m_bufferHeight ? newH : m_bufferHeight;

    // Allocate new zeroed buffer
    std::vector<uint8_t> newPixels(
        static_cast<size_t>(finalW * finalH * 4), 0);

    // Copy existing rows into new buffer (old width may differ from new)
    for (int y = 0; y < m_bufferHeight; ++y) {
        size_t srcOffset  = static_cast<size_t>(y * m_bufferWidth  * 4);
        size_t destOffset = static_cast<size_t>(y * finalW * 4);
        std::memcpy(newPixels.data() + destOffset,
                    m_pixels.data()  + srcOffset,
                    static_cast<size_t>(m_bufferWidth * 4));
    }

    m_pixels       = std::move(newPixels);
    m_bufferWidth  = finalW;
    m_bufferHeight = finalH;
}

Frame Frame::clone() const {
    Frame copy(m_bufferWidth, m_bufferHeight);
    copy.m_pixels       = m_pixels;
    copy.m_width        = m_width;
    copy.m_height       = m_height;
    copy.m_bufferWidth  = m_bufferWidth;
    copy.m_bufferHeight = m_bufferHeight;
    return copy;
}

} // namespace Framenote
