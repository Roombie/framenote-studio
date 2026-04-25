#include "tools/EraserTool.h"
#include <cmath>

namespace Framenote {

void EraserTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_lastX = static_cast<int>(e.canvasX);
    m_lastY = static_cast<int>(e.canvasY);
    erasePixel(doc, frameIndex, m_lastX, m_lastY);
    doc.markDirty();
}

void EraserTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);
    eraseLine(doc, frameIndex, m_lastX, m_lastY, x, y);
    m_lastX = x;
    m_lastY = y;
    doc.markDirty();
}

void EraserTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    m_lastX = -1;
    m_lastY = -1;
}

void EraserTool::erasePixel(Document& doc, int frameIndex, int x, int y) {
    doc.frame(frameIndex).setPixel(x, y, 0x00000000); // fully transparent
}

void EraserTool::eraseLine(Document& doc, int frameIndex,
                             int x0, int y0, int x1, int y1) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        erasePixel(doc, frameIndex, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

} // namespace Framenote
