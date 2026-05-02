#include "tools/LineTool.h"

#include "tools/SelectionPixelClip.h"
#include "tools/ShapeRasterizer.h"

#include <cstdint>

namespace Framenote {

void LineTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;

    m_startX = static_cast<int>(e.canvasX);
    m_startY = static_cast<int>(e.canvasY);
    m_endX = m_startX;
    m_endY = m_startY;

    m_color = (e.rightDown && !e.leftDown)
        ? doc.palette().secondaryColor().toRGBA()
        : doc.palette().primaryColor().toRGBA();

    (void)frameIndex;
}

void LineTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);

    (void)doc;
    (void)frameIndex;
}

void LineTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);
    m_drawing = false;

    drawLine(
        doc,
        frameIndex,
        m_startX,
        m_startY,
        m_endX,
        m_endY,
        e.brushSize,
        m_color,
        e.selection
    );

    doc.markDirty();
}

void LineTool::drawBrush(
    Document& doc,
    int frameIndex,
    int x,
    int y,
    int brushSize,
    uint32_t color,
    const Selection* selection
) {
    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    int half = brushSize / 2;

    for (int dy = -half; dy < brushSize - half; ++dy) {
        for (int dx = -half; dx < brushSize - half; ++dx) {
            int px = x + dx;
            int py = y + dy;

            if (px < 0 || py < 0 || px >= cw || py >= ch)
                continue;

            if (!SelectionPixelClip::canModifyPixel(selection, px, py))
                continue;

            frame.setPixel(px, py, color);
        }
    }
}

void LineTool::drawLine(
    Document& doc,
    int frameIndex,
    int x0,
    int y0,
    int x1,
    int y1,
    int brushSize,
    uint32_t color,
    const Selection* selection
) {
    ShapeRasterizer::rasterizeLine(
        x0,
        y0,
        x1,
        y1,
        [&](int x, int y) {
            drawBrush(doc, frameIndex, x, y, brushSize, color, selection);
        }
    );
}

} // namespace Framenote