#include "ui/CanvasPanel.h"
#include <imgui.h>
#include <string>
#include <SDL3/SDL.h>

namespace Framenote {

static float fnClamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

CanvasPanel::CanvasPanel(Document* document, Timeline* timeline,
                         ToolManager* toolManager, CanvasRenderer* renderer,
                         float& zoom, float& panX, float& panY,
                         History& history)
    : m_document(document), m_timeline(timeline)
    , m_toolManager(toolManager), m_renderer(renderer)
    , m_zoom(zoom), m_panX(panX), m_panY(panY)
    , m_history(history)
{}

static Snapshot currentSnapshot(Document& doc, int frameIndex) {
    auto& frame = doc.frame(frameIndex);
    Snapshot snap;
    snap.frameIndex   = frameIndex;
    snap.bufferWidth  = frame.bufferWidth();
    snap.bufferHeight = frame.bufferHeight();
    snap.pixels       = frame.pixels();
    return snap;
}

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
    ImGui::SetNextWindowPos({180, 62}, ImGuiCond_FirstUseEver);
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

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Draw pre-rendered checkerboard texture (transparency grid).
    // Built once in CanvasRenderer and reused every frame — no per-pixel loops.
    ImTextureID checker = (ImTextureID)(intptr_t)m_renderer->checkerboardTexture();
    dl->AddImage(checker, {originX,originY}, {originX+canvasW,originY+canvasH});

    // Draw onion skin (previous frame ghost) under the current frame.
    // ImGui's AddImage ignores SDL texture alpha/color mods, so we pass
    // the opacity and blue tint directly as the tint color parameter.
    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        ImTextureID onion = (ImTextureID)(intptr_t)m_renderer->onionTexture();
        uint8_t alpha = static_cast<uint8_t>(m_timeline->onionSkinOpacity() * 255.0f);
        ImU32 tint = IM_COL32(100, 180, 255, alpha); // blue-ish ghost
        dl->AddImage(onion, {originX,originY}, {originX+canvasW,originY+canvasH},
                     {0,0}, {1,1}, tint);
    }

    // Draw current frame canvas texture on top
    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();
    dl->AddImage(tid, {originX,originY}, {originX+canvasW,originY+canvasH});

    // Canvas border
    dl->AddRect({originX,originY},{originX+canvasW,originY+canvasH},
                IM_COL32(80,80,80,255), 0.f, 0, 2.f);

    // Input state
    ImVec2 winPos  = ImGui::GetWindowPos();
    ImVec2 winSize = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};
    ImVec2 mp      = io.MousePos;

    bool inWindow  = mp.x >= winPos.x && mp.x < winPos.x + winSize.x &&
                     mp.y >= winPos.y && mp.y < winPos.y + winSize.y;
    bool inCanvas  = mp.x >= originX && mp.x < originX + canvasW &&
                     mp.y >= originY && mp.y < originY + canvasH;
    bool spaceHeld = ImGui::IsKeyDown(ImGuiKey_Space);
    bool ctrlHeld  = io.KeyCtrl;
    bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);

    bool imguiCapturing = false;

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    if (!ctrlHeld && !io.WantTextInput && !popupOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_B, false))
            m_toolManager->selectTool(ToolType::Pencil);
        if (ImGui::IsKeyPressed(ImGuiKey_E, false))
            m_toolManager->selectTool(ToolType::Eraser);
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
            m_toolManager->selectTool(ToolType::Fill);
        if (ImGui::IsKeyPressed(ImGuiKey_I, false))
            m_toolManager->selectTool(ToolType::Eyedropper);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
            m_timeline->nextFrame();
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
            m_timeline->prevFrame();
    }

    // ── Zoom with + / - keys ─────────────────────────────────────────────────
    if (inWindow && !io.WantTextInput && !popupOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom < 2.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);
            m_zoom = fnClamp(m_zoom + step, 0.25f, 64.f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom <= 2.f ? 0.25f : (m_zoom <= 8.f ? 0.5f : 1.f);
            m_zoom = fnClamp(m_zoom - step, 0.25f, 64.f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }
        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_0, false)) {
            m_zoom = 1.0f;
            m_panX = 0.f;
            m_panY = 0.f;
        }
    }

    // ── Zoom with scroll wheel ────────────────────────────────────────────────
    if (inWindow && !popupOpen && io.MouseWheel != 0.f) {
        float step = m_zoom < 2.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);
        float oldZoom = m_zoom;
        m_zoom = fnClamp(
            io.MouseWheel > 0.f ? m_zoom + step : m_zoom - step,
            0.25f, 64.f);
        float ratio = m_zoom / oldZoom;
        m_panX = (m_panX - (mp.x - (panelPos.x + panelSize.x * 0.5f))) * ratio
                 + (mp.x - (panelPos.x + panelSize.x * 0.5f));
        m_panY = (m_panY - (mp.y - (panelPos.y + panelSize.y * 0.5f))) * ratio
                 + (mp.y - (panelPos.y + panelSize.y * 0.5f));
        io.MouseWheel = 0.f;
    }

    // ── Undo / Redo ───────────────────────────────────────────────────────────
    if (ctrlHeld && !popupOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            History& hist = m_history;
            if (hist.canUndo()) {
                int fi = m_timeline->currentFrame();
                Snapshot restored = hist.undo(currentSnapshot(*m_document, fi));
                applySnapshot(*m_document, restored);
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y, false) ||
            (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z, false))) {
            History& hist = m_history;
            if (hist.canRedo()) {
                int fi = m_timeline->currentFrame();
                Snapshot restored = hist.redo(currentSnapshot(*m_document, fi));
                applySnapshot(*m_document, restored);
            }
        }
    }

    // ── Pan ───────────────────────────────────────────────────────────────────
    bool isPanning = io.MouseDown[2] || (spaceHeld && io.MouseDown[0]);
    bool isDrawing = inCanvas && !spaceHeld && !io.MouseDown[2] && io.MouseDown[0];
    if (inWindow && isPanning && !isDrawing && !popupOpen && !imguiCapturing) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }

    // ── Drawing ───────────────────────────────────────────────────────────────
    if (inCanvas && !spaceHeld && !io.MouseDown[2] && !popupOpen && !imguiCapturing) {
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
                // Snapshot current frame state before drawing for undo
                {
                    auto& f = m_document->frame(fi);
                    Snapshot snap;
                    snap.frameIndex   = fi;
                    snap.bufferWidth  = f.bufferWidth();
                    snap.bufferHeight = f.bufferHeight();
                    snap.pixels       = f.pixels();
                    m_history.push(std::move(snap));
                }
                tool->onPress(*m_document, fi, e);
            } else if (io.MouseDown[0]) {
                tool->onDrag(*m_document, fi, e);
            }
            if (io.MouseReleased[0])
                tool->onRelease(*m_document, fi, e);
        }
    }

    // Cursor hint
    if (spaceHeld && inWindow)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

     // Bottom-left overlay: zoom level + canvas coordinates
    std::string overlay = std::to_string((int)(m_zoom * 100)) + "%";
    
    int px = (int)((mp.x - originX) / canvasW * cw);
    int py = (int)((mp.y - originY) / canvasH * ch);
    overlay += "   X: " + std::to_string(px) + "  Y: " + std::to_string(py);

    dl->AddText({panelPos.x + 4, panelPos.y + panelSize.y - 20},
                IM_COL32(150,150,150,200), overlay.c_str());
 
    ImGui::End();
}

void CanvasPanel::handleInput(float, float, float, float) {}

} // namespace Framenote