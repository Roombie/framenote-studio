#include "core/Palette.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

namespace Framenote {

Palette::Palette() {
    loadDefault();
}

void Palette::addColor(Color c) {
    m_colors.push_back(c);
}

void Palette::moveColor(int fromIndex, int toIndex) {
    if (m_colors.empty())
        return;

    fromIndex = clamp(fromIndex);
    toIndex   = clamp(toIndex);

    if (fromIndex == toIndex)
        return;

    Color moved = m_colors[fromIndex];

    m_colors.erase(m_colors.begin() + fromIndex);
    m_colors.insert(m_colors.begin() + toIndex, moved);

    auto remapIndex = [fromIndex, toIndex](int idx) {
        if (idx == fromIndex)
            return toIndex;

        if (fromIndex < toIndex) {
            if (idx > fromIndex && idx <= toIndex)
                return idx - 1;
        }
        else {
            if (idx >= toIndex && idx < fromIndex)
                return idx + 1;
        }

        return idx;
    };

    m_selectedIndex  = clamp(remapIndex(m_selectedIndex));
    m_secondaryIndex = clamp(remapIndex(m_secondaryIndex));
}

void Palette::removeColor(int index) {
    if (m_colors.size() <= 1) return;  // always keep at least one
    m_colors.erase(m_colors.begin() + index);
    m_selectedIndex = clamp(m_selectedIndex);
    m_secondaryIndex = clamp(m_secondaryIndex);
}

void Palette::setColors(std::vector<Color> colors) {
    if (colors.empty()) {
        colors.push_back({0, 0, 0, 0});
    }

    m_colors = std::move(colors);

    m_selectedIndex = clamp(m_selectedIndex);
    m_secondaryIndex = clamp(m_secondaryIndex);
}

int Palette::clamp(int index) const {
    if (index < 0) return 0;
    if (index >= (int)m_colors.size()) return (int)m_colors.size() - 1;
    return index;
}

bool Palette::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "FRAMENOTE_PAL\n";
    for (const auto& c : m_colors)
        f << (int)c.r << " " << (int)c.g << " "
          << (int)c.b << " " << (int)c.a << "\n";
    return true;
}

bool Palette::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string header;
    std::getline(f, header);
    if (header != "FRAMENOTE_PAL") return false;

    std::vector<Color> loaded;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        int r, g, b, a;
        if (ss >> r >> g >> b >> a)
            loaded.push_back({
                (uint8_t)r, (uint8_t)g,
                (uint8_t)b, (uint8_t)a });
    }

    if (loaded.empty()) return false;
    m_colors = std::move(loaded);
    m_selectedIndex = clamp(m_selectedIndex);
    m_secondaryIndex = clamp(m_secondaryIndex);
    return true;
}

void Palette::loadDefault() {
    m_colors.clear();
    m_colors.resize(32);

    // Row 0: Neutrals + greys
    m_colors[ 0] = {  0,   0,   0,   0};  // transparent
    m_colors[ 1] = {  0,   0,   0, 255};  // black
    m_colors[ 2] = { 34,  34,  34, 255};  // dark grey
    m_colors[ 3] = { 85,  85,  85, 255};  // mid grey
    m_colors[ 4] = {160, 160, 160, 255};  // light grey
    m_colors[ 5] = {220, 220, 220, 255};  // near white
    m_colors[ 6] = {255, 255, 255, 255};  // white

    // Row 1: Reds & pinks
    m_colors[ 7] = {190,  38,  51, 255};
    m_colors[ 8] = {224, 111, 139, 255};
    m_colors[ 9] = {255, 180, 180, 255};

    // Row 2: Oranges & yellows
    m_colors[10] = {235, 137,  49, 255};
    m_colors[11] = {247, 226, 107, 255};
    m_colors[12] = {255, 243, 153, 255};

    // Row 3: Greens
    m_colors[13] = { 47, 144,  68, 255};
    m_colors[14] = { 68, 200,  71, 255};
    m_colors[15] = {163, 222, 128, 255};

    // Row 4: Blues & cyans
    m_colors[16] = { 27,  38, 144, 255};
    m_colors[17] = { 44, 140, 213, 255};  // Nibbit blue
    m_colors[18] = {  0, 220, 255, 255};
    m_colors[19] = {148, 220, 255, 255};

    // Row 5: Purples & magentas
    m_colors[20] = {118,  66, 138, 255};
    m_colors[21] = {188,  74, 155, 255};
    m_colors[22] = {230, 160, 220, 255};

    // Row 6: Browns & skin tones
    m_colors[23] = { 89,  43,   0, 255};
    m_colors[24] = {157,  97,  36, 255};
    m_colors[25] = {234, 179, 105, 255};
    m_colors[26] = {255, 218, 168, 255};

    // Row 7: Extra accents
    m_colors[27] = {255,   0,  77, 255};
    m_colors[28] = {  0, 228,  54, 255};
    m_colors[29] = { 41, 173, 255, 255};
    m_colors[30] = {255, 163,   0, 255};
    m_colors[31] = {131,   0, 180, 255};

    m_selectedIndex = 1;   // Primary: black
    m_secondaryIndex = 0;  // Secondary: transparent
}

} // namespace Framenote