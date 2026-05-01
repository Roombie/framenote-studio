#include "tools/LineTool.h"
#include "core/Selection.h"
#include <cmath>

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

void LineTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_drawing = true;
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

    m_endX    = static_cast<int>(e.canvasX);
    m_endY    = static_cast<int>(e.canvasY);
    m_drawing = false;

    {
        SelectionClipScope clip(e.selection);
        drawLine(doc, frameIndex, m_startX, m_startY, m_endX, m_endY,
                 e.brushSize, m_color);
    }

    doc.markDirty();
}

void LineTool::drawBrush(Document& doc, int frameIndex,
                         int x, int y, int brushSize, uint32_t color) {
    auto& frame = doc.frame(frameIndex);

    int half = brushSize / 2;

    for (int dy = -half; dy < brushSize - half; ++dy) {
        for (int dx = -half; dx < brushSize - half; ++dx) {
            int px = x + dx;
            int py = y + dy;

            if (!canModifyPixel(px, py))
                continue;

            frame.setPixel(px, py, color);
        }
    }
}

void LineTool::drawLine(Document& doc, int frameIndex,
                        int x0, int y0, int x1, int y1,
                        int brushSize, uint32_t color) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        drawBrush(doc, frameIndex, x0, y0, brushSize, color);

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