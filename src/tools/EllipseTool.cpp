#include "tools/EllipseTool.h"

#include "tools/ShapeRasterizer.h"
#include "tools/SelectionPixelClip.h"

#include <algorithm>
#include <cstdint>

namespace Framenote {

void EllipseTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;

    m_startX = static_cast<int>(e.canvasX);
    m_startY = static_cast<int>(e.canvasY);
    m_endX   = m_startX;
    m_endY   = m_startY;

    m_color = (e.rightDown && !e.leftDown)
        ? doc.palette().secondaryColor().toRGBA()
        : doc.palette().primaryColor().toRGBA();

    (void)frameIndex;
}

void EllipseTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);

    (void)doc;
    (void)frameIndex;
}

void EllipseTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);

    m_drawing = false;

    drawEllipse(
        doc,
        frameIndex,
        m_startX,
        m_startY,
        m_endX,
        m_endY,
        e.filled,
        m_color,
        e.selection
    );

    doc.markDirty();
}

void EllipseTool::drawEllipse(
    Document& doc,
    int frameIndex,
    int x0,
    int y0,
    int x1,
    int y1,
    bool filled,
    uint32_t color,
    const Selection* selection
) {
    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    ShapeRasterizer::rasterizeEllipse(
        x0,
        y0,
        x1,
        y1,
        filled,
        [&](int px, int py) {
            if (px < 0 || py < 0 || px >= cw || py >= ch)
                return;

            if (!SelectionPixelClip::canModifyPixel(selection, px, py))
                return;

            frame.setPixel(px, py, color);
        }
    );
}

} // namespace Framenote