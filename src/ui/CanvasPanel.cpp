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
} // namespace Framenote