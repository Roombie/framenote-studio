#include "ui/TimelinePanel.h"
#include <imgui.h>

namespace Framenote {

TimelinePanel::TimelinePanel(Document* document, Timeline* timeline)
    : m_document(document), m_timeline(timeline)
{}

void TimelinePanel::render() {
    ImGui::SetNextWindowPos({4, ImGui::GetIO().DisplaySize.y - 160}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x - 8, 155}, ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Timeline", nullptr, flags);

    renderPlaybackControls();
    ImGui::Separator();
    renderFrameStrip();

    ImGui::End();
}

void TimelinePanel::renderPlaybackControls() {
    // ── Play / Pause ──────────────────────────────────────────────────────────
    if (m_timeline->isPlaying()) {
        if (ImGui::Button("⏸ Pause")) m_timeline->pause();
    } else {
        if (ImGui::Button("▶ Play"))  m_timeline->play();
    }

    ImGui::SameLine();
    if (ImGui::Button("⏹ Stop"))      m_timeline->stop();

    ImGui::SameLine();
    if (ImGui::Button("|◀"))          m_timeline->setCurrentFrame(0);

    ImGui::SameLine();
    if (ImGui::Button("◀"))           m_timeline->prevFrame();

    ImGui::SameLine();
    if (ImGui::Button("▶##next"))     m_timeline->nextFrame();

    // ── FPS slider ────────────────────────────────────────────────────────────
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    int fps = m_timeline->fps();
    if (ImGui::SliderInt("FPS", &fps, 1, 30))
        m_timeline->setFps(fps);

    // ── Onion skin toggle ─────────────────────────────────────────────────────
    ImGui::SameLine();
    bool onion = m_timeline->onionSkinEnabled();
    if (ImGui::Checkbox("Onion Skin", &onion))
        m_timeline->setOnionSkin(onion);

    // ── Loop toggle ───────────────────────────────────────────────────────────
    ImGui::SameLine();
    bool loop = m_timeline->looping();
    if (ImGui::Checkbox("Loop", &loop))
        m_timeline->setLooping(loop);
}

void TimelinePanel::renderFrameStrip() {
    ImGui::BeginChild("##FrameStrip", ImVec2(0, THUMB_SIZE + 24), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    int frameCount = m_document->frameCount();
    int current    = m_timeline->currentFrame();

    for (int i = 0; i < frameCount; ++i) {
        ImGui::PushID(i);

        // Highlight current frame
        if (i == current) {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
        }

        // Frame button (no thumbnail yet — shows frame number)
        char label[8];
        snprintf(label, sizeof(label), "%d", i + 1);
        if (ImGui::Button(label, ImVec2(THUMB_SIZE, THUMB_SIZE)))
            m_timeline->setCurrentFrame(i);

        if (i == current)
            ImGui::PopStyleColor(2);

        // Right-click context menu per frame
        if (ImGui::BeginPopupContextItem("##FrameCtx")) {
            if (ImGui::MenuItem("Duplicate")) m_document->duplicateFrame(i);
            if (ImGui::MenuItem("Delete") && frameCount > 1) {
                m_document->deleteFrame(i);
                m_timeline->setFrameCount(m_document->frameCount());
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::PopID();
    }

    // ── Add frame button ──────────────────────────────────────────────────────
    if (ImGui::Button("+##AddFrame", ImVec2(THUMB_SIZE, THUMB_SIZE))) {
        int newIdx = m_document->addFrame();
        m_timeline->setFrameCount(m_document->frameCount());
        m_timeline->setCurrentFrame(newIdx);
    }

    ImGui::EndChild();
}

} // namespace Framenote
