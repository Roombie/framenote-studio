#include "ui/CanvasPanel.h"

#include "core/Palette.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"

#include "tools/LineTool.h"
#include "tools/RectangleTool.h"
#include "tools/EllipseTool.h"
#include "tools/SelectionTool.h"
#include "tools/MoveTool.h"
#include "tools/ShapeRasterizer.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace Framenote {

static float fnClamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

CanvasPanel::CanvasPanel(
    Document* document,
    Timeline* timeline,
    ToolManager* toolManager,
    CanvasRenderer* renderer,
    float& zoom,
    float& panX,
    float& panY,
    History& history,
    bool& strokeActive,
    int& strokeFrameIndex,
    Selection* selection,
    DocumentTab* tab
)
    : m_document(document)
    , m_timeline(timeline)
    , m_toolManager(toolManager)
    , m_renderer(renderer)
    , m_zoom(zoom)
    , m_panX(panX)
    , m_panY(panY)
    , m_history(history)
    , m_strokeActive(strokeActive)
    , m_strokeFrameIndex(strokeFrameIndex)
    , m_selection(selection)
    , m_tab(tab)
{}

void CanvasPanel::render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos({180, 62}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        {
            io.DisplaySize.x - 240,
            io.DisplaySize.y - 200
        },
        ImGuiCond_FirstUseEver
    );

    ImGui::Begin("Canvas", nullptr, flags);

    ImVec2 panelSize = ImGui::GetContentRegionAvail();

    int cw = m_document->canvasSize().width;
    int ch = m_document->canvasSize().height;

    float canvasW = cw * m_zoom;
    float canvasH = ch * m_zoom;

    ImVec2 panelPos = ImGui::GetCursorScreenPos();

    float originX = panelPos.x + (panelSize.x - canvasW) * 0.5f + m_panX;
    float originY = panelPos.y + (panelSize.y - canvasH) * 0.5f + m_panY;

    auto& frame = m_document->frame(m_timeline->currentFrame());
    m_renderer->uploadFrame(frame);

    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        auto& prev = m_document->frame(m_timeline->currentFrame() - 1);
        m_renderer->uploadOnionFrame(prev, m_timeline->onionSkinOpacity());
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    drawCanvasBase(dl, originX, originY, canvasW, canvasH);

    dl->PushClipRect(
        {originX, originY},
        {originX + canvasW, originY + canvasH},
        true
    );

    drawFloatingPixels(dl, originX, originY, cw, ch);
    drawSelectionOverlays(dl, originX, originY, cw, ch);
    drawShapePreviews(dl, originX, originY, cw, ch);

    dl->PopClipRect();

    drawRubberBandSelectionPreview(dl, originX, originY);

    // ── Canvas input region ─────────────────────────────────────────────────
    ImVec2 canvasMin = {originX, originY};
    ImVec2 canvasMax = {originX + canvasW, originY + canvasH};

    ImGui::SetCursorScreenPos(canvasMin);

    ImGui::InvisibleButton(
        "##canvas_input_area",
        ImVec2(canvasW, canvasH),
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    bool canvasInputHovered = ImGui::IsItemHovered();
    bool canvasInputActive = ImGui::IsItemActive();

    ImVec2 winPos = ImGui::GetWindowPos();

    ImVec2 winSize = {
        ImGui::GetWindowWidth(),
        ImGui::GetWindowHeight()
    };

    ImVec2 mp = io.MousePos;

    bool canvasWindowHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

    bool rawInWindow =
        mp.x >= winPos.x &&
        mp.x <  winPos.x + winSize.x &&
        mp.y >= winPos.y &&
        mp.y <  winPos.y + winSize.y;

    bool rawInCanvas =
        mp.x >= canvasMin.x &&
        mp.x <  canvasMax.x &&
        mp.y >= canvasMin.y &&
        mp.y <  canvasMax.y;

    bool inWindow =
        rawInWindow &&
        (canvasWindowHovered || canvasInputActive || m_strokeActive);

    bool inCanvas =
        rawInCanvas &&
        (canvasInputHovered || canvasInputActive || m_strokeActive);

    bool spaceHeld = ImGui::IsKeyDown(ImGuiKey_Space);
    bool ctrlHeld = io.KeyCtrl;
    bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);

    bool imguiCapturing = false;

    bool drawingMouseDown =
        io.MouseDown[0] ||
        io.MouseDown[1];

    bool drawingMousePressed =
        io.MouseClicked[0] ||
        io.MouseClicked[1];

    drawBrushCursorPreview(
        dl,
        mp,
        originX,
        originY,
        cw,
        ch,
        inCanvas,
        spaceHeld,
        popupOpen
    );

    handleCursorBehavior(
        inWindow,
        inCanvas,
        canvasWindowHovered,
        canvasInputHovered,
        spaceHeld,
        popupOpen
    );

    handleKeyboardShortcuts(ctrlHeld, popupOpen);

    handleZoomInput(
        mp,
        panelPos,
        panelSize,
        inWindow,
        ctrlHeld,
        popupOpen
    );

    handlePanInput(
        inWindow,
        spaceHeld,
        popupOpen,
        imguiCapturing,
        drawingMouseDown
    );

    handleToolInput(
        mp,
        io.MouseDelta,
        canvasMin,
        canvasMax,
        originX,
        originY,
        cw,
        ch,
        canvasInputHovered,
        canvasWindowHovered,
        spaceHeld,
        popupOpen,
        imguiCapturing,
        drawingMouseDown,
        drawingMousePressed
    );

    // ── Zoom label ─────────────────────────────────────────────────────────
    dl->AddText(
        {panelPos.x + 4, panelPos.y + panelSize.y - 20},
        IM_COL32(150, 150, 150, 200),
        (std::to_string(static_cast<int>(m_zoom * 100)) + "%").c_str()
    );

    ImGui::End();
}

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

// ─────────────────────────────────────────────────────────────────────────────
// Floating pixels / overlays
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::drawFloatingPixels(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    (void)canvasW;
    (void)canvasH;

    if (!m_tab || !m_tab->hasFloating)
        return;

    int fw = m_tab->floatW;
    int fh = m_tab->floatH;
    int ox = m_tab->floatOffsetX;
    int oy = m_tab->floatOffsetY;

    for (int py = 0; py < fh; ++py) {
        for (int px = 0; px < fw; ++px) {
            size_t idx = static_cast<size_t>((py * fw + px) * 4);

            uint8_t b = m_tab->floatPixels[idx + 0];
            uint8_t g = m_tab->floatPixels[idx + 1];
            uint8_t r = m_tab->floatPixels[idx + 2];
            uint8_t a = m_tab->floatPixels[idx + 3];

            if (a == 0)
                continue;

            float sx0 = originX + (ox + px) * m_zoom;
            float sy0 = originY + (oy + py) * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                IM_COL32(r, g, b, a)
            );
        }
    }
}

void CanvasPanel::drawCanvasBase(
    ImDrawList* dl,
    float originX,
    float originY,
    float canvasW,
    float canvasH
) {
    ImTextureID checker = (ImTextureID)(intptr_t)m_renderer->checkerboardTexture();

    dl->AddImage(
        checker,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        ImTextureID onion = (ImTextureID)(intptr_t)m_renderer->onionTexture();

        uint8_t alpha = static_cast<uint8_t>(
            m_timeline->onionSkinOpacity() * 255.0f
        );

        ImU32 tint = IM_COL32(100, 180, 255, alpha);

        dl->AddImage(
            onion,
            {originX, originY},
            {originX + canvasW, originY + canvasH},
            {0, 0},
            {1, 1},
            tint
        );
    }

    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();

    dl->AddImage(
        tid,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

    dl->AddRect(
        {originX, originY},
        {originX + canvasW, originY + canvasH},
        IM_COL32(80, 80, 80, 255),
        0.0f,
        0,
        2.0f
    );
}

void CanvasPanel::drawRubberBandSelectionPreview(
    ImDrawList* dl,
    float originX,
    float originY
) {
    if (m_toolManager->activeToolType() != ToolType::Select)
        return;

    auto* selTool = static_cast<SelectionTool*>(m_toolManager->activeTool());

    if (!selTool || !selTool->isSelecting())
        return;

    int x0 = std::min(selTool->startX(), selTool->endX());
    int y0 = std::min(selTool->startY(), selTool->endY());
    int x1 = std::max(selTool->startX(), selTool->endX());
    int y1 = std::max(selTool->startY(), selTool->endY());

    float rx0 = originX + x0 * m_zoom;
    float ry0 = originY + y0 * m_zoom;
    float rx1 = originX + (x1 + 1) * m_zoom;
    float ry1 = originY + (y1 + 1) * m_zoom;

    dl->AddRectFilled(
        {rx0, ry0},
        {rx1, ry1},
        IM_COL32(44, 184, 213, 40)
    );

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
        IM_COL32(44, 184, 213, 220),
        0.0f,
        0,
        1.0f
    );
}

void CanvasPanel::drawDash(
    ImDrawList* dl,
    float x0,
    float y0,
    float x1,
    float y1,
    float& edgeLen,
    float dashOffset,
    ImU32 colorA,
    ImU32 colorB
) {
    float len = std::sqrt(
        (x1 - x0) * (x1 - x0) +
        (y1 - y0) * (y1 - y0)
    );

    if (len < 0.001f)
        return;

    float pos = std::fmod(edgeLen + dashOffset, 8.0f);
    float remaining = len;
    float start = 0.0f;

    while (remaining > 0.001f) {
        float segLen = std::min(
            remaining,
            4.0f - std::fmod(pos, 4.0f)
        );

        float t0 = start / len;
        float t1 = (start + segLen) / len;

        float lx0 = x0 + (x1 - x0) * t0;
        float ly0 = y0 + (y1 - y0) * t0;
        float lx1 = x0 + (x1 - x0) * t1;
        float ly1 = y0 + (y1 - y0) * t1;

        ImU32 col =
            (static_cast<int>(pos / 4.0f) % 2 == 0)
                ? colorA
                : colorB;

        dl->AddLine({lx0, ly0}, {lx1, ly1}, col, 1.0f);

        pos += segLen;
        start += segLen;
        remaining -= segLen;
    }

    edgeLen += len;
}

void CanvasPanel::drawSelectionOverlays(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    (void)canvasW;
    (void)canvasH;

    bool hasAnts =
        (m_tab && m_tab->hasFloating) ||
        (m_selection && !m_selection->isEmpty());

    if (!hasAnts)
        return;

    float t = static_cast<float>(ImGui::GetTime());
    float dashOffset = std::fmod(t * 6.0f, 8.0f);

    ImU32 antWhite = IM_COL32(255, 255, 255, 230);
    ImU32 antBlack = IM_COL32(0, 0, 0, 230);

    float edgeLen = 0.0f;

    if (m_tab && m_tab->hasFloating) {
        if (m_selection && !m_selection->isEmpty()) {
            int sw = m_selection->width();
            int sh = m_selection->height();

            int deltaX = m_tab->floatOffsetX - m_tab->floatStartX;
            int deltaY = m_tab->floatOffsetY - m_tab->floatStartY;

            auto isTranslatedSelected = [&](int movedX, int movedY) -> bool {
                int originalX = movedX - deltaX;
                int originalY = movedY - deltaY;

                if (originalX < 0 || originalY < 0 ||
                    originalX >= sw || originalY >= sh) {
                    return false;
                }

                return m_selection->isSelected(originalX, originalY);
            };

            for (int y = 0; y < sh; ++y) {
                for (int x = 0; x < sw; ++x) {
                    if (!m_selection->isSelected(x, y))
                        continue;

                    int tx = x + deltaX;
                    int ty = y + deltaY;

                    float cx0 = originX + tx * m_zoom;
                    float cy0 = originY + ty * m_zoom;
                    float cx1 = cx0 + m_zoom;
                    float cy1 = cy0 + m_zoom;

                    if (!isTranslatedSelected(tx, ty - 1)) {
                        drawDash(
                            dl,
                            cx0,
                            cy0,
                            cx1,
                            cy0,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx, ty + 1)) {
                        drawDash(
                            dl,
                            cx0,
                            cy1,
                            cx1,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx - 1, ty)) {
                        drawDash(
                            dl,
                            cx0,
                            cy0,
                            cx0,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx + 1, ty)) {
                        drawDash(
                            dl,
                            cx1,
                            cy0,
                            cx1,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }
                }
            }
        }
        else {
            int ox = m_tab->floatOffsetX;
            int oy = m_tab->floatOffsetY;
            int fw = m_tab->floatW;
            int fh = m_tab->floatH;

            float rx0 = originX + ox * m_zoom;
            float ry0 = originY + oy * m_zoom;
            float rx1 = originX + (ox + fw) * m_zoom;
            float ry1 = originY + (oy + fh) * m_zoom;

            drawDash(dl, rx0, ry0, rx1, ry0, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx1, ry0, rx1, ry1, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx1, ry1, rx0, ry1, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx0, ry1, rx0, ry0, edgeLen, dashOffset, antWhite, antBlack);
        }

        return;
    }

    if (!m_selection || m_selection->isEmpty())
        return;

    int sw = m_selection->width();
    int sh = m_selection->height();

    auto isSelected = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= sw || y >= sh)
            return false;

        return m_selection->isSelected(x, y);
    };

    for (int y = 0; y < sh; ++y) {
        for (int x = 0; x < sw; ++x) {
            if (!isSelected(x, y))
                continue;

            struct Edge {
                float x0;
                float y0;
                float x1;
                float y1;
            };

            Edge edges[4] = {
                {
                    originX + x * m_zoom,
                    originY + y * m_zoom,
                    originX + (x + 1) * m_zoom,
                    originY + y * m_zoom
                },
                {
                    originX + x * m_zoom,
                    originY + (y + 1) * m_zoom,
                    originX + (x + 1) * m_zoom,
                    originY + (y + 1) * m_zoom
                },
                {
                    originX + x * m_zoom,
                    originY + y * m_zoom,
                    originX + x * m_zoom,
                    originY + (y + 1) * m_zoom
                },
                {
                    originX + (x + 1) * m_zoom,
                    originY + y * m_zoom,
                    originX + (x + 1) * m_zoom,
                    originY + (y + 1) * m_zoom
                }
            };

            bool neighbors[4] = {
                isSelected(x, y - 1),
                isSelected(x, y + 1),
                isSelected(x - 1, y),
                isSelected(x + 1, y)
            };

            for (int ei = 0; ei < 4; ++ei) {
                if (neighbors[ei])
                    continue;

                const Edge& ed = edges[ei];

                drawDash(
                    dl,
                    ed.x0,
                    ed.y0,
                    ed.x1,
                    ed.y1,
                    edgeLen,
                    dashOffset,
                    antWhite,
                    antBlack
                );
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shape previews
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::drawShapePreviews(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    ImGuiIO& io = ImGui::GetIO();

    Color previewColor =
        (io.MouseDown[1] && !io.MouseDown[0])
            ? m_document->palette().secondaryColor()
            : m_document->palette().primaryColor();

    ImU32 previewFill = IM_COL32(
        previewColor.r,
        previewColor.g,
        previewColor.b,
        180
    );

    ToolType activeTool = m_toolManager->activeToolType();

    if (activeTool == ToolType::Line) {
        auto* lineTool = static_cast<LineTool*>(
            m_toolManager->getTool(ToolType::Line)
        );

        if (lineTool && lineTool->isDrawing()) {
            drawPreviewLinePixels(
                dl,
                lineTool->startX(),
                lineTool->startY(),
                lineTool->endX(),
                lineTool->endY(),
                m_toolManager->brushSize(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
    else if (activeTool == ToolType::Rectangle) {
        auto* rectTool = static_cast<RectangleTool*>(
            m_toolManager->getTool(ToolType::Rectangle)
        );

        if (rectTool && rectTool->isDrawing()) {
            drawPreviewRectPixels(
                dl,
                rectTool->startX(),
                rectTool->startY(),
                rectTool->endX(),
                rectTool->endY(),
                rectTool->filled(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
    else if (activeTool == ToolType::Ellipse) {
        auto* ellipseTool = static_cast<EllipseTool*>(
            m_toolManager->getTool(ToolType::Ellipse)
        );

        if (ellipseTool && ellipseTool->isDrawing()) {
            drawPreviewEllipsePixels(
                dl,
                ellipseTool->startX(),
                ellipseTool->startY(),
                ellipseTool->endX(),
                ellipseTool->endY(),
                m_toolManager->rectFilled(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
}

void CanvasPanel::drawPreviewBrushPixel(
    ImDrawList* dl,
    int x,
    int y,
    int brushSize,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int half = brushSize / 2;

    for (int by = -half; by < brushSize - half; ++by) {
        for (int bx = -half; bx < brushSize - half; ++bx) {
            int px = x + bx;
            int py = y + by;

            if (px < 0 || py < 0 || px >= canvasW || py >= canvasH)
                continue;

            float sx0 = originX + px * m_zoom;
            float sy0 = originY + py * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    }
}

void CanvasPanel::drawPreviewLinePixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    int brushSize,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        drawPreviewBrushPixel(
            dl,
            x0,
            y0,
            brushSize,
            color,
            originX,
            originY,
            canvasW,
            canvasH
        );

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

void CanvasPanel::drawPreviewRectPixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    bool filled,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int left   = std::min(x0, x1);
    int right  = std::max(x0, x1);
    int top    = std::min(y0, y1);
    int bottom = std::max(y0, y1);

    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            bool edge =
                x == left || x == right ||
                y == top  || y == bottom;

            if (!filled && !edge)
                continue;

            if (x < 0 || y < 0 || x >= canvasW || y >= canvasH)
                continue;

            float sx0 = originX + x * m_zoom;
            float sy0 = originY + y * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    }
}

void CanvasPanel::drawPreviewEllipsePixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    bool filled,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    ShapeRasterizer::rasterizeEllipse(
        x0,
        y0,
        x1,
        y1,
        filled,
        [&](int x, int y) {
            if (x < 0 || y < 0 || x >= canvasW || y >= canvasH)
                return;

            float sx0 = originX + x * m_zoom;
            float sy0 = originY + y * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    );
}
} // namespace Framenote