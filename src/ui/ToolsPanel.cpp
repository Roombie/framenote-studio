#include "ui/ToolsPanel.h"
#include "ui/Theme.h"

#include <imgui.h>

namespace Framenote {

void ToolsPanel::render() {
    ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoScrollbar);

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
        { ToolType::Line,       m_icons ? m_icons->line       : nullptr, "L", "Line (L)"       },
        { ToolType::Rectangle,  m_icons ? m_icons->rectangle  : nullptr, "R", "Rectangle (R)"  },
        { ToolType::Ellipse,    m_icons ? m_icons->ellipse    : nullptr, "O", "Ellipse (O)"    },
        { ToolType::Select,     m_icons ? m_icons->select     : nullptr, "S", "Select (M)"     },
        { ToolType::Move,       m_icons ? m_icons->move       : nullptr, "V", "Move (V)"       },
    };

    ImVec2 btnSize = {36, 36};

    for (auto& btn : buttons) {
        bool active = (m_toolManager->activeToolType() == btn.type);

        if (active) {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImVec4(0.17f, 0.72f, 0.84f, 0.35f)
            );
        }

        bool clicked = false;

        if (btn.icon) {
            ImTextureID tid = (ImTextureID)(intptr_t)btn.icon;

            ImVec4 tint = (Theme::current() == ThemeMode::Light)
                ? ImVec4(0.15f, 0.15f, 0.15f, 1.0f)
                : ImVec4(1.0f,  1.0f,  1.0f,  1.0f);

            clicked = ImGui::ImageButton(
                btn.fallback,
                tid,
                btnSize,
                {0, 0},
                {1, 1},
                {0, 0, 0, 0},
                tint
            );
        }
        else {
            // Fallback text button if icon failed to load.
            clicked = ImGui::Button(btn.fallback, btnSize);
        }

        // ImGui buttons only return true for left-click by default.
        // Allow right-click to select tools too, since right-click should only
        // behave as secondary color inside the canvas, not on UI buttons.
        bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);

        if (clicked || rightClicked) {
            m_toolManager->selectTool(btn.type);
        }

        if (active) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\nLeft-click or right-click to select", btn.tooltip);
        }
    }

    // Only show brush size for tools that use it.
    ToolType active = m_toolManager->activeToolType();
    bool showBrush = (
        active == ToolType::Pencil ||
        active == ToolType::Eraser ||
        active == ToolType::Line
    );

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

    // Rectangle and Ellipse share the filled toggle
    if (active == ToolType::Rectangle || active == ToolType::Ellipse) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        bool filled = m_toolManager->rectFilled();
        if (ImGui::Checkbox("Filled##rect", &filled))
            m_toolManager->setRectFilled(filled);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Hold Shift to toggle while drawing");
    }

    ImGui::End();
}

} // namespace Framenote