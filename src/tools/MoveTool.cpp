#include "tools/MoveTool.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"

#include <cstdint>
#include <utility>

namespace Framenote {

namespace {

// Commits any existing floating pixels back into the frame.
//
// This is important when switching from SelectionTool -> MoveTool.
// The SelectionTool can leave selected pixels floating after a selection move.
// If MoveTool does not commit that first, it will accidentally keep moving only
// the selected floating region instead of lifting the whole canvas.
void commitExistingFloatingSelection(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!e.tab || !e.tab->hasFloating)
        return;

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
            if (a == 0)
                continue;

            int dx = ox + px;
            int dy = oy + py;

            if (dx < 0 || dy < 0 || dx >= cw || dy >= ch)
                continue;

            uint32_t col =
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 3]) << 24) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 2]) << 16) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 1]) <<  8) |
                (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 0]));

            frame.setPixel(dx, dy, col);
        }
    }

    // Keep the Photoshop/Aseprite behavior:
    // pixels are committed, but the selection itself remains and follows the moved pixels.
    if (e.selection && !e.selection->isEmpty()) {
        int deltaX = e.tab->floatOffsetX - e.tab->floatStartX;
        int deltaY = e.tab->floatOffsetY - e.tab->floatStartY;

        if (deltaX != 0 || deltaY != 0) {
            int sw = e.selection->width();
            int sh = e.selection->height();

            Selection translated(sw, sh);

            for (int y = 0; y < sh; ++y) {
                for (int x = 0; x < sw; ++x) {
                    if (!e.selection->isSelected(x, y))
                        continue;

                    int nx = x + deltaX;
                    int ny = y + deltaY;

                    if (nx >= 0 && ny >= 0 && nx < sw && ny < sh)
                        translated.setRect(nx, ny, 1, 1);
                }
            }

            *e.selection = std::move(translated);
        }
    }

    e.tab->hasFloating = false;
    e.tab->floatingSource = FloatingSource::None;
    e.tab->floatPixels.clear();
    e.tab->floatW = 0;
    e.tab->floatH = 0;
    e.tab->floatOffsetX = 0;
    e.tab->floatOffsetY = 0;
    e.tab->floatStartX = 0;
    e.tab->floatStartY = 0;

    doc.markDirty();
}

} // namespace

// ── liftFloat ────────────────────────────────────────────────────────────────
// Move tool behavior:
// - Always move the whole canvas/layer content.
// - If SelectionTool left a floating selected region active, commit it first.
// - Do NOT clear the selection.
// - The selection remains visible and keeps its current position.

void MoveTool::liftFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!e.tab)
        return;

    // Fix:
    // If we came from SelectionTool and it left selected pixels floating,
    // commit them first so MoveTool can start from a clean whole-canvas state.
    if (e.tab->hasFloating) {
        commitExistingFloatingSelection(doc, frameIndex, e);
    }

    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    SelectionRect bounds{0, 0, cw, ch};

    e.tab->floatW       = bounds.w;
    e.tab->floatH       = bounds.h;
    e.tab->floatOffsetX = 0;
    e.tab->floatOffsetY = 0;
    e.tab->floatStartX  = 0;
    e.tab->floatStartY  = 0;

    e.tab->floatPixels.assign(
        static_cast<size_t>(bounds.w * bounds.h * 4),
        0
    );

    const auto& src = frame.pixels();

    for (int py = 0; py < bounds.h; ++py) {
        for (int px = 0; px < bounds.w; ++px) {
            int cx = px;
            int cy = py;

            size_t srcIdx = static_cast<size_t>((cy * cw + cx) * 4);
            size_t dstIdx = static_cast<size_t>((py * bounds.w + px) * 4);

            e.tab->floatPixels[dstIdx + 0] = src[srcIdx + 0];
            e.tab->floatPixels[dstIdx + 1] = src[srcIdx + 1];
            e.tab->floatPixels[dstIdx + 2] = src[srcIdx + 2];
            e.tab->floatPixels[dstIdx + 3] = src[srcIdx + 3];
        }
    }

    e.tab->hasFloating = true;
    e.tab->floatingSource = FloatingSource::CanvasMove;
}

// ── stampFloat ────────────────────────────────────────────────────────────────

void MoveTool::stampFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!e.tab || !e.tab->hasFloating || e.tab->floatingSource != FloatingSource::CanvasMove) {
        return;
    }

    auto& frame = doc.frame(frameIndex);

    int cw = doc.canvasSize().width;
    int ch = doc.canvasSize().height;

    int fw = e.tab->floatW;
    int fh = e.tab->floatH;
    int ox = e.tab->floatOffsetX;
    int oy = e.tab->floatOffsetY;

    int deltaX = e.tab->floatOffsetX - e.tab->floatStartX;
    int deltaY = e.tab->floatOffsetY - e.tab->floatStartY;

    if (m_erased) {
        for (int py = 0; py < fh; ++py) {
            for (int px = 0; px < fw; ++px) {
                size_t srcIdx = static_cast<size_t>((py * fw + px) * 4);

                uint8_t a = e.tab->floatPixels[srcIdx + 3];
                if (a == 0)
                    continue;

                int dx = ox + px;
                int dy = oy + py;

                if (dx < 0 || dy < 0 || dx >= cw || dy >= ch)
                    continue;

                uint32_t col =
                    (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 3]) << 24) |
                    (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 2]) << 16) |
                    (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 1]) <<  8) |
                    (static_cast<uint32_t>(e.tab->floatPixels[srcIdx + 0]));

                frame.setPixel(dx, dy, col);
            }
        }

        doc.markDirty();
    }

    // Keep the selection, but move its mask with the whole-canvas movement.
    if (e.selection && !e.selection->isEmpty() && m_erased) {
        if (deltaX != 0 || deltaY != 0) {
            int sw = e.selection->width();
            int sh = e.selection->height();

            Selection translated(sw, sh);

            for (int y = 0; y < sh; ++y) {
                for (int x = 0; x < sw; ++x) {
                    if (!e.selection->isSelected(x, y))
                        continue;

                    int nx = x + deltaX;
                    int ny = y + deltaY;

                    if (nx >= 0 && ny >= 0 && nx < sw && ny < sh)
                        translated.setRect(nx, ny, 1, 1);
                }
            }

            *e.selection = std::move(translated);
        }
    }

    e.tab->hasFloating = false;
    e.tab->floatingSource = FloatingSource::None;
    e.tab->floatPixels.clear();
    e.tab->floatW = 0;
    e.tab->floatH = 0;
    e.tab->floatOffsetX = 0;
    e.tab->floatOffsetY = 0;
    e.tab->floatStartX = 0;
    e.tab->floatStartY = 0;
}

// ── Tool events ───────────────────────────────────────────────────────────────

void MoveTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    m_dragging = true;
    m_lifted   = false;
    m_erased   = false;

    m_lastX = static_cast<int>(e.canvasX);
    m_lastY = static_cast<int>(e.canvasY);

    (void)doc;
    (void)frameIndex;
    (void)e;
}

void MoveTool::onDrag(Document& doc, int frameIndex, const ToolEvent& e) {
    if (!m_dragging || !e.tab)
        return;

    int px = static_cast<int>(e.canvasX);
    int py = static_cast<int>(e.canvasY);

    int dx = px - m_lastX;
    int dy = py - m_lastY;

    if (dx == 0 && dy == 0)
        return;

    if (!m_lifted) {
        m_lifted = true;
        liftFloat(doc, frameIndex, e);
    }

    if (!e.tab->hasFloating ||
        e.tab->floatingSource != FloatingSource::CanvasMove) {
        return;
    }

    // Erase the original whole drawing only once, on first real movement.
    if (!m_erased) {
        m_erased = true;

        auto& frame = doc.frame(frameIndex);

        int cw = doc.canvasSize().width;
        int ch = doc.canvasSize().height;

        int fw = e.tab->floatW;
        int fh = e.tab->floatH;

        for (int fy = 0; fy < fh; ++fy) {
            for (int fx = 0; fx < fw; ++fx) {
                size_t idx = static_cast<size_t>((fy * fw + fx) * 4 + 3);

                if (e.tab->floatPixels[idx] == 0)
                    continue;

                if (fx >= 0 && fy >= 0 && fx < cw && fy < ch)
                    frame.setPixel(fx, fy, 0x00000000);
            }
        }

        doc.markDirty();
    }

    e.tab->floatOffsetX += dx;
    e.tab->floatOffsetY += dy;

    m_lastX = px;
    m_lastY = py;
}

void MoveTool::onRelease(Document& doc, int frameIndex, const ToolEvent& e) {
    m_dragging = false;

    (void)doc;
    (void)frameIndex;
    (void)e;
}

void MoveTool::commitFloat(Document& doc, int frameIndex, const ToolEvent& e) {
    // If MoveTool is active but did not lift anything yet, do NOT destroy
    // an existing floating selection. Commit it safely instead.
    if (!m_lifted) {
        if (e.tab &&
            e.tab->hasFloating &&
            e.tab->floatingSource == FloatingSource::CanvasMove) {
            stampFloat(doc, frameIndex, e);
        }

        m_dragging = false;
        m_lifted   = false;
        m_erased   = false;
        return;
    }

    stampFloat(doc, frameIndex, e);

    m_dragging = false;
    m_lifted   = false;
    m_erased   = false;
}

} // namespace Framenote