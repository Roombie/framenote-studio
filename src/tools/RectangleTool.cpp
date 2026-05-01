#include "tools/RectangleTool.h"
#include "core/Selection.h"
#include <algorithm>

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
        y >= g_selectionClip->height())
        return false;

    return g_selectionClip->isSelected(x, y);
}

} // namespace

void RectangleTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
    m_filled  = e.filled;
    m_startX  = static_cast<int>(e.canvasX);
    m_startY  = static_cast<int>(e.canvasY);
    m_endX    = m_startX;
    m_endY    = m_startY;
    m_color   = (e.rightDown && !e.leftDown)
        ? doc.palette().secondaryColor().toRGBA()
        : doc.palette().primaryColor().toRGBA();

    (void)doc;
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

    m_endX    = static_cast<int>(e.canvasX);
    m_endY    = static_cast<int>(e.canvasY);
    m_drawing = false;

    {
        SelectionClipScope clip(e.selection);
        drawRect(doc, frameIndex, m_startX, m_startY, m_endX, m_endY,
                 m_filled, m_color);
    }

    doc.markDirty();
}

void RectangleTool::drawRect(Document& doc, int frameIndex,
                             int x0, int y0, int x1, int y1,
                             bool fill, uint32_t color) {
    auto& frame = doc.frame(frameIndex);

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    if (fill) {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (!canModifyPixel(x, y))
                    continue;

                frame.setPixel(x, y, color);
            }
        }
    } else {
        for (int x = x0; x <= x1; ++x) {
            if (canModifyPixel(x, y0))
                frame.setPixel(x, y0, color);

            if (canModifyPixel(x, y1))
                frame.setPixel(x, y1, color);
        }

        for (int y = y0; y <= y1; ++y) {
            if (canModifyPixel(x0, y))
                frame.setPixel(x0, y, color);

            if (canModifyPixel(x1, y))
                frame.setPixel(x1, y, color);
        }
    }
}

} // namespace Framenote