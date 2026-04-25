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
    : m_document(document)
    , m_timeline(timeline)
    , m_toolManager(toolManager)
    , m_renderer(renderer)
{}

void CanvasPanel::render() {
    ImGui::Begin("Canvas");

    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    int cw = m_document->canvasSize().width;
    int ch = m_document->canvasSize().height;
    float canvasW = cw * m_zoom;
    float canvasH = ch * m_zoom;

    ImVec2 panelPos = ImGui::GetCursorScreenPos();
    float originX = panelPos.x + m_panX + (panelSize.x - canvasW) * 0.5f;
    float originY = panelPos.y + m_panY + (panelSize.y - canvasH) * 0.5f;

    auto& currentFrame = m_document->frame(m_timeline->currentFrame());
    m_renderer->uploadFrame(currentFrame);

    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        auto& prevFrame = m_document->frame(m_timeline->currentFrame() - 1);
        m_renderer->uploadOnionFrame(prevFrame, m_timeline->onionSkinOpacity());
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float checkSize = m_zoom > 4.0f ? m_zoom : 4.0f;
    for (float y = originY; y < originY + canvasH; y += checkSize * 2)
        for (float x = originX; x < originX + canvasW; x += checkSize * 2) {
            drawList->AddRectFilled({x, y}, {x+checkSize, y+checkSize}, IM_COL32(180,180,180,255));
            drawList->AddRectFilled({x+checkSize, y+checkSize}, {x+checkSize*2, y+checkSize*2}, IM_COL32(180,180,180,255));
        }

    ImTextureID texId = (ImTextureID)(intptr_t)m_renderer->canvasTexture();
    drawList->AddImage(texId, {originX, originY}, {originX+canvasW, originY+canvasH});
    drawList->AddRect({originX, originY}, {originX+canvasW, originY+canvasH}, IM_COL32(80,80,80,255));

    handleInput(originX, originY, canvasW, canvasH);

    if (ImGui::IsWindowHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
            m_zoom = fnClamp(m_zoom + wheel * 0.5f, 1.0f, 32.0f);
    }

    ImGui::End();
}

void CanvasPanel::handleInput(float originX, float originY, float canvasW, float canvasH) {
    if (!ImGui::IsWindowHovered()) return;

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    float relX = (mousePos.x - originX) / canvasW;
    float relY = (mousePos.y - originY) / canvasH;

    if (relX < 0.0f || relX >= 1.0f || relY < 0.0f || relY >= 1.0f) return;

    int cx = static_cast<int>(relX * m_document->canvasSize().width);
    int cy = static_cast<int>(relY * m_document->canvasSize().height);

    ToolEvent e;
    e.canvasX   = static_cast<float>(cx);
    e.canvasY   = static_cast<float>(cy);
    e.leftDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    e.rightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    Tool* tool = m_toolManager->activeTool();
    if (!tool) return;

    int frameIdx = m_timeline->currentFrame();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        tool->onPress(*m_document, frameIdx, e);
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        tool->onDrag(*m_document, frameIdx, e);
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        tool->onRelease(*m_document, frameIdx, e);
}

} // namespace Framenote