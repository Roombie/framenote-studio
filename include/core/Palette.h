#pragma once

#include <array>
#include <cstdint>

namespace Framenote {

// RGBA color (8 bits per channel)
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    uint32_t toRGBA() const {
        return (static_cast<uint32_t>(a) << 24) |
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) <<  8) |
            (static_cast<uint32_t>(b));
    }

    static Color fromRGBA(uint32_t rgba) {
        return {
            static_cast<uint8_t>((rgba >> 24) & 0xFF),
            static_cast<uint8_t>((rgba >> 16) & 0xFF),
            static_cast<uint8_t>((rgba >>  8) & 0xFF),
            static_cast<uint8_t>( rgba        & 0xFF)
        };
    }

    // Transparent sentinel
    static Color transparent() { return {0, 0, 0, 0}; }
};

constexpr int PALETTE_SIZE = 32;

// Fixed 32-color palette â€” Flipnote-inspired simplicity for v0.1
class Palette {
public:
    Palette();  // Initializes with a default pixel-art friendly palette

    Color&       color(int index)       { return m_colors[index]; }
    const Color& color(int index) const { return m_colors[index]; }

    int  selectedIndex() const       { return m_selectedIndex; }
    void selectIndex(int index)      { m_selectedIndex = index; }
    Color selectedColor() const      { return m_colors[m_selectedIndex]; }

    static constexpr int size() { return PALETTE_SIZE; }

    void loadDefault();   // Reset to built-in palette

private:
    std::array<Color, PALETTE_SIZE> m_colors;
    int                             m_selectedIndex = 1; // default: black
};

} // namespace Framenote



