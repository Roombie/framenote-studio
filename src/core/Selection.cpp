#include "core/Selection.h"
#include <algorithm>

namespace Framenote {

Selection::Selection(int canvasW, int canvasH) {
    resize(canvasW, canvasH);
}

void Selection::resize(int canvasW, int canvasH) {
    m_width  = canvasW;
    m_height = canvasH;
    m_mask.assign(static_cast<size_t>(canvasW * canvasH), false);
}

void Selection::setRect(int x, int y, int w, int h, bool value) {
    for (int py = y; py < y + h; ++py)
        for (int px = x; px < x + w; ++px)
            if (inBounds(px, py))
                m_mask[static_cast<size_t>(py * m_width + px)] = value;
}

void Selection::addRect(int x, int y, int w, int h) {
    setRect(x, y, w, h, true);
}

void Selection::removeRect(int x, int y, int w, int h) {
    setRect(x, y, w, h, false);
}

void Selection::selectAll() {
    std::fill(m_mask.begin(), m_mask.end(), true);
}

void Selection::clear() {
    std::fill(m_mask.begin(), m_mask.end(), false);
}

bool Selection::isSelected(int x, int y) const {
    if (!inBounds(x, y)) return false;
    return m_mask[static_cast<size_t>(y * m_width + x)];
}

bool Selection::isEmpty() const {
    for (bool b : m_mask)
        if (b) return false;
    return true;
}

SelectionRect Selection::bounds() const {
    int minX = m_width, minY = m_height, maxX = -1, maxY = -1;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (m_mask[static_cast<size_t>(y * m_width + x)]) {
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }
    if (maxX < 0) return {};
    return { minX, minY, maxX - minX + 1, maxY - minY + 1 };
}

} // namespace Framenote
