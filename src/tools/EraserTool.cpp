#include "tools/EraserTool.h"
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

void EraserTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_erasing = true;

    m_lastX = static_cast<int>(e.canvasX);
    m_lastY = static_cast<int>(e.canvasY);

    {
        SelectionClipScope clip(e.selection);
        eraseBrush(doc, frameIndex, m_lastX, m_lastY, e.brushSize);
    }

    doc.markDirty();
}

void EraserTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_erasing)
        return;

    if (m_lastX < 0 || m_lastY < 0)
        return;

    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);

    {
        SelectionClipScope clip(e.selection);
        eraseLine(doc, frameIndex, m_lastX, m_lastY, x, y, e.brushSize);
    }

    m_lastX = x;
    m_lastY = y;

    doc.markDirty();
}

void EraserTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    (void)doc;
    (void)frameIndex;
    (void)e;

    m_erasing = false;

    m_lastX = -1;
    m_lastY = -1;
}

void EraserTool::eraseBrush(Document& doc, int frameIndex, int x, int y, int size) {
    auto& frame = doc.frame(frameIndex);

    int half = size / 2;

    for (int dy = -half; dy < size - half; ++dy) {
        for (int dx = -half; dx < size - half; ++dx) {
            int px = x + dx;
            int py = y + dy;

            if (!canModifyPixel(px, py))
                continue;

            frame.setPixel(px, py, 0x00000000);
        }
    }
}

void EraserTool::eraseLine(Document& doc, int frameIndex,
                           int x0, int y0, int x1, int y1, int size) {
    int dx =  std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        eraseBrush(doc, frameIndex, x0, y0, size);

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