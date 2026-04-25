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
    return (static_cast<uint32_t>(m_pixels[idx + 3]) << 24) |
           (static_cast<uint32_t>(m_pixels[idx + 2]) << 16) |
           (static_cast<uint32_t>(m_pixels[idx + 1]) <<  8) |
           (static_cast<uint32_t>(m_pixels[idx    ]));
}

void Frame::setPixel(int x, int y, uint32_t rgba) {
    if (x < 0 || x >= m_bufferWidth || y < 0 || y >= m_bufferHeight) return;
    size_t idx = static_cast<size_t>((y * m_bufferWidth + x) * 4);
    // Write as little-endian ARGB8888: bytes in memory = [B, G, R, A]
    m_pixels[idx    ] =  rgba        & 0xFF;  // B
    m_pixels[idx + 1] = (rgba >>  8) & 0xFF;  // G
    m_pixels[idx + 2] = (rgba >> 16) & 0xFF;  // R
    m_pixels[idx + 3] = (rgba >> 24) & 0xFF;  // A
}

void Frame::clear(uint32_t rgba) {
    for (int y = 0; y < m_bufferHeight; ++y)
        for (int x = 0; x < m_bufferWidth; ++x)
            setPixel(x, y, rgba);
}

void Frame::setVisibleSize(int w, int h) {
    // Just update visible bounds — buffer data is untouched
    m_width  = w;
    m_height = h;
}

void Frame::expandBuffer(int newW, int newH) {
    if (newW <= m_bufferWidth && newH <= m_bufferHeight) return;

    int finalW = newW > m_bufferWidth  ? newW : m_bufferWidth;
    int finalH = newH > m_bufferHeight ? newH : m_bufferHeight;

    // Create new zeroed buffer
    std::vector<uint8_t> newPixels(
        static_cast<size_t>(finalW * finalH * 4), 0);

    // Copy old pixel rows into new buffer
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
