#include "ui/CanvasPanel.h"
#include <imgui.h>
#include <SDL3/SDL.h>

namespace Framenote {

static float fnClamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

CanvasPanel::CanvasPanel(Document* document, Timeline* timeline,
                         ToolManager* toolManager, CanvasRenderer* renderer)
    : m_document(document), m_timeline(timeline)
    , m_toolManager(toolManager), m_renderer(renderer)
{}

// Helper: get current frame as snapshot
static Snapshot currentSnapshot(Document& doc, int frameIndex) {
    auto& frame = doc.frame(frameIndex);
    Snapshot snap;
    snap.frameIndex   = frameIndex;
    snap.bufferWidth  = frame.bufferWidth();
    snap.bufferHeight = frame.bufferHeight();
    snap.pixels       = frame.pixels();
    return snap;
}

// Helper: apply a snapshot back to the document
static void applySnapshot(Document& doc, const Snapshot& snap) {
    auto& frame = doc.frame(snap.frameIndex);
    frame.pixels() = snap.pixels;
    doc.markDirty();
}

void CanvasPanel::render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoMove;
                         
    ImGui::SetNextWindowPos({180, 30}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({
        io.DisplaySize.x - 240,
        io.DisplaySize.y - 200
    }, ImGuiCond_FirstUseEver);
    ImGui::Begin("Canvas", nullptr, flags);

    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    int cw = m_document->canvasSize().width;
    int ch = m_document->canvasSize().height;
    float canvasW = cw * m_zoom;
    float canvasH = ch * m_zoom;

    ImVec2 panelPos = ImGui::GetCursorScreenPos();
    float originX = panelPos.x + (panelSize.x - canvasW) * 0.5f + m_panX;
    float originY = panelPos.y + (panelSize.y - canvasH) * 0.5f + m_panY;

    // Upload frame
    auto& frame = m_document->frame(m_timeline->currentFrame());
    m_renderer->uploadFrame(frame);
    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        auto& prev = m_document->frame(m_timeline->currentFrame() - 1);
        m_renderer->uploadOnionFrame(prev, m_timeline->onionSkinOpacity());
    }

    // Draw checkerboard
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float cs = fnClamp(m_zoom, 4.f, 16.f);
    for (float y = originY; y < originY + canvasH; y += cs * 2)
        for (float x = originX; x < originX + canvasW; x += cs * 2) {
            dl->AddRectFilled({x,y},{x+cs,y+cs},IM_COL32(160,160,160,255));
            dl->AddRectFilled({x+cs,y+cs},{x+cs*2,y+cs*2},IM_COL32(160,160,160,255));
        }

    // Canvas texture + border
    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();
    dl->AddImage(tid, {originX,originY}, {originX+canvasW,originY+canvasH});
    dl->AddRect({originX,originY},{originX+canvasW,originY+canvasH},
                IM_COL32(80,80,80,255), 0.f, 0, 2.f);

    // Window bounds
    ImVec2 winMin = ImGui::GetWindowPos();
    ImVec2 winMax = {winMin.x + ImGui::GetWindowWidth(),
                     winMin.y + ImGui::GetWindowHeight()};
    ImVec2 mp = io.MousePos;
    bool inWindow = mp.x >= winMin.x && mp.x < winMax.x &&
                    mp.y >= winMin.y && mp.y < winMax.y;
    bool inCanvas = mp.x >= originX && mp.x < originX + canvasW &&
                    mp.y >= originY && mp.y < originY + canvasH;
    bool spaceHeld = ImGui::IsKeyDown(ImGuiKey_Space);
    bool ctrlHeld  = io.KeyCtrl;

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        History& hist = m_toolManager->history();
        if (hist.canUndo()) {
            int fi = m_timeline->currentFrame();
            Snapshot restored = hist.undo(currentSnapshot(*m_document, fi));
            applySnapshot(*m_document, restored);
        }
    }
    if (ctrlHeld && (ImGui::IsKeyPressed(ImGuiKey_Y, false) ||
                    (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false)))) {
        History& hist = m_toolManager->history();
        if (hist.canRedo()) {
            int fi = m_timeline->currentFrame();
            Snapshot restored = hist.redo(currentSnapshot(*m_document, fi));
            applySnapshot(*m_document, restored);
        }
    }

    // ── Zoom ──────────────────────────────────────────────────────────────────
    if (inWindow && io.MouseWheel != 0.f) {
        float oldZoom = m_zoom;
        float zoomStep = m_zoom < 2.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);
        m_zoom = fnClamp(
            io.MouseWheel > 0.f ? m_zoom + zoomStep : m_zoom - zoomStep,
            0.25f, 64.f);
        float ratio = m_zoom / oldZoom;
        float cx = panelPos.x + panelSize.x * 0.5f;
        float cy = panelPos.y + panelSize.y * 0.5f;
        m_panX = (m_panX + mp.x - cx) * ratio - (mp.x - cx);
        m_panY = (m_panY + mp.y - cy) * ratio - (mp.y - cy);
        io.MouseWheel = 0.f;
    }

    // ── Pan ───────────────────────────────────────────────────────────────────
    // Middle mouse always pans, Space+left only pans when not already drawing
    bool isPanning = io.MouseDown[2] || (spaceHeld && io.MouseDown[0]);
    bool isDrawing = inCanvas && !spaceHeld && !io.MouseDown[2] && io.MouseDown[0];
    if (inWindow && isPanning && !isDrawing) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }

    // ── Drawing ───────────────────────────────────────────────────────────────
    if (inCanvas && !spaceHeld && !io.MouseDown[2]) {
        float relX = (mp.x - originX) / canvasW;
        float relY = (mp.y - originY) / canvasH;
        int px = (int)(relX * cw);
        int py = (int)(relY * ch);

        ToolEvent e;
        e.canvasX   = (float)px;
        e.canvasY   = (float)py;
        e.leftDown  = io.MouseDown[0];
        e.rightDown = io.MouseDown[1];

        Tool* tool = m_toolManager->activeTool();
        if (tool) {
            int fi = m_timeline->currentFrame();
            if (io.MouseClicked[0]) {
                // Snapshot BEFORE drawing starts
                m_toolManager->snapshotBefore(*m_document, fi);
                tool->onPress(*m_document, fi, e);
            } else if (io.MouseDown[0]) {
                tool->onDrag(*m_document, fi, e);
            }
            if (io.MouseReleased[0])
                tool->onRelease(*m_document, fi, e);
        }
    }

    if (spaceHeld && inWindow)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    ImGui::End();
}

void CanvasPanel::handleInput(float, float, float, float) {}

} // namespace Framenote
