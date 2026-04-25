#include "tools/PencilTool.h"
#include <cmath>

namespace Framenote {

void PencilTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
    m_lastX   = static_cast<int>(e.canvasX);
    m_lastY   = static_cast<int>(e.canvasY);
    drawPixel(doc, frameIndex, m_lastX, m_lastY);
    doc.markDirty();
}

void PencilTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing) return;
    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);
    drawLine(doc, frameIndex, m_lastX, m_lastY, x, y);
    m_lastX = x;
    m_lastY = y;
    doc.markDirty();
}

void PencilTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = false;
    m_lastX   = -1;
    m_lastY   = -1;
}

void PencilTool::drawPixel(Document& doc, int frameIndex, int x, int y) {
    auto& frame = doc.frame(frameIndex);
    uint32_t color = doc.palette().selectedColor().toRGBA();
    frame.setPixel(x, y, color);
}

// Bresenham's line algorithm — pixel-perfect, no anti-aliasing
void PencilTool::drawLine(Document& doc, int frameIndex,
                           int x0, int y0, int x1, int y1) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        drawPixel(doc, frameIndex, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

} // namespace Framenote
