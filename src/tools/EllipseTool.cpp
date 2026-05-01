#include "tools/EllipseTool.h"
#include "core/Selection.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

void EllipseTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
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

    m_endX    = static_cast<int>(e.canvasX);
    m_endY    = static_cast<int>(e.canvasY);
    m_drawing = false;

    {
        SelectionClipScope clip(e.selection);
        drawEllipse(doc, frameIndex, m_startX, m_startY, m_endX, m_endY,
                    e.filled, m_color);
    }

    doc.markDirty();
}

void EllipseTool::drawEllipse(Document& doc, int frameIndex,
                              int x0, int y0, int x1, int y1,
                              bool fill, uint32_t color) {
    auto& frame = doc.frame(frameIndex);

    if (x0 > x1)
        std::swap(x0, x1);

    if (y0 > y1)
        std::swap(y0, y1);

    float cx = (x0 + x1) * 0.5f;
    float cy = (y0 + y1) * 0.5f;
    float rx = (x1 - x0) * 0.5f;
    float ry = (y1 - y0) * 0.5f;

    if (rx < 0.5f && ry < 0.5f) {
        if (canModifyPixel(x0, y0))
            frame.setPixel(x0, y0, color);

        return;
    }

    for (int py = y0; py <= y1; ++py) {
        for (int px = x0; px <= x1; ++px) {
            if (!canModifyPixel(px, py))
                continue;

            float dx = px - cx + 0.5f;
            float dy = py - cy + 0.5f;
            float rx2 = rx + 0.5f;
            float ry2 = ry + 0.5f;

            bool inside = (rx > 0 && ry > 0)
                ? ((dx * dx) / (rx2 * rx2) + (dy * dy) / (ry2 * ry2)) <= 1.0f
                : false;

            if (!inside)
                continue;

            if (fill) {
                frame.setPixel(px, py, color);
            } else {
                float irx = rx - 0.5f;
                float iry = ry - 0.5f;

                bool innerInside = (irx > 0 && iry > 0)
                    ? ((dx * dx) / (irx * irx) + (dy * dy) / (iry * iry)) <= 1.0f
                    : false;

                if (!innerInside)
                    frame.setPixel(px, py, color);
            }
        }
    }
}

} // namespace Framenote