#include "ui/TimelinePanel.h"
#include <imgui.h>
#include "ui/Theme.h"

namespace Framenote {

TimelinePanel::TimelinePanel(Document* document, Timeline* timeline,
                             ToolIcons* icons)
    : m_document(document), m_timeline(timeline), m_icons(icons)
{}

bool TimelinePanel::iconButton(const char* id, SDL_Texture* icon,
                               const char* fallback, ImVec2 size,
                               const char* tooltip) {
    bool clicked = false;
    if (icon) {
        ImTextureID tid = (ImTextureID)(intptr_t)icon;
        ImVec4 tint = (Theme::current() == ThemeMode::Light)
            ? ImVec4(0.15f, 0.15f, 0.15f, 1.0f)
            : ImVec4(1.0f,  1.0f,  1.0f,  1.0f);
        clicked = ImGui::ImageButton(id, tid, size, {0,0}, {1,1}, {0,0,0,0}, tint);
    } else {
        clicked = ImGui::Button(fallback, size);
    }
    if (tooltip && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

void TimelinePanel::render() {
    ImGui::SetNextWindowPos({4, ImGui::GetIO().DisplaySize.y - 160},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x - 8, 155},
                              ImGuiCond_FirstUseEver);
    ImGui::Begin("Timeline", nullptr, ImGuiWindowFlags_NoScrollbar);

    renderPlaybackControls();
    ImGui::Separator();
    renderFrameStrip();

    ImGui::End();
}

void TimelinePanel::renderPlaybackControls() {
    ImVec2 btnSize = {28, 28};
    bool   playing = m_timeline->isPlaying();
    int    current = m_timeline->currentFrame();
    int    total   = m_timeline->frameCount();

    // |< First frame — auto-pauses if playing
    ImGui::BeginDisabled(current == 0 && !playing);
    if (iconButton("##first", m_icons ? m_icons->firstFrame : nullptr,
                   "|<", btnSize, "First frame  (Shift+Left)")) {
        if (playing) m_timeline->pause();
        m_timeline->setCurrentFrame(0);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // < Previous frame — auto-pauses if playing
    ImGui::BeginDisabled(current == 0 && !playing);
    if (iconButton("##prev", m_icons ? m_icons->prevFrame : nullptr,
                   "<", btnSize, "Previous frame  (Left arrow)")) {
        if (playing) m_timeline->pause();
        m_timeline->prevFrame();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Play / Pause toggle
    if (playing) {
        if (iconButton("##pause", m_icons ? m_icons->pause : nullptr,
                       "||", btnSize, "Pause  (Space)"))
            m_timeline->pause();
    } else {
        if (iconButton("##play", m_icons ? m_icons->play : nullptr,
                       ">", btnSize, "Play  (Space)"))
            m_timeline->play();
    }

    ImGui::SameLine();

    // > Next frame — auto-pauses if playing
    ImGui::BeginDisabled(current == total - 1 && !playing);
    if (iconButton("##next", m_icons ? m_icons->nextFrame : nullptr,
                   ">", btnSize, "Next frame  (Right arrow)")) {
        if (playing) m_timeline->pause();
        m_timeline->nextFrame();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    // >| Last frame — auto-pauses if playing
    ImGui::BeginDisabled(current == total - 1 && !playing);
    if (iconButton("##last", m_icons ? m_icons->lastFrame : nullptr,
                   ">|", btnSize, "Last frame  (Shift+Right)")) {
        if (playing) m_timeline->pause();
        m_timeline->setCurrentFrame(total - 1);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

    // FPS slider
    ImGui::SetNextItemWidth(90.0f);
    int fps = m_timeline->fps();
    if (ImGui::SliderInt("FPS", &fps, 1, 60))
        m_timeline->setFps(fps);

    ImGui::SameLine();

    // Onion skin
    bool onion = m_timeline->onionSkinEnabled();
    if (ImGui::Checkbox("Onion Skin", &onion))
        m_timeline->setOnionSkin(onion);

    ImGui::SameLine();

    // Loop
    bool loop = m_timeline->looping();
    if (ImGui::Checkbox("Loop", &loop))
        m_timeline->setLooping(loop);

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    ImGui::TextDisabled("%d / %d", current + 1, total);
}

void TimelinePanel::renderFrameStrip() {
    ImGui::BeginChild("##FrameStrip", ImVec2(0, THUMB_SIZE + 24), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    int frameCount = m_document->frameCount();
    int current    = m_timeline->currentFrame();

    for (int i = 0; i < frameCount; ++i) {
        ImGui::PushID(i);

        if (i == current) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImVec4(0.17f, 0.72f, 0.84f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(0.25f, 0.80f, 0.92f, 1.0f));
        }

        char label[8];
        snprintf(label, sizeof(label), "%d", i + 1);
        if (ImGui::Button(label, ImVec2(THUMB_SIZE, THUMB_SIZE)))
            m_timeline->setCurrentFrame(i);

        if (i == current)
            ImGui::PopStyleColor(2);

        if (ImGui::BeginPopupContextItem("##FrameCtx")) {
            if (ImGui::MenuItem("Duplicate"))
                m_document->duplicateFrame(i);
            if (ImGui::MenuItem("Delete") && frameCount > 1) {
                m_document->deleteFrame(i);
                m_timeline->setFrameCount(m_document->frameCount());
                if (m_timeline->currentFrame() >= m_document->frameCount())
                    m_timeline->setCurrentFrame(m_document->frameCount() - 1);
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::PopID();
    }

    if (ImGui::Button("+##AddFrame", ImVec2(THUMB_SIZE, THUMB_SIZE))) {
        int newIdx = m_document->addFrame();
        m_timeline->setFrameCount(m_document->frameCount());
        m_timeline->setCurrentFrame(newIdx);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add frame");

    ImGui::EndChild();
}

} // namespace Framenote