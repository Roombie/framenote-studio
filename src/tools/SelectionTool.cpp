#include "tools/SelectionTool.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"
#include <algorithm>

namespace Framenote {

// ── Float helpers ─────────────────────────────────────────────────────────────

void SelectionTool::liftFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!e.tab || !e.selection || e.selection->isEmpty()) return;
    if (e.tab->hasFloating) return;

    auto& frame = doc.frame(frameIndex);
    int cw = doc.canvasSize().width;
    SelectionRect bounds = e.selection->bounds();

    e.tab->floatW       = bounds.w;
    e.tab->floatH       = bounds.h;
    e.tab->floatOffsetX = bounds.x;
    e.tab->floatOffsetY = bounds.y;
    e.tab->floatStartX  = bounds.x;
    e.tab->floatStartY  = bounds.y;
    e.tab->floatPixels.assign(
        static_cast<size_t>(bounds.w * bounds.h * 4), 0);

    for (int py = 0; py < bounds.h; ++py) {
        for (int px = 0; px < bounds.w; ++px) {
            int cx = bounds.x + px;
            int cy = bounds.y + py;
            if (!e.selection->isSelected(cx, cy)) continue;
            size_t srcIdx = static_cast<size_t>((cy * cw + cx) * 4);
            size_t dstIdx = static_cast<size_t>((py * bounds.w + px) * 4);
            const auto& src = frame.pixels();
            e.tab->floatPixels[dstIdx + 0] = src[srcIdx + 0];
            e.tab->floatPixels[dstIdx + 1] = src[srcIdx + 1];
            e.tab->floatPixels[dstIdx + 2] = src[srcIdx + 2];
            e.tab->floatPixels[dstIdx + 3] = src[srcIdx + 3];
            frame.setPixel(cx, cy, 0x00000000);
        }
    }

    e.tab->hasFloating = true;
    doc.markDirty();
    (void)cw;
}

void SelectionTool::stampFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!e.tab || !e.tab->hasFloating) return;

    auto& frame = doc.frame(frameIndex);
    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;
    int fw = e.tab->floatW;
    int fh = e.tab->floatH;
    int ox = e.tab->floatOffsetX;
    int oy = e.tab->floatOffsetY;

    for (int py = 0; py < fh; ++py) {
        for (int px = 0; px < fw; ++px) {
            size_t srcIdx = static_cast<size_t>((py * fw + px) * 4);
            uint8_t a = e.tab->floatPixels[srcIdx + 3];
            if (a == 0) continue;
            int dx = ox + px, dy = oy + py;
            if (dx < 0 || dy < 0 || dx >= cw || dy >= ch) continue;
            uint32_t col =
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 3]) << 24) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 2]) << 16) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 1]) <<  8) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 0]));
            frame.setPixel(dx, dy, col);
        }
    }

    // Translate selection bitmask to match new position
    if (e.selection) {
        int deltaX = ox - e.tab->floatStartX;
        int deltaY = oy - e.tab->floatStartY;
        if (deltaX != 0 || deltaY != 0) {
            int sw = e.selection->width();
            int sh = e.selection->height();
            Selection translated(sw, sh);
            for (int y = 0; y < sh; ++y)
                for (int x = 0; x < sw; ++x) {
                    if (!e.selection->isSelected(x, y)) continue;
                    int nx = x + deltaX, ny = y + deltaY;
                    if (nx >= 0 && ny >= 0 && nx < sw && ny < sh)
                        translated.setRect(nx, ny, 1, 1);
                }
            *e.selection = std::move(translated);
        }
    }

    e.tab->hasFloating = false;
    e.tab->floatPixels.clear();
    doc.markDirty();
    (void)cw; (void)ch;
}

// ── Public ────────────────────────────────────────────────────────────────────

void SelectionTool::commitFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    stampFloat(doc, frameIndex, e);
    m_mode = Mode::Idle;
}

void SelectionTool::deselect(const ToolEvent& e) {
    if (e.selection) e.selection->clear();
    m_mode = Mode::Idle;
}

// ── Events ────────────────────────────────────────────────────────────────────
void SelectionTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    int px = static_cast<int>(e.canvasX);
    int py = static_cast<int>(e.canvasY);

    bool hasSelection =
        e.selection &&
        !e.selection->isEmpty();

    bool hasFloat =
        e.tab &&
        e.tab->hasFloating;

    bool insideFloatBounds =
        hasFloat &&
        px >= e.tab->floatOffsetX &&
        px <  e.tab->floatOffsetX + e.tab->floatW &&
        py >= e.tab->floatOffsetY &&
        py <  e.tab->floatOffsetY + e.tab->floatH;

    bool insideSelection = false;

    if (hasSelection) {
        if (hasFloat) {
            // The selection may be visually shifted because a float is active.
            // Hit-test against the translated selection mask, not the full float.
            int deltaX = e.tab->floatOffsetX - e.tab->floatStartX;
            int deltaY = e.tab->floatOffsetY - e.tab->floatStartY;

            int originalX = px - deltaX;
            int originalY = py - deltaY;

            insideSelection =
                originalX >= 0 &&
                originalY >= 0 &&
                originalX < e.selection->width() &&
                originalY < e.selection->height() &&
                e.selection->isSelected(originalX, originalY);
        } else {
            insideSelection = e.selection->isSelected(px, py);
        }
    }

    // Only allow insideFloatBounds to count as a move target when there is
    // no active selection mask. If a selection exists, the selection mask wins.
    bool insideMoveTarget =
        insideSelection ||
        (!hasSelection && insideFloatBounds);

    if (insideMoveTarget) {
        // Click inside existing selection/float: move it.
        if (e.tab && !e.tab->hasFloating)
            liftFloat(doc, frameIndex, e);

        m_mode      = Mode::Moving;
        m_dragLastX = px;
        m_dragLastY = py;
    }
    else if (e.addToSelection && e.selection && !e.selection->isEmpty()) {
        // Shift-click/drag outside: add to existing selection.
        if (e.tab && e.tab->hasFloating)
            stampFloat(doc, frameIndex, e);

        m_mode   = Mode::Selecting;
        m_startX = px;
        m_startY = py;
        m_endX   = px;
        m_endY   = py;
    }
    else {
        // Plain left-click outside:
        // Commit any floating pixels, then clear the selection.
        // Do NOT immediately create a new 1x1 selection.
        if (e.tab && e.tab->hasFloating)
            stampFloat(doc, frameIndex, e);

        if (e.selection)
            e.selection->clear();

        m_mode   = Mode::PendingSelecting;
        m_startX = px;
        m_startY = py;
        m_endX   = px;
        m_endY   = py;
    }

    (void)frameIndex;
}

void SelectionTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    int px = static_cast<int>(e.canvasX);
    int py = static_cast<int>(e.canvasY);

    if (m_mode == Mode::Moving && e.tab && e.tab->hasFloating) {
        e.tab->floatOffsetX += px - m_dragLastX;
        e.tab->floatOffsetY += py - m_dragLastY;

        m_dragLastX = px;
        m_dragLastY = py;
    }
    else if (m_mode == Mode::PendingSelecting) {
        // Only become a real selection operation after actual movement.
        if (px != m_startX || py != m_startY) {
            m_mode = Mode::Selecting;
            m_endX = px;
            m_endY = py;
        }
    }
    else if (m_mode == Mode::Selecting) {
        m_endX = px;
        m_endY = py;
    }

    (void)doc;
    (void)frameIndex;
}

void SelectionTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    if (m_mode == Mode::PendingSelecting) {
        // Clicked outside and released without dragging.
        // Selection was already cleared in onPress.
        m_mode = Mode::Idle;

        (void)doc;
        (void)frameIndex;
        (void)e;
        return;
    }

    if (m_mode == Mode::Selecting) {
        m_endX = static_cast<int>(e.canvasX);
        m_endY = static_cast<int>(e.canvasY);

        int x0 = std::min(m_startX, m_endX);
        int y0 = std::min(m_startY, m_endY);
        int x1 = std::max(m_startX, m_endX);
        int y1 = std::max(m_startY, m_endY);

        int w = x1 - x0 + 1;
        int h = y1 - y0 + 1;

        if (e.selection && w > 0 && h > 0) {
            if (e.rightDown && !e.leftDown)
                e.selection->removeRect(x0, y0, w, h);
            else if (e.addToSelection)
                e.selection->addRect(x0, y0, w, h);
            else
                e.selection->setRect(x0, y0, w, h);
        }
    }

    m_mode = Mode::Idle;

    (void)doc;
    (void)frameIndex;
}

} // namespace Framenote