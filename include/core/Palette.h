#pragma once

#include <vector>
#include <string>
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

    static Color transparent() { return {0, 0, 0, 0}; }
};

class Palette {
public:
    Palette();

    // Color access
    Color&       color(int index)       { return m_colors[index]; }
    const Color& color(int index) const { return m_colors[index]; }
    int          size()           const { return (int)m_colors.size(); }

    // Selection
    int   selectedIndex() const  { return m_selectedIndex; }
    void  selectIndex(int index) { m_selectedIndex = clamp(index); }
    Color selectedColor() const  { return m_colors[m_selectedIndex]; }

    // Primary / secondary colors.
    // selectedIndex() remains as primary color for backward compatibility.
    int   primaryIndex() const { return m_selectedIndex; }
    int   secondaryIndex() const { return clamp(m_secondaryIndex); }

    void  selectPrimaryIndex(int index) { selectIndex(index); }
    void  selectSecondaryIndex(int index) { m_secondaryIndex = clamp(index); }

    Color primaryColor() const { return selectedColor(); }
    Color secondaryColor() const { return m_colors[clamp(m_secondaryIndex)]; }

    // Add / remove / reorder colors
    void addColor(Color c = {0, 0, 0, 255});
    void removeColor(int index);   // clamps selection if needed
    void moveColor(int fromIndex, int toIndex);

    // Persistence
    void loadDefault();
    bool save(const std::string& path) const;
    bool load(const std::string& path);

private:
    int clamp(int index) const;

    std::vector<Color> m_colors;
    int                m_selectedIndex = 1; // Primary color
    int                m_secondaryIndex = 0; // Secondary color, usually transparent
};

} // namespace Framenote