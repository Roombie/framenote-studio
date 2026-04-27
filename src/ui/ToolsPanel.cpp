#include "ui/ToolsPanel.h"
#include <imgui.h>

namespace Framenote {

void ToolsPanel::render() {
    ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - 60, 62},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({50, 220}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Tools", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

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
            // Use the pixel art icon as the button image
            ImTextureID tid = (ImTextureID)(intptr_t)btn.icon;
            if (ImGui::ImageButton(btn.fallback, tid, btnSize))
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

    ImGui::End();
}

} // namespace Framenote