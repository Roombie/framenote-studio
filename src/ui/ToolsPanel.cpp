#include "ui/ToolsPanel.h"
#include <imgui.h>

namespace Framenote {

ToolsPanel::ToolsPanel(ToolManager* toolManager)
    : m_toolManager(toolManager)
{}

void ToolsPanel::render() {
    ImGui::SetNextWindowPos({ImGui::GetIO().DisplaySize.x - 60, 30},
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({50, 200}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Tools");

    struct ToolBtn { ToolType type; const char* icon; const char* tooltip; };
    static const ToolBtn buttons[] = {
        { ToolType::Pencil,     "✏",  "Pencil (B)"    },
        { ToolType::Eraser,     "⌫",  "Eraser (E)"    },
        { ToolType::Fill,       "🪣", "Fill (F)"      },
        { ToolType::Eyedropper, "💧", "Eyedropper (I)" },
    };

    for (auto& btn : buttons) {
        bool active = (m_toolManager->activeToolType() == btn.type);

        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));

        if (ImGui::Button(btn.icon, ImVec2(40, 40)))
            m_toolManager->selectTool(btn.type);

        if (active)
            ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", btn.tooltip);
    }

    ImGui::End();
}

} // namespace Framenote
