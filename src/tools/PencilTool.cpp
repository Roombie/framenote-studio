#include "tools/PencilTool.h"

#include "tools/SelectionPixelClip.h"
#include "tools/ShapeRasterizer.h"

#include <cstdint>

namespace Framenote {

void PencilTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
    m_lastX = static_cast<int>(e.canvasX);
    m_lastY = static_cast<int>(e.canvasY);

    m_drawColor =
        (e.rightDown && !e.leftDown)
            ? doc.palette().secondaryColor().toRGBA()
            : doc.palette().primaryColor().toRGBA();

    drawBrush(doc, frameIndex, m_lastX, m_lastY, e.brushSize, e.selection);

    doc.markDirty();
}

void PencilTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);

    drawLine(doc, frameIndex, m_lastX, m_lastY, x, y, e.brushSize, e.selection);

    m_lastX = x;
    m_lastY = y;

    doc.markDirty();
}

void PencilTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    (void)doc;
    (void)frameIndex;
    (void)e;

    m_drawing = false;
    m_lastX = -1;
    m_lastY = -1;
}

void PencilTool::drawBrush(
    Document& doc,
    int frameIndex,
    int x,
    int y,
    int size,
    const Selection* selection
) {
    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    int half = size / 2;

    for (int dy = -half; dy < size - half; ++dy) {
        for (int dx = -half; dx < size - half; ++dx) {
            int px = x + dx;
            int py = y + dy;

            if (px < 0 || py < 0 || px >= cw || py >= ch)
                continue;

            if (!SelectionPixelClip::canModifyPixel(selection, px, py))
                continue;

            frame.setPixel(px, py, m_drawColor);
        }
    }
}

void PencilTool::drawLine(
    Document& doc,
    int frameIndex,
    int x0,
    int y0,
    int x1,
    int y1,
    int size,
    const Selection* selection
) {
    ShapeRasterizer::rasterizeLine(
        x0,
        y0,
        x1,
        y1,
        [&](int x, int y) {
            drawBrush(doc, frameIndex, x, y, size, selection);
        }
    );
}

} // namespace Framenote