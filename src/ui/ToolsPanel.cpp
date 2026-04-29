#include "ui/ToolsPanel.h"
#include "ui/Theme.h"
#include <imgui.h>

namespace Framenote {

void ToolsPanel::render() {
    ImGui::Begin("Tools", nullptr,
        ImGuiWindowFlags_NoScrollbar);

    struct ToolBtn {
        ToolType      type;
        SDL_Texture*  icon;
        const char*   fallback;
        const char*   tooltip;
    };

    ToolBtn buttons[] = {
        { ToolType::Pencil,     m_icons ? m_icons->pencil     : nullptr, "P", "Pencil (B)"     },
        { ToolType::Eraser,     m_icons ? m_icons->eraser     : nullptr, "E", "Eraser (E)"     },
        { ToolType::Fill,       m_icons ? m_icons->fill       : nullptr, "F", "Fill (F)"       },
        { ToolType::Eyedropper, m_icons ? m_icons->eyedropper : nullptr, "I", "Eyedropper (I)" },
    };

    for (auto& btn : buttons) {
        bool active = (m_toolManager->activeToolType() == btn.type);

        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImVec4(0.17f, 0.72f, 0.84f, 0.35f));

        ImVec2 btnSize = {36, 36};

        if (btn.icon) {
            ImTextureID tid = (ImTextureID)(intptr_t)btn.icon;
            ImVec4 tint = (Theme::current() == ThemeMode::Light)
                ? ImVec4(0.15f, 0.15f, 0.15f, 1.0f)
                : ImVec4(1.0f,  1.0f,  1.0f,  1.0f);
            if (ImGui::ImageButton(btn.fallback, tid, btnSize,
                    {0,0}, {1,1}, {0,0,0,0}, tint))
                m_toolManager->selectTool(btn.type);
        } else {
            // Fallback text button if icon failed to load
            if (ImGui::Button(btn.fallback, btnSize))
                m_toolManager->selectTool(btn.type);
        }

        if (active)
            ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", btn.tooltip);
    }

    // Only show brush size for tools that use it
    ToolType active = m_toolManager->activeToolType();
    bool showBrush  = (active == ToolType::Pencil || active == ToolType::Eraser);

    if (showBrush) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        int bsize = m_toolManager->brushSize();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderInt("##bsize", &bsize, 1, 32, "%d px"))
            m_toolManager->setBrushSize(bsize);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Brush size  ( [ / ] )");
    }

    ImGui::End();
}

} // namespace Framenote