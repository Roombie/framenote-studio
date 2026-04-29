#include "ui/PalettePanel.h"
#include "io/FileDialog.h"

#include <imgui.h>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <string>

namespace Framenote {

static ImVec4 toImVec4(Color c) {
    return {
        c.r / 255.0f,
        c.g / 255.0f,
        c.b / 255.0f,
        c.a / 255.0f
    };
}

static uint8_t toByte(float v) {
    v = std::clamp(v, 0.0f, 1.0f);
    return static_cast<uint8_t>(v * 255.0f + 0.5f);
}

static Color fromFloatColor(const float values[4]) {
    return {
        toByte(values[0]),
        toByte(values[1]),
        toByte(values[2]),
        toByte(values[3])
    };
}

static Color mixColor(Color a, Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);

    auto lerpByte = [t](uint8_t x, uint8_t y) -> uint8_t {
        float v = x + (y - x) * t;
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f) + 0.5f);
    };

    return {
        lerpByte(a.r, b.r),
        lerpByte(a.g, b.g),
        lerpByte(a.b, b.b),
        a.a
    };
}

static Color shiftWarm(Color c, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);

    int r = static_cast<int>(c.r + 45.0f * amount);
    int g = static_cast<int>(c.g + 18.0f * amount);
    int b = static_cast<int>(c.b - 28.0f * amount);

    return {
        static_cast<uint8_t>(std::clamp(r, 0, 255)),
        static_cast<uint8_t>(std::clamp(g, 0, 255)),
        static_cast<uint8_t>(std::clamp(b, 0, 255)),
        c.a
    };
}

static Color shiftCool(Color c, float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);

    int r = static_cast<int>(c.r - 28.0f * amount);
    int g = static_cast<int>(c.g + 10.0f * amount);
    int b = static_cast<int>(c.b + 45.0f * amount);

    return {
        static_cast<uint8_t>(std::clamp(r, 0, 255)),
        static_cast<uint8_t>(std::clamp(g, 0, 255)),
        static_cast<uint8_t>(std::clamp(b, 0, 255)),
        c.a
    };
}

static float luminance(Color c) {
    return (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.0f;
}

static Color outlineCandidate(Color c) {
    float luma = luminance(c);

    if (luma > 0.45f) {
        return mixColor(c, {0, 0, 0, c.a}, 0.75f);
    }

    return mixColor(c, {0, 0, 0, c.a}, 0.45f);
}

static void applyLabColor(
    Palette& palette,
    Document* document,
    Color color,
    bool& editingSecondary
) {
    int activeIndex = editingSecondary
        ? palette.secondaryIndex()
        : palette.primaryIndex();

    // Left-click applies to whichever color slot is currently active.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        palette.color(activeIndex) = color;
        document->markDirty();
    }

    // Right-click always applies to Secondary and makes Secondary active.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        palette.color(palette.secondaryIndex()) = color;
        editingSecondary = true;
        document->markDirty();
    }

    // Middle-click adds as a new palette color.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Middle)) {
        palette.addColor(color);
        document->markDirty();
    }
}

static void drawLabSwatch(
    const char* label,
    Color color,
    Palette& palette,
    Document* document,
    bool& editingSecondary
) {
    ImGuiColorEditFlags flags =
        ImGuiColorEditFlags_NoTooltip |
        ImGuiColorEditFlags_NoPicker |
        ImGuiColorEditFlags_AlphaPreview |
        ImGuiColorEditFlags_NoOptions;

    ImGui::ColorButton(label, toImVec4(color), flags, {24, 24});

    applyLabColor(palette, document, color, editingSecondary);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "%s\n#%02X%02X%02X%02X\nLeft-click: apply to active color\nRight-click: apply to Secondary\nMiddle-click: add to palette",
            label,
            color.r,
            color.g,
            color.b,
            color.a
        );
    }
}

static void renderColorRampSuggestions(
    Palette& palette,
    Document* document,
    bool& editingSecondary
) {
    Color base = editingSecondary
        ? palette.secondaryColor()
        : palette.primaryColor();

    int baseIndex = editingSecondary
        ? palette.secondaryIndex()
        : palette.primaryIndex();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Color Lab");

    ImGui::Text(
        "Based on: %s %d",
        editingSecondary ? "Secondary" : "Primary",
        baseIndex
    );

    ImGui::TextDisabled("Left: active   Right: secondary   Middle: add");

    if (base.a == 0) {
        ImGui::Spacing();
        ImGui::TextWrapped("Transparent selected. Pick a visible color to generate ramps.");
        return;
    }

    Color black = {0, 0, 0, base.a};
    Color white = {255, 255, 255, base.a};

    Color shadow2   = mixColor(base, black, 0.70f);
    Color shadow1   = mixColor(base, black, 0.45f);
    Color darker    = mixColor(base, black, 0.25f);
    Color lighter   = mixColor(base, white, 0.30f);
    Color highlight = mixColor(base, white, 0.55f);
    Color warm      = shiftWarm(base, 0.75f);
    Color cool      = shiftCool(base, 0.75f);
    Color outline   = outlineCandidate(base);

    ImGui::Spacing();

    ImGui::Text("Ramp");

    drawLabSwatch("Shadow 2", shadow2, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Shadow 1", shadow1, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Darker", darker, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Base", base, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Lighter", lighter, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Highlight", highlight, palette, document, editingSecondary);

    ImGui::Spacing();

    ImGui::Text("Temperature");

    drawLabSwatch("Cooler", cool, palette, document, editingSecondary);
    ImGui::SameLine();

    drawLabSwatch("Warmer", warm, palette, document, editingSecondary);

    ImGui::Spacing();

    ImGui::Text("Utility");

    drawLabSwatch("Outline", outline, palette, document, editingSecondary);
}

PalettePanel::PalettePanel(Document* document, bool& editingSecondary)
    : m_document(document)
    , m_editingSecondary(editingSecondary)
{}

void PalettePanel::render() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("Palette", nullptr, flags);

    ImGui::BeginChild(
        "##palette-scroll",
        ImVec2(0, 0),
        false,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    Palette& palette = m_document->palette();

    renderColorSlots(palette);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Palette");
    ImGui::Spacing();

    renderSwatchGrid(palette);

    ImGui::Spacing();
    renderToolbar(palette);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Color chooser");
    ImGui::Spacing();

    renderColorChooser(palette);

    ImGui::EndChild();
    ImGui::End();
}

void PalettePanel::renderColorSlots(Palette& palette) {
    Color primary = palette.primaryColor();
    Color secondary = palette.secondaryColor();

    ImGui::TextDisabled("Primary / Secondary");

    ImGuiColorEditFlags flags =
        ImGuiColorEditFlags_NoTooltip |
        ImGuiColorEditFlags_NoPicker |
        ImGuiColorEditFlags_AlphaPreview |
        ImGuiColorEditFlags_NoOptions;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::Indent(4.0f);

    ImGui::PushID("primary-slot");
    ImGui::ColorButton("##primary", toImVec4(primary), flags, {42, 42});

    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        if (!m_editingSecondary) {
            dl->AddRect(
                {min.x + 1.0f, min.y + 1.0f},
                {max.x - 1.0f, max.y - 1.0f},
                IM_COL32(255, 255, 255, 255),
                2.0f,
                0,
                2.0f
            );
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        m_editingSecondary = false;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Primary color\nClick to edit Primary\n#%02X%02X%02X%02X",
            primary.r,
            primary.g,
            primary.b,
            primary.a
        );
    }

    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID("secondary-slot");
    ImGui::ColorButton("##secondary", toImVec4(secondary), flags, {42, 42});

    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        if (m_editingSecondary) {
            dl->AddRect(
                {min.x + 1.0f, min.y + 1.0f},
                {max.x - 1.0f, max.y - 1.0f},
                IM_COL32(0, 190, 255, 255),
                2.0f,
                0,
                2.0f
            );
        }
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        m_editingSecondary = true;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Secondary color\nClick to edit Secondary\n#%02X%02X%02X%02X",
            secondary.r,
            secondary.g,
            secondary.b,
            secondary.a
        );
    }

    ImGui::PopID();

    ImGui::SameLine();

    ImGui::BeginGroup();

    ImGui::Text("P: %d", palette.primaryIndex());
    ImGui::Text("S: %d", palette.secondaryIndex());

    if (ImGui::SmallButton("Swap")) {
        int p = palette.primaryIndex();
        int s = palette.secondaryIndex();

        palette.selectPrimaryIndex(s);
        palette.selectSecondaryIndex(p);

        m_document->markDirty();
    }

    ImGui::EndGroup();

    ImGui::Unindent(4.0f);

    ImGui::TextDisabled(
        "P #%02X%02X%02X%02X",
        primary.r,
        primary.g,
        primary.b,
        primary.a
    );

    ImGui::TextDisabled(
        "S #%02X%02X%02X%02X",
        secondary.r,
        secondary.g,
        secondary.b,
        secondary.a
    );
}

void PalettePanel::renderSwatchGrid(Palette& palette) {
    float availW      = ImGui::GetContentRegionAvail().x;
    float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

    int cols = static_cast<int>((availW + itemSpacing) / (SWATCH_SIZE + itemSpacing));

    if (cols < 1)
        cols = 1;

    int deleteIndex = -1;

    int moveFrom = -1;
    int moveTo   = -1;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::Indent(4.0f);

    for (int i = 0; i < palette.size(); ++i) {
        Color col = palette.color(i);
        ImVec4 swatchColor = toImVec4(col);

        ImGui::PushID(i);

        ImGuiColorEditFlags flags =
            ImGuiColorEditFlags_NoTooltip |
            ImGuiColorEditFlags_NoPicker |
            ImGuiColorEditFlags_AlphaPreview |
            ImGuiColorEditFlags_NoOptions;

        ImGui::ColorButton("##swatch", swatchColor, flags, {SWATCH_SIZE, SWATCH_SIZE});

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        bool isPrimary   = i == palette.primaryIndex();
        bool isSecondary = i == palette.secondaryIndex();

        if (isPrimary) {
            dl->AddRect(
                {min.x + 1.0f, min.y + 1.0f},
                {max.x - 1.0f, max.y - 1.0f},
                IM_COL32(255, 255, 255, 255),
                2.0f,
                0,
                2.0f
            );
        }

        if (isSecondary) {
            dl->AddRect(
                {min.x + 3.0f, min.y + 3.0f},
                {max.x - 3.0f, max.y - 3.0f},
                IM_COL32(0, 190, 255, 255),
                2.0f,
                0,
                2.0f
            );
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            palette.selectPrimaryIndex(i);
            m_editingSecondary = false;
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !ImGui::GetIO().KeyShift) {
            palette.selectSecondaryIndex(i);
            m_editingSecondary = true;
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(
                "FRAMENOTE_PALETTE_COLOR",
                &i,
                sizeof(int)
            );

            ImGui::Text("Move color %d", i);

            ImGui::ColorButton(
                "##drag-preview",
                swatchColor,
                flags,
                {SWATCH_SIZE, SWATCH_SIZE}
            );

            ImGui::TextDisabled("Drop on another color to reorder");
            ImGui::TextDisabled("Drop on trash to delete");

            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("FRAMENOTE_PALETTE_COLOR")) {
                IM_ASSERT(payload->DataSize == sizeof(int));

                int from = *static_cast<const int*>(payload->Data);

                if (from != i) {
                    moveFrom = from;
                    moveTo   = i;
                }
            }

            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Color %d\n#%02X%02X%02X%02X\nLeft-click: primary\nRight-click: secondary\nDrag: reorder/delete\nShift + right-click: menu",
                i,
                col.r,
                col.g,
                col.b,
                col.a
            );

            if (ImGui::GetIO().KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup("##swatchMenu");
            }
        }

        if (ImGui::BeginPopup("##swatchMenu")) {
            if (ImGui::MenuItem("Set as Primary")) {
                palette.selectPrimaryIndex(i);
                m_editingSecondary = false;
            }

            if (ImGui::MenuItem("Set as Secondary")) {
                palette.selectSecondaryIndex(i);
                m_editingSecondary = true;
            }

            if (ImGui::MenuItem("Duplicate")) {
                palette.addColor(col);
                m_document->markDirty();
            }

            ImGui::Separator();

            bool canDelete = palette.size() > 1;

            if (ImGui::MenuItem("Delete", nullptr, false, canDelete)) {
                deleteIndex = i;
            }

            ImGui::EndPopup();
        }

        if ((i + 1) % cols != 0)
            ImGui::SameLine();

        ImGui::PopID();
    }

    if (palette.size() % cols != 0)
        ImGui::SameLine();

    if (ImGui::Button("+##add-color", {SWATCH_SIZE, SWATCH_SIZE})) {
        palette.addColor({0, 0, 0, 255});

        int newIdx = palette.size() - 1;

        palette.selectPrimaryIndex(newIdx);
        m_editingSecondary = false;

        m_document->markDirty();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Add color");
    }

    ImGui::Unindent(4.0f);

    ImGui::Spacing();

    bool canDelete = palette.size() > 1;

    ImGui::BeginDisabled(!canDelete);

    ImVec2 trashSize = {
        ImGui::GetContentRegionAvail().x,
        30.0f
    };

    ImGui::Button("Drop color here to delete", trashSize);

    ImVec2 trashMin = ImGui::GetItemRectMin();
    ImVec2 trashMax = ImGui::GetItemRectMax();

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("FRAMENOTE_PALETTE_COLOR")) {
            IM_ASSERT(payload->DataSize == sizeof(int));

            int from = *static_cast<const int*>(payload->Data);

            if (canDelete) {
                deleteIndex = from;
            }
        }

        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Drag a palette color here to delete it");
    }

    ImGui::EndDisabled();

    dl->AddRect(
        trashMin,
        trashMax,
        canDelete ? IM_COL32(180, 80, 80, 180) : IM_COL32(80, 80, 80, 120),
        4.0f,
        0,
        1.5f
    );

    if (moveFrom >= 0 && moveTo >= 0 && moveFrom != moveTo) {
        palette.moveColor(moveFrom, moveTo);
        m_document->markDirty();
    }

    if (deleteIndex >= 0 && palette.size() > 1) {
        palette.removeColor(deleteIndex);
        m_document->markDirty();
    }
}

void PalettePanel::renderColorChooser(Palette& palette) {
    int primaryIndex = palette.primaryIndex();
    int secondaryIndex = palette.secondaryIndex();

    if (primaryIndex < 0 || primaryIndex >= palette.size())
        return;

    if (secondaryIndex < 0 || secondaryIndex >= palette.size())
        return;

    Color& primary = palette.color(primaryIndex);
    Color& secondary = palette.color(secondaryIndex);

    float primaryEdit[4] = {
        primary.r / 255.0f,
        primary.g / 255.0f,
        primary.b / 255.0f,
        primary.a / 255.0f
    };

    float secondaryEdit[4] = {
        secondary.r / 255.0f,
        secondary.g / 255.0f,
        secondary.b / 255.0f,
        secondary.a / 255.0f
    };

    ImGuiColorEditFlags pickerFlags =
        ImGuiColorEditFlags_AlphaBar |
        ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_InputRGB |
        ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_NoSmallPreview |
        ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_NoOptions;

    ImGuiColorEditFlags editFlags =
        ImGuiColorEditFlags_AlphaBar |
        ImGuiColorEditFlags_DisplayRGB |
        ImGuiColorEditFlags_InputRGB |
        ImGuiColorEditFlags_AlphaPreview |
        ImGuiColorEditFlags_NoOptions;

    ImGui::TextWrapped(
        "Primary box edits Primary. Secondary box edits Secondary."
    );
    ImGui::TextWrapped(
        "Left-drag picker: Primary. Right-drag picker: Secondary."
    );

    bool leftMouseActive =
        ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    bool rightMouseActive =
        ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right);

    bool editingSecondaryNow =
        rightMouseActive ||
        (!leftMouseActive && m_editingSecondary);

    float pickerEdit[4] = {
        editingSecondaryNow ? secondaryEdit[0] : primaryEdit[0],
        editingSecondaryNow ? secondaryEdit[1] : primaryEdit[1],
        editingSecondaryNow ? secondaryEdit[2] : primaryEdit[2],
        editingSecondaryNow ? secondaryEdit[3] : primaryEdit[3]
    };

    ImGui::Spacing();

    ImGui::TextDisabled(
        editingSecondaryNow
            ? "Editing: Secondary"
            : "Editing: Primary"
    );

    float availW = ImGui::GetContentRegionAvail().x;
    float pickerW = availW;

    if (pickerW > 220.0f)
        pickerW = 220.0f;

    if (pickerW < 120.0f)
        pickerW = availW;

    if (pickerW < 80.0f)
        pickerW = 80.0f;

    bool changed = false;

    ImGui::PushItemWidth(pickerW);

    if (rightMouseActive) {
        ImGuiIO& io = ImGui::GetIO();

        bool oldMouseDown0 = io.MouseDown[0];
        bool oldMouseClicked0 = io.MouseClicked[0];
        bool oldMouseReleased0 = io.MouseReleased[0];
        bool oldMouseDoubleClicked0 = io.MouseDoubleClicked[0];

        float oldMouseDownDuration0 = io.MouseDownDuration[0];
        float oldMouseDownDurationPrev0 = io.MouseDownDurationPrev[0];

        ImVec2 oldMouseClickedPos0 = io.MouseClickedPos[0];
        float oldMouseDragMaxDistanceSqr0 = io.MouseDragMaxDistanceSqr[0];

        io.MouseDown[0] = io.MouseDown[1];
        io.MouseClicked[0] = io.MouseClicked[1];
        io.MouseReleased[0] = io.MouseReleased[1];
        io.MouseDoubleClicked[0] = io.MouseDoubleClicked[1];

        io.MouseDownDuration[0] = io.MouseDownDuration[1];
        io.MouseDownDurationPrev[0] = io.MouseDownDurationPrev[1];

        io.MouseClickedPos[0] = io.MouseClickedPos[1];
        io.MouseDragMaxDistanceSqr[0] = io.MouseDragMaxDistanceSqr[1];

        changed = ImGui::ColorPicker4(
            "##shared-primary-secondary-picker",
            pickerEdit,
            pickerFlags
        );

        io.MouseDown[0] = oldMouseDown0;
        io.MouseClicked[0] = oldMouseClicked0;
        io.MouseReleased[0] = oldMouseReleased0;
        io.MouseDoubleClicked[0] = oldMouseDoubleClicked0;

        io.MouseDownDuration[0] = oldMouseDownDuration0;
        io.MouseDownDurationPrev[0] = oldMouseDownDurationPrev0;

        io.MouseClickedPos[0] = oldMouseClickedPos0;
        io.MouseDragMaxDistanceSqr[0] = oldMouseDragMaxDistanceSqr0;
    }
    else {
        changed = ImGui::ColorPicker4(
            "##shared-primary-secondary-picker",
            pickerEdit,
            pickerFlags
        );
    }

    ImGui::PopItemWidth();

    bool pickerHovered = ImGui::IsItemHovered();

    if (pickerHovered) {
        if (rightMouseActive) {
            m_editingSecondary = true;
        }
        else if (leftMouseActive) {
            m_editingSecondary = false;
        }
    }

    if (changed) {
        if (editingSecondaryNow) {
            secondary = fromFloatColor(pickerEdit);
            m_editingSecondary = true;
        }
        else {
            primary = fromFloatColor(pickerEdit);
            m_editingSecondary = false;
        }

        m_document->markDirty();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Primary");

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::ColorEdit4("##primary-edit", primaryEdit, editFlags)) {
        primary = fromFloatColor(primaryEdit);
        m_editingSecondary = false;
        m_document->markDirty();
    }

    ImGui::PopItemWidth();

    ImGui::Spacing();

    ImGui::Text("Secondary");

    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::ColorEdit4("##secondary-edit", secondaryEdit, editFlags)) {
        secondary = fromFloatColor(secondaryEdit);
        m_editingSecondary = true;
        m_document->markDirty();
    }

    ImGui::PopItemWidth();

    renderColorRampSuggestions(palette, m_document, m_editingSecondary);
}

void PalettePanel::renderToolbar(Palette& palette) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float btnW = (ImGui::GetContentRegionAvail().x -
                  ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;

    if (ImGui::Button("Load##palette", {btnW, 0})) {
        std::string path = FileDialog::openFile(
            "Framenote Palette\0*.pal\0All Files\0*.*\0",
            "Load Palette"
        );

        if (!path.empty()) {
            if (palette.load(path))
                m_document->markDirty();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Save##palette", {btnW, 0})) {
        std::string path = FileDialog::saveFile(
            "Framenote Palette\0*.pal\0All Files\0*.*\0",
            "Save Palette",
            "palette.pal"
        );

        if (!path.empty())
            palette.save(path);
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset##palette", {btnW, 0})) {
        ImGui::OpenPopup("##ResetPalette");
    }

    if (ImGui::BeginPopup("##ResetPalette")) {
        ImGui::Text("Reset palette to default?");
        ImGui::Separator();

        if (ImGui::Button("Yes", {60, 0})) {
            palette.loadDefault();
            m_document->markDirty();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", {70, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace Framenote