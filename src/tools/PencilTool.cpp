#include "tools/PencilTool.h"
#include <cmath>

namespace Framenote {

void PencilTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
    m_lastX   = static_cast<int>(e.canvasX);
    m_lastY   = static_cast<int>(e.canvasY);

    m_drawColor =
        (e.rightDown && !e.leftDown)
            ? doc.palette().secondaryColor().toRGBA()
            : doc.palette().primaryColor().toRGBA();

    drawBrush(doc, frameIndex, m_lastX, m_lastY, e.brushSize);
    doc.markDirty();
}

void PencilTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing) return;

    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);

    drawLine(doc, frameIndex, m_lastX, m_lastY, x, y, e.brushSize);

    m_lastX = x;
    m_lastY = y;

    doc.markDirty();
}

void PencilTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    (void)doc;
    (void)frameIndex;
    (void)e;

    m_drawing = false;
    m_lastX   = -1;
    m_lastY   = -1;
}

void PencilTool::drawBrush(Document& doc, int frameIndex, int x, int y, int size) {
    auto& frame = doc.frame(frameIndex);

    int half = size / 2;

    for (int dy = -half; dy < size - half; ++dy) {
        for (int dx = -half; dx < size - half; ++dx) {
            frame.setPixel(x + dx, y + dy, m_drawColor);
        }
    }
}

void PencilTool::drawLine(Document& doc, int frameIndex,
                          int x0, int y0, int x1, int y1, int size) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        drawBrush(doc, frameIndex, x0, y0, size);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

} // namespace Framenote