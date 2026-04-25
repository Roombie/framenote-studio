#include "core/Palette.h"

namespace Framenote {

Palette::Palette() {
    loadDefault();
}

void Palette::loadDefault() {
    // 32-color default palette — pixel-art friendly, Nibbit-approved 🐰
    // Row 0: Neutrals + greys
    m_colors[ 0] = {  0,   0,   0,   0};  // transparent (index 0 = eraser)
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

    // Row 4: Blues & cyans (Nibbit blue lives here!)
    m_colors[16] = { 27,  38, 144, 255};
    m_colors[17] = { 44, 140, 213, 255};  // Nibbit blue ✨
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
    m_colors[27] = {255,   0,  77, 255};  // hot pink accent
    m_colors[28] = {  0, 228,  54, 255};  // lime accent
    m_colors[29] = { 41, 173, 255, 255};  // sky accent
    m_colors[30] = {255, 163,   0, 255};  // amber accent
    m_colors[31] = {131,   0, 180, 255};  // violet accent

    m_selectedIndex = 1; // start with black
}

} // namespace Framenote
