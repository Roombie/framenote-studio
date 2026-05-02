#include "tools/EllipseTool.h"

#include "core/Selection.h"
#include "tools/ShapeRasterizer.h"

#include <algorithm>
#include <cstdint>

namespace Framenote {
namespace {

const Selection* g_selectionClip = nullptr;

class SelectionClipScope {
public:
    explicit SelectionClipScope(const Selection* selection)
        : m_previous(g_selectionClip) {
        g_selectionClip = selection;
    }

    ~SelectionClipScope() {
        g_selectionClip = m_previous;
    }

private:
    const Selection* m_previous = nullptr;
};

bool canModifyPixel(int x, int y) {
    if (!g_selectionClip || g_selectionClip->isEmpty())
        return true;

    if (x < 0 || y < 0 ||
        x >= g_selectionClip->width() ||
        y >= g_selectionClip->height()) {
        return false;
    }

    return g_selectionClip->isSelected(x, y);
}

} // namespace

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

    {
        SelectionClipScope clip(e.selection);

        drawEllipse(
            doc,
            frameIndex,
            m_startX,
            m_startY,
            m_endX,
            m_endY,
            e.filled,
            m_color
        );
    }

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
    uint32_t color
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

            if (!canModifyPixel(px, py))
                return;

            frame.setPixel(px, py, color);
        }
    );
}

} // namespace Framenote