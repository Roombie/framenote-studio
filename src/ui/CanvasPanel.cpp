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

void CanvasPanel::render() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

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

    // Draw checkerboard + canvas
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float cs = fnClamp(m_zoom, 4.f, 16.f);
    for (float y = originY; y < originY + canvasH; y += cs * 2)
        for (float x = originX; x < originX + canvasW; x += cs * 2) {
            dl->AddRectFilled({x,y},{x+cs,y+cs},IM_COL32(160,160,160,255));
            dl->AddRectFilled({x+cs,y+cs},{x+cs*2,y+cs*2},IM_COL32(160,160,160,255));
        }
    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();
    dl->AddImage(tid, {originX,originY}, {originX+canvasW,originY+canvasH});
    dl->AddRect({originX,originY},{originX+canvasW,originY+canvasH},
                IM_COL32(80,80,80,255), 0.f, 0, 2.f);

    ImVec2 mp = io.MousePos;
    bool winHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bool inCanvas = mp.x >= originX && mp.x < originX + canvasW &&
                    mp.y >= originY && mp.y < originY + canvasH;
    bool spaceHeld = ImGui::IsKeyDown(ImGuiKey_Space);

    bool inWindow = mp.x >= panelPos.x && mp.x < panelPos.x + panelSize.x &&
                mp.y >= panelPos.y && mp.y < panelPos.y + panelSize.y;

    // ── Zoom toward mouse with scroll wheel ───────────────────────────────────
    if (inWindow && io.MouseWheel != 0.f) {
        float oldZoom = m_zoom;
        float zoomStep = m_zoom < 4.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);
        if (io.MouseWheel > 0.f)
            m_zoom = fnClamp(m_zoom + zoomStep, 1.f, 32.f);
        else
            m_zoom = fnClamp(m_zoom - zoomStep, 1.f, 32.f);

        // Zoom toward mouse position
        float ratio = m_zoom / oldZoom;
        float mouseRelX = mp.x - (panelPos.x + panelSize.x * 0.5f + m_panX);
        float mouseRelY = mp.y - (panelPos.y + panelSize.y * 0.5f + m_panY);
        m_panX = mp.x - panelPos.x - panelSize.x * 0.5f - mouseRelX * ratio;
        m_panY = mp.y - panelPos.y - panelSize.y * 0.5f - mouseRelY * ratio;
    }

    // ── Pan with middle mouse or Space+left drag ───────────────────────────────
    bool isPanning = io.MouseDown[2] || (spaceHeld && io.MouseDown[0]);
    if (inWindow && isPanning) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }

    // ── Drawing — only when not panning and not holding space ─────────────────
    if (inCanvas && !spaceHeld && !io.MouseDown[2]) {
        float relX = (mp.x - originX) / canvasW;
        float relY = (mp.y - originY) / canvasH;
        int cx = (int)(relX * cw);
        int cy = (int)(relY * ch);

        ToolEvent e;
        e.canvasX   = (float)cx;
        e.canvasY   = (float)cy;
        e.leftDown  = io.MouseDown[0];
        e.rightDown = io.MouseDown[1];

        Tool* tool = m_toolManager->activeTool();
        if (tool) {
            int fi = m_timeline->currentFrame();
            if (io.MouseClicked[0])
                tool->onPress(*m_document, fi, e);
            else if (io.MouseDown[0])
                tool->onDrag(*m_document, fi, e);
            if (io.MouseReleased[0])
                tool->onRelease(*m_document, fi, e);
        }
    }

    // ── Cursor hint ───────────────────────────────────────────────────────────
    if (winHovered && spaceHeld)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    ImGui::End();
}

void CanvasPanel::handleInput(float, float, float, float) {}

} // namespace Framenote
