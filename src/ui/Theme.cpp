#include "ui/Theme.h"
#include <imgui.h>

namespace Framenote {

ThemeMode Theme::s_current = ThemeMode::Dark;

// Helper: convert 0xRRGGBB hex to ImVec4 with alpha
static ImVec4 hex(unsigned int rgb, float a = 1.0f) {
    return ImVec4(
        ((rgb >> 16) & 0xFF) / 255.0f,
        ((rgb >>  8) & 0xFF) / 255.0f,
        ( rgb        & 0xFF) / 255.0f,
        a
    );
}

void Theme::applyDark() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    // Spacing & shape settings
    s.WindowRounding    = 4.0f;
    s.ChildRounding     = 4.0f;
    s.FrameRounding     = 3.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 3.0f;
    s.GrabRounding      = 3.0f;
    s.TabRounding       = 4.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.WindowPadding     = {8, 8};
    s.FramePadding      = {6, 4};
    s.ItemSpacing       = {6, 5};
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 8.0f;

    // Colors
    // Nibbit blue: #2CB8D5
    // Backgrounds
    c[ImGuiCol_WindowBg]          = hex(0x1E1C1A);
    c[ImGuiCol_ChildBg]           = hex(0x1E1C1A);
    c[ImGuiCol_PopupBg]           = hex(0x252321);
    c[ImGuiCol_MenuBarBg]         = hex(0x2A2826);

    // Borders
    c[ImGuiCol_Border]            = hex(0x3A3835);
    c[ImGuiCol_BorderShadow]      = hex(0x000000, 0.0f);

    // Frames (inputs, checkboxes etc)
    c[ImGuiCol_FrameBg]           = hex(0x2A2826);
    c[ImGuiCol_FrameBgHovered]    = hex(0x333130);
    c[ImGuiCol_FrameBgActive]     = hex(0x1A3A4A);

    // Title bars
    c[ImGuiCol_TitleBg]           = hex(0x1A1816);
    c[ImGuiCol_TitleBgActive]     = hex(0x1A3A4A);
    c[ImGuiCol_TitleBgCollapsed]  = hex(0x1A1816);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]       = hex(0x1E1C1A);
    c[ImGuiCol_ScrollbarGrab]     = hex(0x3A3835);
    c[ImGuiCol_ScrollbarGrabHovered] = hex(0x2CB8D5, 0.6f);
    c[ImGuiCol_ScrollbarGrabActive]  = hex(0x2CB8D5);

    // Checkmark & slider
    c[ImGuiCol_CheckMark]         = hex(0x2CB8D5);
    c[ImGuiCol_SliderGrab]        = hex(0x2CB8D5);
    c[ImGuiCol_SliderGrabActive]  = hex(0x1A8FA5);

    // Buttons
    c[ImGuiCol_Button]            = hex(0x2A2826);
    c[ImGuiCol_ButtonHovered]     = hex(0x2CB8D5, 0.25f);
    c[ImGuiCol_ButtonActive]      = hex(0x2CB8D5, 0.5f);

    // Headers (menu items, selectables, collapsing headers)
    c[ImGuiCol_Header]            = hex(0x1A3A4A);
    c[ImGuiCol_HeaderHovered]     = hex(0x2CB8D5, 0.2f);
    c[ImGuiCol_HeaderActive]      = hex(0x2CB8D5, 0.4f);

    // Separator
    c[ImGuiCol_Separator]         = hex(0x3A3835);
    c[ImGuiCol_SeparatorHovered]  = hex(0x2CB8D5, 0.5f);
    c[ImGuiCol_SeparatorActive]   = hex(0x2CB8D5);

    // Resize grip
    c[ImGuiCol_ResizeGrip]        = hex(0x3A3835);
    c[ImGuiCol_ResizeGripHovered] = hex(0x2CB8D5, 0.5f);
    c[ImGuiCol_ResizeGripActive]  = hex(0x2CB8D5);

    // Tabs
    c[ImGuiCol_Tab]               = hex(0x252321);
    c[ImGuiCol_TabHovered]        = hex(0x2CB8D5, 0.3f);
    c[ImGuiCol_TabActive]         = hex(0x1A3A4A);
    c[ImGuiCol_TabUnfocused]      = hex(0x1E1C1A);
    c[ImGuiCol_TabUnfocusedActive]= hex(0x252321);

    // Text
    c[ImGuiCol_Text]              = hex(0xD4D0C8);
    c[ImGuiCol_TextDisabled]      = hex(0x888480);

    // Docking
    // Misc
    c[ImGuiCol_PlotLines]         = hex(0x2CB8D5);
    c[ImGuiCol_PlotLinesHovered]  = hex(0x8DD8E8);
    c[ImGuiCol_PlotHistogram]     = hex(0x2CB8D5);
    c[ImGuiCol_PlotHistogramHovered] = hex(0x8DD8E8);
    c[ImGuiCol_TableHeaderBg]     = hex(0x2A2826);
    c[ImGuiCol_TableBorderStrong] = hex(0x3A3835);
    c[ImGuiCol_TableBorderLight]  = hex(0x2A2826);
    c[ImGuiCol_TableRowBg]        = hex(0x000000, 0.0f);
    c[ImGuiCol_TableRowBgAlt]     = hex(0xFFFFFF, 0.03f);
    c[ImGuiCol_TextSelectedBg]    = hex(0x2CB8D5, 0.3f);
    c[ImGuiCol_DragDropTarget]    = hex(0x2CB8D5);
    c[ImGuiCol_NavHighlight]      = hex(0x2CB8D5);
    c[ImGuiCol_NavWindowingHighlight] = hex(0x2CB8D5, 0.7f);
    c[ImGuiCol_NavWindowingDimBg] = hex(0x000000, 0.4f);
    c[ImGuiCol_ModalWindowDimBg]  = hex(0x000000, 0.5f);

    s_current = ThemeMode::Dark;
}

void Theme::applyLight() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;

    // Same shape settings as dark
    s.WindowRounding    = 4.0f;
    s.ChildRounding     = 4.0f;
    s.FrameRounding     = 3.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 3.0f;
    s.GrabRounding      = 3.0f;
    s.TabRounding       = 4.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.WindowPadding     = {8, 8};
    s.FramePadding      = {6, 4};
    s.ItemSpacing       = {6, 5};
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 8.0f;

    // Colors
    c[ImGuiCol_WindowBg]          = hex(0xE8E6E0);
    c[ImGuiCol_ChildBg]           = hex(0xE8E6E0);
    c[ImGuiCol_PopupBg]           = hex(0xF0EEE8);
    c[ImGuiCol_MenuBarBg]         = hex(0xC8C4BC);

    c[ImGuiCol_Border]            = hex(0xB0ACA4);
    c[ImGuiCol_BorderShadow]      = hex(0x000000, 0.0f);

    c[ImGuiCol_FrameBg]           = hex(0xD8D6D0);
    c[ImGuiCol_FrameBgHovered]    = hex(0xC8C4BC);
    c[ImGuiCol_FrameBgActive]     = hex(0xE6F6FA);

    c[ImGuiCol_TitleBg]           = hex(0xC8C4BC);
    c[ImGuiCol_TitleBgActive]     = hex(0x2CB8D5);
    c[ImGuiCol_TitleBgCollapsed]  = hex(0xC8C4BC);

    c[ImGuiCol_ScrollbarBg]       = hex(0xE8E6E0);
    c[ImGuiCol_ScrollbarGrab]     = hex(0xB0ACA4);
    c[ImGuiCol_ScrollbarGrabHovered] = hex(0x2CB8D5, 0.6f);
    c[ImGuiCol_ScrollbarGrabActive]  = hex(0x2CB8D5);

    c[ImGuiCol_CheckMark]         = hex(0x1A8FA5);
    c[ImGuiCol_SliderGrab]        = hex(0x2CB8D5);
    c[ImGuiCol_SliderGrabActive]  = hex(0x1A8FA5);

    c[ImGuiCol_Button]            = hex(0xD8D6D0);
    c[ImGuiCol_ButtonHovered]     = hex(0x2CB8D5, 0.25f);
    c[ImGuiCol_ButtonActive]      = hex(0x2CB8D5, 0.5f);

    c[ImGuiCol_Header]            = hex(0xE6F6FA);
    c[ImGuiCol_HeaderHovered]     = hex(0x2CB8D5, 0.2f);
    c[ImGuiCol_HeaderActive]      = hex(0x2CB8D5, 0.35f);

    c[ImGuiCol_Separator]         = hex(0xB0ACA4);
    c[ImGuiCol_SeparatorHovered]  = hex(0x2CB8D5, 0.5f);
    c[ImGuiCol_SeparatorActive]   = hex(0x2CB8D5);

    c[ImGuiCol_ResizeGrip]        = hex(0xB0ACA4);
    c[ImGuiCol_ResizeGripHovered] = hex(0x2CB8D5, 0.5f);
    c[ImGuiCol_ResizeGripActive]  = hex(0x2CB8D5);

    c[ImGuiCol_Tab]               = hex(0xD8D6D0);
    c[ImGuiCol_TabHovered]        = hex(0x2CB8D5, 0.3f);
    c[ImGuiCol_TabActive]         = hex(0xE6F6FA);
    c[ImGuiCol_TabUnfocused]      = hex(0xE8E6E0);
    c[ImGuiCol_TabUnfocusedActive]= hex(0xD8D6D0);

    c[ImGuiCol_Text]              = hex(0x2A2826);
    c[ImGuiCol_TextDisabled]      = hex(0x888480);
    c[ImGuiCol_PlotLines]         = hex(0x1A8FA5);
    c[ImGuiCol_PlotLinesHovered]  = hex(0x2CB8D5);
    c[ImGuiCol_PlotHistogram]     = hex(0x1A8FA5);
    c[ImGuiCol_PlotHistogramHovered] = hex(0x2CB8D5);
    c[ImGuiCol_TableHeaderBg]     = hex(0xC8C4BC);
    c[ImGuiCol_TableBorderStrong] = hex(0xB0ACA4);
    c[ImGuiCol_TableBorderLight]  = hex(0xC8C4BC);
    c[ImGuiCol_TableRowBg]        = hex(0x000000, 0.0f);
    c[ImGuiCol_TableRowBgAlt]     = hex(0x000000, 0.03f);
    c[ImGuiCol_TextSelectedBg]    = hex(0x2CB8D5, 0.3f);
    c[ImGuiCol_DragDropTarget]    = hex(0x2CB8D5);
    c[ImGuiCol_NavHighlight]      = hex(0x2CB8D5);
    c[ImGuiCol_NavWindowingHighlight] = hex(0x2CB8D5, 0.7f);
    c[ImGuiCol_NavWindowingDimBg] = hex(0x000000, 0.2f);
    c[ImGuiCol_ModalWindowDimBg]  = hex(0x000000, 0.2f);

    s_current = ThemeMode::Light;
}

void Theme::apply(ThemeMode mode) {
    if (mode == ThemeMode::Dark)
        applyDark();
    else
        applyLight();
}

void Theme::toggle() {
    apply(s_current == ThemeMode::Dark ? ThemeMode::Light : ThemeMode::Dark);
}

} // namespace Framenote