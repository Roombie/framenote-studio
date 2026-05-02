#pragma once

#include <imgui.h>

#include "core/Document.h"
#include "core/Timeline.h"
#include "core/History.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"
#include "tools/ToolManager.h"
#include "renderer/CanvasRenderer.h"

namespace Framenote {

class CanvasPanel {
public:
    CanvasPanel(
        Document*       document,
        Timeline*       timeline,
        ToolManager*    toolManager,
        CanvasRenderer* renderer,
        float&          zoom,
        float&          panX,
        float&          panY,
        History&        history,
        bool&           strokeActive,
        int&            strokeFrameIndex,
        Selection*      selection = nullptr,
        DocumentTab*    tab       = nullptr
    );

    void render();

private:
    // ── Canvas base rendering ─────────────────────────────────────────────────
    void drawCanvasBase(
        ImDrawList* dl,
        float originX,
        float originY,
        float canvasW,
        float canvasH
    );

    void drawRubberBandSelectionPreview(
        ImDrawList* dl,
        float originX,
        float originY
    );

    // ── Editor actions ─────────────────────────────────────────────────────
    void commitFloatIfNeeded();
    void commitSelectionFloatIfNeeded();
    void pushCurrentFrameSnapshot();
    void clearFloatingState();
    void deleteSelectionOrFrame();

    ToolEvent makeToolEvent(
        const ImVec2& mousePos,
        float originX,
        float originY,
        int canvasW,
        int canvasH,
        bool clampCoords
    );

    // ── Input handling ─────────────────────────────────────────────────────
    void handleKeyboardShortcuts(bool ctrlHeld, bool popupOpen);

    void handleZoomInput(
        const ImVec2& mousePos,
        const ImVec2& panelPos,
        const ImVec2& panelSize,
        bool inWindow,
        bool ctrlHeld,
        bool popupOpen
    );

    void handlePanInput(
        bool inWindow,
        bool spaceHeld,
        bool popupOpen,
        bool imguiCapturing,
        bool drawingMouseDown
    );

    void handleToolInput(
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
    );

    // ── Cursor / preview helpers ───────────────────────────────────────────
    void drawBrushCursorPreview(
        ImDrawList* dl,
        const ImVec2& mousePos,
        float originX,
        float originY,
        int canvasW,
        int canvasH,
        bool inCanvas,
        bool spaceHeld,
        bool popupOpen
    );

    void handleCursorBehavior(
        bool inWindow,
        bool inCanvas,
        bool canvasWindowHovered,
        bool canvasInputHovered,
        bool spaceHeld,
        bool popupOpen
    );

    // ── Floating selection / floating canvas / marching ants ───────────────
    void drawFloatingPixels(
        ImDrawList* dl,
        float originX,
        float originY,
        int canvasW,
        int canvasH
    );

    void drawSelectionOverlays(
        ImDrawList* dl,
        float originX,
        float originY,
        int canvasW,
        int canvasH
    );

    void drawDash(
        ImDrawList* dl,
        float x0,
        float y0,
        float x1,
        float y1,
        float& edgeLen,
        float dashOffset,
        ImU32 colorA,
        ImU32 colorB
    );

    // ── Drawing tool previews ──────────────────────────────────────────────
    void drawShapePreviews(
        ImDrawList* dl,
        float originX,
        float originY,
        int canvasW,
        int canvasH
    );

    void drawPreviewBrushPixel(
        ImDrawList* dl,
        int x,
        int y,
        int brushSize,
        ImU32 color,
        float originX,
        float originY,
        int canvasW,
        int canvasH
    );

    void drawPreviewLinePixels(
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
    );

    void drawPreviewRectPixels(
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
    );

    void drawPreviewEllipsePixels(
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
    );

private:
    Document*       m_document;
    Timeline*       m_timeline;
    ToolManager*    m_toolManager;
    CanvasRenderer* m_renderer;

    float&   m_zoom;
    float&   m_panX;
    float&   m_panY;
    History& m_history;

    bool& m_strokeActive;
    int&  m_strokeFrameIndex;

    Selection*   m_selection = nullptr;
    DocumentTab* m_tab       = nullptr;
};

} // namespace Framenote