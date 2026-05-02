#include "ui/CanvasPanel.h"

#include "core/Selection.h"
#include "core/DocumentTab.h"

#include "tools/SelectionTool.h"
#include "tools/MoveTool.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <utility>
#include <cstddef>

namespace Framenote {

// ─────────────────────────────────────────────────────────────────────────────
// Editor actions
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::commitFloatIfNeeded() {
    if (!m_tab || !m_tab->hasFloating)
        return;

    ToolEvent ce;
    ce.selection = m_selection;
    ce.tab = m_tab;

    if (m_tab->floatingSource == FloatingSource::Selection) {
        auto* selectionTool = static_cast<SelectionTool*>(
            m_toolManager->getTool(ToolType::Select)
        );

        if (selectionTool) {
            selectionTool->commitFloat(
                *m_document,
                m_timeline->currentFrame(),
                ce
            );
        }
    }
    else if (m_tab->floatingSource == FloatingSource::CanvasMove) {
        auto* moveTool = static_cast<MoveTool*>(
            m_toolManager->getTool(ToolType::Move)
        );

        if (moveTool) {
            moveTool->commitFloat(
                *m_document,
                m_timeline->currentFrame(),
                ce
            );
        }
    }
}

void CanvasPanel::commitSelectionFloatIfNeeded() {
    if (!m_tab ||
        !m_tab->hasFloating ||
        m_tab->floatingSource != FloatingSource::Selection) {
        return;
    }

    auto* selectionTool = static_cast<SelectionTool*>(
        m_toolManager->getTool(ToolType::Select)
    );

    if (!selectionTool)
        return;

    ToolEvent ce;
    ce.selection = m_selection;
    ce.tab = m_tab;

    selectionTool->commitFloat(
        *m_document,
        m_timeline->currentFrame(),
        ce
    );
}

ToolEvent CanvasPanel::makeToolEvent(
    const ImVec2& mousePos,
    float originX,
    float originY,
    int canvasW,
    int canvasH,
    bool clampCoords
) {
    ImGuiIO& io = ImGui::GetIO();

    int px = static_cast<int>(std::floor((mousePos.x - originX) / m_zoom));
    int py = static_cast<int>(std::floor((mousePos.y - originY) / m_zoom));

    if (clampCoords) {
        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px >= canvasW) px = canvasW - 1;
        if (py >= canvasH) py = canvasH - 1;
    }

    ToolEvent e;
    e.canvasX = static_cast<float>(px);
    e.canvasY = static_cast<float>(py);
    e.leftDown = io.MouseDown[0];
    e.rightDown = io.MouseDown[1];
    e.brushSize = m_toolManager->brushSize();
    e.filled = m_toolManager->rectFilled();
    e.addToSelection = io.KeyShift;
    e.selection = m_selection;
    e.tab = m_tab;

    return e;
}

void CanvasPanel::pushCurrentFrameSnapshot() {
    int fi = m_timeline->currentFrame();
    auto& f = m_document->frame(fi);

    Snapshot snap;
    snap.frameIndex = fi;
    snap.bufferWidth = f.bufferWidth();
    snap.bufferHeight = f.bufferHeight();
    snap.pixels = f.pixels();

    m_history.push(std::move(snap));
}

void CanvasPanel::clearFloatingState() {
    if (!m_tab)
        return;

    m_tab->hasFloating = false;
    m_tab->floatingSource = FloatingSource::None;
    m_tab->floatPixels.clear();
    m_tab->floatW = 0;
    m_tab->floatH = 0;
    m_tab->floatOffsetX = 0;
    m_tab->floatOffsetY = 0;
    m_tab->floatStartX = 0;
    m_tab->floatStartY = 0;
}

void CanvasPanel::deleteSelectionOrFrame() {
    int fi = m_timeline->currentFrame();
    auto& frame = m_document->frame(fi);

    int cw = m_document->canvasSize().width;
    int ch = m_document->canvasSize().height;

    bool hasSelection =
        m_selection &&
        !m_selection->isEmpty();

    bool hasFloating =
        m_tab &&
        m_tab->hasFloating;

    bool frameHasPixels = false;

    const auto& pixels = frame.pixels();

    for (size_t i = 3; i < pixels.size(); i += 4) {
        if (pixels[i] != 0) {
            frameHasPixels = true;
            break;
        }
    }

    if (!hasSelection && !hasFloating && !frameHasPixels)
        return;

    pushCurrentFrameSnapshot();

    if (hasFloating) {
        clearFloatingState();
        m_document->markDirty();
        return;
    }

    if (hasSelection) {
        int sw = m_selection->width();
        int sh = m_selection->height();

        for (int y = 0; y < sh; ++y) {
            for (int x = 0; x < sw; ++x) {
                if (!m_selection->isSelected(x, y))
                    continue;

                if (x < 0 || y < 0 || x >= cw || y >= ch)
                    continue;

                frame.setPixel(x, y, 0x00000000);
            }
        }
    }
    else {
        for (int y = 0; y < ch; ++y) {
            for (int x = 0; x < cw; ++x) {
                frame.setPixel(x, y, 0x00000000);
            }
        }
    }

    m_document->markDirty();
}
} // namespace Framenote