#include "ui/CanvasPanel.h"

#include "core/Palette.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"

#include "tools/SelectionTool.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace Framenote {

namespace {

float fnClamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Input handling
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::handleKeyboardShortcuts(bool ctrlHeld, bool popupOpen) {
    ImGuiIO& io = ImGui::GetIO();

    if (ctrlHeld || io.WantTextInput || popupOpen)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Pencil);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Eraser);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Fill);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_I, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Eyedropper);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_L, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Line);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Rectangle);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Ellipse);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_M, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Select);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        commitFloatIfNeeded();
        m_toolManager->selectTool(ToolType::Move);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        commitFloatIfNeeded();

        if (m_selection)
            m_selection->clear();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) ||
        ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
        deleteSelectionOrFrame();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
        if (m_tab && m_tab->hasFloating) {
            commitFloatIfNeeded();
        }
        else {
            m_timeline->isPlaying()
                ? m_timeline->pause()
                : m_timeline->play();
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false)) {
        m_toolManager->setBrushSize(m_toolManager->brushSize() - 1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
        m_toolManager->setBrushSize(m_toolManager->brushSize() + 1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
        m_timeline->nextFrame();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
        m_timeline->prevFrame();
    }

    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
        if (m_timeline->isPlaying())
            m_timeline->pause();

        m_timeline->setCurrentFrame(0);
    }

    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
        if (m_timeline->isPlaying())
            m_timeline->pause();

        m_timeline->setCurrentFrame(m_timeline->frameCount() - 1);
    }
}

void CanvasPanel::handleZoomInput(
    const ImVec2& mousePos,
    const ImVec2& panelPos,
    const ImVec2& panelSize,
    bool inWindow,
    bool ctrlHeld,
    bool popupOpen
) {
    ImGuiIO& io = ImGui::GetIO();

    if (inWindow && !io.WantTextInput && !popupOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom < 2.0f ? 0.25f : (m_zoom < 8.0f ? 0.5f : 1.0f);

            m_zoom = fnClamp(m_zoom + step, 0.25f, 64.0f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom <= 2.0f ? 0.25f : (m_zoom <= 8.0f ? 0.5f : 1.0f);

            m_zoom = fnClamp(m_zoom - step, 0.25f, 64.0f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }

        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_0, false)) {
            m_zoom = 1.0f;
            m_panX = 0.0f;
            m_panY = 0.0f;
        }
    }

    if (inWindow && !popupOpen && io.MouseWheel != 0.0f) {
        float step = m_zoom < 2.0f ? 0.25f : (m_zoom < 8.0f ? 0.5f : 1.0f);
        float oldZoom = m_zoom;

        m_zoom = fnClamp(
            io.MouseWheel > 0.0f ? m_zoom + step : m_zoom - step,
            0.25f,
            64.0f
        );

        float ratio = m_zoom / oldZoom;

        m_panX =
            (m_panX - (mousePos.x - (panelPos.x + panelSize.x * 0.5f))) * ratio +
            (mousePos.x - (panelPos.x + panelSize.x * 0.5f));

        m_panY =
            (m_panY - (mousePos.y - (panelPos.y + panelSize.y * 0.5f))) * ratio +
            (mousePos.y - (panelPos.y + panelSize.y * 0.5f));

        io.MouseWheel = 0.0f;
    }
}

void CanvasPanel::handlePanInput(
    bool inWindow,
    bool spaceHeld,
    bool popupOpen,
    bool imguiCapturing,
    bool drawingMouseDown
) {
    ImGuiIO& io = ImGui::GetIO();

    bool isPanning =
        io.MouseDown[2] ||
        (spaceHeld && io.MouseDown[0]);

    bool isDrawing =
        m_strokeActive &&
        !spaceHeld &&
        !io.MouseDown[2] &&
        drawingMouseDown;

    if (inWindow && isPanning && !isDrawing && !popupOpen && !imguiCapturing) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }
}

void CanvasPanel::handleToolInput(
    const ImVec2& mousePos,
    const ImVec2& mouseDelta,
    const ImVec2& canvasMin,
    const ImVec2& canvasMax,
    float originX,
    float originY,
    int canvasW,
    int canvasH,
    bool canvasInputHovered,
    bool canvasWindowHovered,
    bool spaceHeld,
    bool popupOpen,
    bool imguiCapturing,
    bool drawingMouseDown,
    bool drawingMousePressed
) {
    ImGuiIO& io = ImGui::GetIO();

    Tool* tool = m_toolManager->activeTool();
    ToolType activeTool = m_toolManager->activeToolType();

    bool isFreehandTool =
        activeTool == ToolType::Pencil ||
        activeTool == ToolType::Eraser;

    bool isShapeTool =
        activeTool == ToolType::Line ||
        activeTool == ToolType::Rectangle ||
        activeTool == ToolType::Ellipse;

    bool isSelectionTool =
        activeTool == ToolType::Select;

    bool isMoveTool =
        activeTool == ToolType::Move;

    bool isDragOutsideTool =
        isShapeTool ||
        isSelectionTool ||
        isMoveTool;

    bool canStartFromOutsideThenEnter =
        isFreehandTool ||
        isShapeTool ||
        isSelectionTool ||
        isMoveTool;

    auto makeToolEventForActiveTool = [&]() {
        bool clampCoords = !isMoveTool;

        return makeToolEvent(
            mousePos,
            originX,
            originY,
            canvasW,
            canvasH,
            clampCoords
        );
    };

    bool canDraw =
        !spaceHeld &&
        !io.MouseDown[2] &&
        !popupOpen &&
        !imguiCapturing;

    ImVec2 prevMousePos = {
        mousePos.x - mouseDelta.x,
        mousePos.y - mouseDelta.y
    };

    bool rawInCanvas =
        mousePos.x >= canvasMin.x &&
        mousePos.x <  canvasMax.x &&
        mousePos.y >= canvasMin.y &&
        mousePos.y <  canvasMax.y;

    bool prevRawInCanvas =
        prevMousePos.x >= canvasMin.x &&
        prevMousePos.x <  canvasMax.x &&
        prevMousePos.y >= canvasMin.y &&
        prevMousePos.y <  canvasMax.y;

    bool enteredCanvasThisFrame =
        rawInCanvas &&
        !prevRawInCanvas;

    bool pressedOnCanvas =
        drawingMousePressed &&
        rawInCanvas &&
        canvasInputHovered;

    bool pressedInEmptyCanvasWindow =
        drawingMousePressed &&
        !rawInCanvas &&
        canvasWindowHovered &&
        !ImGui::IsAnyItemActive();

    if (tool &&
        isSelectionTool &&
        pressedInEmptyCanvasWindow) {
        commitSelectionFloatIfNeeded();

        if (m_selection)
            m_selection->clear();

        if (m_tab) {
            m_tab->canvasToolDragArmed =
                canDraw &&
                canStartFromOutsideThenEnter;
        }
    }

    bool canStartFromCanvasClick =
        tool &&
        canDraw &&
        pressedOnCanvas;

    bool canStartMoveFromOutsideCanvas =
        tool &&
        isMoveTool &&
        canDraw &&
        pressedInEmptyCanvasWindow;

    if (drawingMousePressed && m_tab) {
        bool canArm =
            tool &&
            canDraw &&
            canStartFromOutsideThenEnter &&
            pressedInEmptyCanvasWindow;

        m_tab->canvasToolDragArmed = canArm;
    }

    if (!drawingMouseDown || !canDraw) {
        if (m_tab)
            m_tab->canvasToolDragArmed = false;
    }

    bool canStartFromArmedOutsideDrag =
        tool &&
        canDraw &&
        drawingMouseDown &&
        rawInCanvas &&
        m_tab &&
        m_tab->canvasToolDragArmed &&
        !m_strokeActive;

    bool canStartStroke =
        canStartFromCanvasClick ||
        canStartMoveFromOutsideCanvas ||
        canStartFromArmedOutsideDrag;

    bool shouldStopStroke =
        !drawingMouseDown ||
        !canDraw;

    if (tool && m_strokeActive && shouldStopStroke) {
        ToolEvent e = makeToolEventForActiveTool();

        int releaseFrame =
            m_strokeFrameIndex >= 0
                ? m_strokeFrameIndex
                : m_timeline->currentFrame();

        tool->onRelease(*m_document, releaseFrame, e);

        m_strokeActive = false;
        m_strokeFrameIndex = -1;

        if (m_tab)
            m_tab->canvasToolDragArmed = false;
    }

    if (!tool || !canDraw || !drawingMouseDown)
        return;

    int fi = m_timeline->currentFrame();

    if (!m_strokeActive) {
        if (!canStartStroke)
            return;

        ToolEvent e = makeToolEventForActiveTool();

        pushCurrentFrameSnapshot();

        m_strokeActive = true;
        m_strokeFrameIndex = fi;

        if (m_tab)
            m_tab->canvasToolDragArmed = false;

        tool->onPress(*m_document, fi, e);
        return;
    }

    if (isFreehandTool) {
        if (!rawInCanvas)
            return;

        ToolEvent e = makeToolEventForActiveTool();

        if (enteredCanvasThisFrame) {
            tool->onPress(*m_document, fi, e);
        }
        else {
            tool->onDrag(*m_document, fi, e);
        }

        return;
    }

    if (isShapeTool || isSelectionTool) {
        if (rawInCanvas || isDragOutsideTool) {
            ToolEvent e = makeToolEventForActiveTool();
            tool->onDrag(*m_document, fi, e);
        }

        return;
    }

    if (isMoveTool) {
        ToolEvent e = makeToolEventForActiveTool();
        tool->onDrag(*m_document, fi, e);
        return;
    }

    if (rawInCanvas) {
        ToolEvent e = makeToolEventForActiveTool();
        tool->onDrag(*m_document, fi, e);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Cursor / preview helpers
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::drawBrushCursorPreview(
    ImDrawList* dl,
    const ImVec2& mousePos,
    float originX,
    float originY,
    int canvasW,
    int canvasH,
    bool inCanvas,
    bool spaceHeld,
    bool popupOpen
) {
    if (!inCanvas || spaceHeld || popupOpen)
        return;

    ToolType activeTool = m_toolManager->activeToolType();

    bool showCursor =
        activeTool == ToolType::Pencil ||
        activeTool == ToolType::Eraser ||
        activeTool == ToolType::Line ||
        activeTool == ToolType::Rectangle ||
        activeTool == ToolType::Ellipse;

    if (!showCursor)
        return;

    ImGuiIO& io = ImGui::GetIO();

    int bsize = m_toolManager->brushSize();
    int half = bsize / 2;

    int px = static_cast<int>((mousePos.x - originX) / m_zoom);
    int py = static_cast<int>((mousePos.y - originY) / m_zoom);

    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px >= canvasW) px = canvasW - 1;
    if (py >= canvasH) py = canvasH - 1;

    float rx0 = originX + (px - half) * m_zoom;
    float ry0 = originY + (py - half) * m_zoom;
    float rx1 = originX + (px - half + bsize) * m_zoom;
    float ry1 = originY + (py - half + bsize) * m_zoom;

    if (activeTool == ToolType::Pencil ||
        activeTool == ToolType::Line ||
        activeTool == ToolType::Rectangle ||
        activeTool == ToolType::Ellipse) {
        Color c =
            (io.MouseDown[1] && !io.MouseDown[0])
                ? m_document->palette().secondaryColor()
                : m_document->palette().primaryColor();

        if (c.a > 0) {
            dl->AddRectFilled(
                {rx0, ry0},
                {rx1, ry1},
                IM_COL32(c.r, c.g, c.b, 255)
            );
        }
    }

    dl->AddRect(
        {rx0 - 1, ry0 - 1},
        {rx1 + 1, ry1 + 1},
        IM_COL32(0, 0, 0, 180),
        0.0f,
        0,
        1.5f
    );

    dl->AddRect(
        {rx0, ry0},
        {rx1, ry1},
        IM_COL32(255, 255, 255, 220),
        0.0f,
        0,
        1.0f
    );
}

void CanvasPanel::handleCursorBehavior(
    bool inWindow,
    bool inCanvas,
    bool canvasWindowHovered,
    bool canvasInputHovered,
    bool spaceHeld,
    bool popupOpen
) {
    ImGuiIO& io = ImGui::GetIO();

    ToolType activeTool = m_toolManager->activeToolType();

    bool brushTool =
        activeTool == ToolType::Pencil ||
        activeTool == ToolType::Eraser;

    bool activelyDrawing =
        inCanvas &&
        brushTool &&
        !spaceHeld &&
        !popupOpen &&
        (io.MouseDown[0] || io.MouseDown[1]);

    if (spaceHeld && inWindow) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    else if (activelyDrawing) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
    else if (canvasWindowHovered || canvasInputHovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    }
}
} // namespace Framenote