#include "ui/PalettePanel.h"
#include <imgui.h>

namespace Framenote {

PalettePanel::PalettePanel(Document* document)
    : m_document(document)
{}

void PalettePanel::render() {
    ImGui::Begin("Palette");

    Palette& palette = m_document->palette();
    int selected = palette.selectedIndex();

    // ── Selected color preview ────────────────────────────────────────────────
    Color c = palette.selectedColor();
    ImVec4 previewColor = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
    ImGui::ColorButton("##SelectedColor", previewColor,
                       ImGuiColorEditFlags_NoTooltip, ImVec2(40, 40));
    ImGui::SameLine();
    ImGui::Text("Color %d\n#%02X%02X%02X", selected, c.r, c.g, c.b);

    ImGui::Separator();

    // ── Swatch grid ───────────────────────────────────────────────────────────
    for (int i = 0; i < Palette::size(); ++i) {
        Color col = palette.color(i);

        ImVec4 swatchColor = { col.r / 255.0f, col.g / 255.0f,
                               col.b / 255.0f, col.a / 255.0f };

        ImGui::PushID(i);

        // Highlight selected swatch
        if (i == selected) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 1));
        }

        ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoTooltip |
                                    ImGuiColorEditFlags_NoPicker;
        if (ImGui::ColorButton("##swatch", swatchColor, flags,
                               ImVec2(SWATCH_SIZE, SWATCH_SIZE))) {
            palette.selectIndex(i);
        }

        if (i == selected) {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("#%02X%02X%02X", col.r, col.g, col.b);
        }

        // Row wrap
        if ((i + 1) % SWATCHES_PER_ROW != 0)
            ImGui::SameLine();

        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace Framenote
