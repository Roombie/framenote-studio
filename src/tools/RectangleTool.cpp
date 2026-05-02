#include "tools/RectangleTool.h"

#include "tools/SelectionPixelClip.h"

#include <algorithm>
#include <cstdint>

namespace Framenote {

void RectangleTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
    m_filled = e.filled;

    m_startX = static_cast<int>(e.canvasX);
    m_startY = static_cast<int>(e.canvasY);
    m_endX = m_startX;
    m_endY = m_startY;

    m_color = (e.rightDown && !e.leftDown)
        ? doc.palette().secondaryColor().toRGBA()
        : doc.palette().primaryColor().toRGBA();

    (void)frameIndex;
}

void RectangleTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);

    (void)doc;
    (void)frameIndex;
}

void RectangleTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_drawing)
        return;

    m_endX = static_cast<int>(e.canvasX);
    m_endY = static_cast<int>(e.canvasY);
    m_drawing = false;

    drawRect(
        doc,
        frameIndex,
        m_startX,
        m_startY,
        m_endX,
        m_endY,
        m_filled,
        m_color,
        e.selection
    );

    doc.markDirty();
}

void RectangleTool::drawRect(
    Document& doc,
    int frameIndex,
    int x0,
    int y0,
    int x1,
    int y1,
    bool fill,
    uint32_t color,
    const Selection* selection
) {
    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    auto plotPixel = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= cw || y >= ch)
            return;

        if (!SelectionPixelClip::canModifyPixel(selection, x, y))
            return;

        frame.setPixel(x, y, color);
    };

    if (fill) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                plotPixel(x, y);
            }
        }

        return;
    }

    for (int x = x0; x <= x1; ++x) {
        plotPixel(x, y0);
        plotPixel(x, y1);
    }

    for (int y = y0; y <= y1; ++y) {
        plotPixel(x0, y);
        plotPixel(x1, y);
    }
}

} // namespace Framenote