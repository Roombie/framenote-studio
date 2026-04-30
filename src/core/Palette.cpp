#include "core/Palette.h"

#include <vector>
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
    moveColors(std::vector<int>{fromIndex}, toIndex);
}

void Palette::removeColor(int index) {
    removeColors(std::vector<int>{index});
}

static std::vector<int> normalizeIndices(std::vector<int> indices, int size) {
    indices.erase(
        std::remove_if(
            indices.begin(),
            indices.end(),
            [size](int idx) {
                return idx < 0 || idx >= size;
            }
        ),
        indices.end()
    );

    std::sort(indices.begin(), indices.end());

    indices.erase(
        std::unique(indices.begin(), indices.end()),
        indices.end()
    );

    return indices;
}

std::vector<int> Palette::moveColors(const std::vector<int>& indices, int toIndex) {
    if (m_colors.empty())
        return {};

    std::vector<int> selected = normalizeIndices(indices, size());

    if (selected.empty())
        return {};

    toIndex = clamp(toIndex);

    if (std::find(selected.begin(), selected.end(), toIndex) != selected.end()) {
        return selected;
    }

    std::vector<Color> oldColors = m_colors;
    std::vector<int> oldToNew(oldColors.size(), -1);

    std::vector<Color> moving;
    moving.reserve(selected.size());

    for (int idx : selected) {
        moving.push_back(oldColors[idx]);
    }

    int removedBeforeTarget = 0;

    for (int idx : selected) {
        if (idx < toIndex)
            removedBeforeTarget++;
    }

    int insertAt = toIndex - removedBeforeTarget;
    int remainingCount = static_cast<int>(oldColors.size() - selected.size());

    if (insertAt < 0)
        insertAt = 0;

    if (insertAt > remainingCount)
        insertAt = remainingCount;

    std::vector<Color> newColors;
    newColors.reserve(oldColors.size());

    std::vector<int> movedNewIndices;
    movedNewIndices.reserve(selected.size());

    bool insertedMoving = false;
    int keptSeen = 0;

    auto appendMoving = [&]() {
        if (insertedMoving)
            return;

        insertedMoving = true;

        for (int i = 0; i < static_cast<int>(selected.size()); ++i) {
            int oldIdx = selected[i];

            oldToNew[oldIdx] = static_cast<int>(newColors.size());
            movedNewIndices.push_back(oldToNew[oldIdx]);

            newColors.push_back(oldColors[oldIdx]);
        }
    };

    for (int oldIdx = 0; oldIdx < static_cast<int>(oldColors.size()); ++oldIdx) {
        bool isMoving =
            std::binary_search(selected.begin(), selected.end(), oldIdx);

        if (isMoving)
            continue;

        if (keptSeen == insertAt) {
            appendMoving();
        }

        oldToNew[oldIdx] = static_cast<int>(newColors.size());
        newColors.push_back(oldColors[oldIdx]);
        keptSeen++;
    }

    appendMoving();

    int oldPrimary = m_selectedIndex;
    int oldSecondary = m_secondaryIndex;

    m_colors = std::move(newColors);

    if (oldPrimary >= 0 &&
        oldPrimary < static_cast<int>(oldToNew.size()) &&
        oldToNew[oldPrimary] >= 0) {
        m_selectedIndex = clamp(oldToNew[oldPrimary]);
    }
    else {
        m_selectedIndex = clamp(m_selectedIndex);
    }

    if (oldSecondary >= 0 &&
        oldSecondary < static_cast<int>(oldToNew.size()) &&
        oldToNew[oldSecondary] >= 0) {
        m_secondaryIndex = clamp(oldToNew[oldSecondary]);
    }
    else {
        m_secondaryIndex = clamp(m_secondaryIndex);
    }

    return movedNewIndices;
}

void Palette::removeColors(const std::vector<int>& indices) {
    if (m_colors.size() <= 1)
        return;

    std::vector<int> selected = normalizeIndices(indices, size());

    if (selected.empty())
        return;

    while (selected.size() >= m_colors.size()) {
        selected.pop_back();
    }

    if (selected.empty())
        return;

    std::vector<Color> oldColors = m_colors;
    std::vector<int> oldToNew(oldColors.size(), -1);

    std::vector<Color> newColors;
    newColors.reserve(oldColors.size() - selected.size());

    for (int oldIdx = 0; oldIdx < static_cast<int>(oldColors.size()); ++oldIdx) {
        bool remove =
            std::binary_search(selected.begin(), selected.end(), oldIdx);

        if (remove)
            continue;

        oldToNew[oldIdx] = static_cast<int>(newColors.size());
        newColors.push_back(oldColors[oldIdx]);
    }

    if (newColors.empty()) {
        newColors.push_back({0, 0, 0, 0});
    }

    int oldPrimary = m_selectedIndex;
    int oldSecondary = m_secondaryIndex;

    m_colors = std::move(newColors);

    auto remapOrClamp = [&](int oldIndex) {
        if (oldIndex >= 0 &&
            oldIndex < static_cast<int>(oldToNew.size()) &&
            oldToNew[oldIndex] >= 0) {
            return clamp(oldToNew[oldIndex]);
        }

        return clamp(oldIndex);
    };

    m_selectedIndex = remapOrClamp(oldPrimary);
    m_secondaryIndex = remapOrClamp(oldSecondary);
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