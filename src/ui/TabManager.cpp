#include "ui/TabManager.h"
#include "ui/CanvasPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ToolsPanel.h"
#include "ui/PalettePanel.h"
#include "ui/Theme.h"

#include <cstdlib>
#include <cstdint>
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "io/FileDialog.h"
#include "io/FileManager.h"

namespace Framenote {

TabManager::TabManager(SDL_Renderer* renderer, ToolIcons* icons)
    : m_sdlRenderer(renderer), m_icons(icons)
{}

static void drawRecentThumbnail(
    ImDrawList* dl,
    const RecentFileEntry& entry,
    ImVec2 thumbPos,
    ImVec2 thumbSize,
    bool exists
) {
    bool isDark = (Theme::current() == ThemeMode::Dark);

    ImVec2 thumbMax = {
        thumbPos.x + thumbSize.x,
        thumbPos.y + thumbSize.y
    };

    // Background
    ImU32 bgColor = exists
        ? (isDark ? IM_COL32(35, 35, 42, 255)  : IM_COL32(210, 210, 215, 255))
        : (isDark ? IM_COL32(55, 35, 35, 255)  : IM_COL32(230, 200, 200, 255));

    dl->AddRectFilled(thumbPos, thumbMax, bgColor, 5.0f);

    // Checkerboard (transparent area indicator)
    float checkerSize = 6.0f;

    for (float y = thumbPos.y; y < thumbMax.y; y += checkerSize) {
        for (float x = thumbPos.x; x < thumbMax.x; x += checkerSize) {
            int cx = static_cast<int>((x - thumbPos.x) / checkerSize);
            int cy = static_cast<int>((y - thumbPos.y) / checkerSize);
            bool light = ((cx + cy) % 2) == 0;

            ImU32 checkerColor = light
                ? (isDark ? IM_COL32(80, 80, 85, 255)  : IM_COL32(190, 190, 195, 255))
                : (isDark ? IM_COL32(55, 55, 60, 255)  : IM_COL32(165, 165, 170, 255));

            dl->AddRectFilled(
                {x, y},
                {
                    std::min(x + checkerSize, thumbMax.x),
                    std::min(y + checkerSize, thumbMax.y)
                },
                checkerColor
            );
        }
    }

    if (entry.hasThumbnail()) {
        float pixelW = thumbSize.x / static_cast<float>(entry.thumbnailWidth);
        float pixelH = thumbSize.y / static_cast<float>(entry.thumbnailHeight);

        for (int py = 0; py < entry.thumbnailHeight; ++py) {
            for (int px = 0; px < entry.thumbnailWidth; ++px) {
                uint32_t argb =
                    entry.thumbnailPixels[py * entry.thumbnailWidth + px];

                uint8_t a = static_cast<uint8_t>((argb >> 24) & 0xFF);
                uint8_t r = static_cast<uint8_t>((argb >> 16) & 0xFF);
                uint8_t g = static_cast<uint8_t>((argb >> 8) & 0xFF);
                uint8_t b = static_cast<uint8_t>(argb & 0xFF);

                if (a == 0)
                    continue;

                float x0 = thumbPos.x + px * pixelW;
                float y0 = thumbPos.y + py * pixelH;
                float x1 = thumbPos.x + (px + 1) * pixelW;
                float y1 = thumbPos.y + (py + 1) * pixelH;

                dl->AddRectFilled(
                    {x0, y0},
                    {x1, y1},
                    IM_COL32(r, g, b, a)
                );
            }
        }
    }
    else {
        const char* label = exists ? "FN" : "?";
        ImVec2 textSize = ImGui::CalcTextSize(label);

        dl->AddText(
            {
                thumbPos.x + (thumbSize.x - textSize.x) * 0.5f,
                thumbPos.y + (thumbSize.y - textSize.y) * 0.5f
            },
            exists
                ? (isDark ? IM_COL32(220, 220, 230, 255) : IM_COL32(50, 50, 60, 255))
                : IM_COL32(255, 120, 120, 255),
            label
        );
    }

    dl->AddRect(
        thumbPos,
        thumbMax,
        exists
            ? (isDark ? IM_COL32(95, 98, 115, 255) : IM_COL32(150, 150, 160, 255))
            : IM_COL32(210, 95, 95, 255),
        5.0f,
        0,
        1.5f
    );
}

static void drawRecoveryThumbnail(
    ImDrawList* dl,
    const RecoveryEntry& entry,
    ImVec2 thumbPos,
    ImVec2 thumbSize
) {
    bool isDark = (Theme::current() == ThemeMode::Dark);

    ImVec2 thumbMax = {
        thumbPos.x + thumbSize.x,
        thumbPos.y + thumbSize.y
    };

    dl->AddRectFilled(
        thumbPos,
        thumbMax,
        isDark ? IM_COL32(35, 35, 42, 255) : IM_COL32(210, 210, 215, 255),
        5.0f
    );

    float checkerSize = 6.0f;

    for (float y = thumbPos.y; y < thumbMax.y; y += checkerSize) {
        for (float x = thumbPos.x; x < thumbMax.x; x += checkerSize) {
            int cx = static_cast<int>((x - thumbPos.x) / checkerSize);
            int cy = static_cast<int>((y - thumbPos.y) / checkerSize);
            bool light = ((cx + cy) % 2) == 0;

            ImU32 checkerColor = light
                ? (isDark ? IM_COL32(80, 80, 85, 255)  : IM_COL32(190, 190, 195, 255))
                : (isDark ? IM_COL32(55, 55, 60, 255)  : IM_COL32(165, 165, 170, 255));

            dl->AddRectFilled(
                {x, y},
                {
                    std::min(x + checkerSize, thumbMax.x),
                    std::min(y + checkerSize, thumbMax.y)
                },
                checkerColor
            );
        }
    }

    if (entry.hasThumbnail()) {
        float pixelW = thumbSize.x / static_cast<float>(entry.thumbnailWidth);
        float pixelH = thumbSize.y / static_cast<float>(entry.thumbnailHeight);

        for (int py = 0; py < entry.thumbnailHeight; ++py) {
            for (int px = 0; px < entry.thumbnailWidth; ++px) {
                uint32_t argb =
                    entry.thumbnailPixels[py * entry.thumbnailWidth + px];

                uint8_t a = static_cast<uint8_t>((argb >> 24) & 0xFF);
                uint8_t r = static_cast<uint8_t>((argb >> 16) & 0xFF);
                uint8_t g = static_cast<uint8_t>((argb >> 8) & 0xFF);
                uint8_t b = static_cast<uint8_t>(argb & 0xFF);

                if (a == 0)
                    continue;

                float x0 = thumbPos.x + px * pixelW;
                float y0 = thumbPos.y + py * pixelH;
                float x1 = thumbPos.x + (px + 1) * pixelW;
                float y1 = thumbPos.y + (py + 1) * pixelH;

                dl->AddRectFilled(
                    {x0, y0},
                    {x1, y1},
                    IM_COL32(r, g, b, a)
                );
            }
        }
    }
    else {
        const char* label = "REC";
        ImVec2 textSize = ImGui::CalcTextSize(label);

        dl->AddText(
            {
                thumbPos.x + (thumbSize.x - textSize.x) * 0.5f,
                thumbPos.y + (thumbSize.y - textSize.y) * 0.5f
            },
            isDark ? IM_COL32(220, 220, 230, 255) : IM_COL32(50, 50, 60, 255),
            label
        );
    }

    dl->AddRect(
        thumbPos,
        thumbMax,
        isDark ? IM_COL32(95, 98, 115, 255) : IM_COL32(150, 150, 160, 255),
        5.0f,
        0,
        1.5f
    );
}

static std::string fitTextMiddleEllipsis(const std::string& text, float maxWidth) {
    if (text.empty())
        return text;

    if (maxWidth <= 0.0f)
        return "...";

    if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth)
        return text;

    const std::string dots = "...";

    if (ImGui::CalcTextSize(dots.c_str()).x >= maxWidth)
        return dots;

    size_t leftCount = text.size() / 2;
    size_t rightCount = text.size() - leftCount;

    auto makeCandidate = [&]() {
        std::string result;

        if (leftCount > 0)
            result += text.substr(0, leftCount);

        result += dots;

        if (rightCount > 0 && rightCount <= text.size())
            result += text.substr(text.size() - rightCount);

        return result;
    };

    while (leftCount + rightCount > 0) {
        std::string candidate = makeCandidate();

        if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
            return candidate;

        if (leftCount > 8) {
            --leftCount;
        }
        else if (rightCount > 24) {
            --rightCount;
        }
        else if (rightCount > 8) {
            --rightCount;
        }
        else if (leftCount > 2) {
            --leftCount;
        }
        else {
            break;
        }
    }

    return dots;
}

static void showFileInFolder(const std::string& path) {
#if defined(_WIN32)
    std::string command = "explorer.exe /select,\"" + path + "\"";
    system(command.c_str());
#else
    std::filesystem::path filePath(path);
    SDL_OpenURL(filePath.parent_path().string().c_str());
#endif
}

void TabManager::render(ToolManager& toolManager) {
    renderTabBar();

    if (m_activeIndex == -1) {
        renderHomeTab(toolManager);
    }
    else if (m_activeIndex < static_cast<int>(m_tabs.size())) {
        renderDocumentTab(*m_tabs[m_activeIndex], toolManager);
    }

    if (m_showNewDialog) {
        ImGui::OpenPopup("New Document##dlg");
        m_showNewDialog = false;
    }

    if (ImGui::BeginPopupModal(
            "New Document##dlg",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::InputText("Name##new", m_newDocName, sizeof(m_newDocName));

        ImGui::Separator();
        ImGui::Text("Canvas size:");

        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("W##new", &m_newDocW);

        ImGui::SameLine();
        ImGui::Text("x");

        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("H##new", &m_newDocH);

        if (m_newDocW < 1) m_newDocW = 1;
        if (m_newDocH < 1) m_newDocH = 1;
        if (m_newDocW > 4096) m_newDocW = 4096;
        if (m_newDocH > 4096) m_newDocH = 4096;

        ImGui::Separator();
        ImGui::Text("Presets:");

        ImGui::SameLine();
        if (ImGui::SmallButton("16"))  { m_newDocW = 16;  m_newDocH = 16;  }

        ImGui::SameLine();
        if (ImGui::SmallButton("32"))  { m_newDocW = 32;  m_newDocH = 32;  }

        ImGui::SameLine();
        if (ImGui::SmallButton("64"))  { m_newDocW = 64;  m_newDocH = 64;  }

        ImGui::SameLine();
        if (ImGui::SmallButton("128")) { m_newDocW = 128; m_newDocH = 128; }

        ImGui::SameLine();
        if (ImGui::SmallButton("256")) { m_newDocW = 256; m_newDocH = 256; }

        ImGui::Separator();

        if (ImGui::SmallButton("-##new_fps"))
            m_newDocFps--;

        ImGui::SameLine();
        ImGui::SetNextItemWidth(55.0f);
        ImGui::InputInt("##new_fps_value", &m_newDocFps, 0, 0);

        ImGui::SameLine();

        if (ImGui::SmallButton("+##new_fps"))
            m_newDocFps++;

        ImGui::SameLine();
        ImGui::Text("FPS");

        if (m_newDocFps < 1)  m_newDocFps = 1;
        if (m_newDocFps > 60) m_newDocFps = 60;

        ImGui::Separator();

        if (ImGui::Button("Create", {100, 0})) {
            newDocument(
                m_newDocName[0] ? m_newDocName : "untitled",
                m_newDocW,
                m_newDocH,
                m_newDocFps
            );

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", {100, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void TabManager::renderTabBar() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos({0, ImGui::GetFrameHeight()}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, 32}, ImGuiCond_Always);

    ImGui::Begin("##TabBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    bool homeActive = (m_activeIndex == -1);

    if (homeActive)
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);

    if (ImGui::Button("  Home  "))
        m_activeIndex = -1;

    if (homeActive)
        ImGui::PopStyleColor();

    float homeRightEdge = ImGui::GetItemRectMax().x + 4;

    struct TabRect {
        float x;
        float w;
    };

    std::vector<TabRect> tabRects;

    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        ImGui::SameLine();

        bool active   = (m_activeIndex == i);
        bool dragging = (m_draggingTab == i);

        if (dragging)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.4f));
        else if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);

        std::string label = m_tabs[i]->name;

        if (m_tabs[i]->document->isDirty())
            label += " *";

        label += "##tab" + std::to_string(i);

        float tabX = ImGui::GetCursorScreenPos().x;

        ImGui::Button(label.c_str());

        float tabW = ImGui::GetItemRectSize().x;
        tabRects.push_back({tabX, tabW});

        if (dragging || active)
            ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_activeIndex = i;

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f)) {
            if (m_draggingTab < 0)
                m_draggingTab = i;
        }

        ImGui::SameLine();

        std::string closeId = "x##close" + std::to_string(i);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (ImGui::SmallButton(closeId.c_str())) {
            if (m_tabs[i]->document->isDirty()) {
                m_pendingCloseIndex = i;
                ImGui::OpenPopup("Unsaved Changes##close");
            }
            else {
                m_tabs.erase(m_tabs.begin() + i);

                if (m_activeIndex >= static_cast<int>(m_tabs.size()))
                    m_activeIndex = m_tabs.empty() ? -1 : static_cast<int>(m_tabs.size()) - 1;

                if (m_draggingTab >= static_cast<int>(m_tabs.size()))
                    m_draggingTab = -1;

                ImGui::PopStyleColor();
                ImGui::End();
                return;
            }
        }

        ImGui::PopStyleColor();
    }

    if (m_draggingTab >= 0 && m_draggingTab < static_cast<int>(m_tabs.size())) {
        float mouseX = io.MousePos.x;

        std::string ghostLabel = m_tabs[m_draggingTab]->name;

        if (m_tabs[m_draggingTab]->document->isDirty())
            ghostLabel += " *";

        ImVec2 textSize = ImGui::CalcTextSize(ghostLabel.c_str());

        float ghostW = textSize.x + 16;
        float ghostX = mouseX - ghostW * 0.5f;
        float ghostY = ImGui::GetWindowPos().y + 4;

        dl->AddRectFilled(
            {ghostX, ghostY},
            {ghostX + ghostW, ghostY + 24},
            IM_COL32(44, 184, 213, 180),
            4.0f
        );

        dl->AddText(
            {ghostX + 8, ghostY + 4},
            IM_COL32(255, 255, 255, 255),
            ghostLabel.c_str()
        );

        int dropTarget = m_draggingTab;

        if (mouseX > tabRects[m_draggingTab].x + tabRects[m_draggingTab].w * 0.5f) {
            for (int j = m_draggingTab + 1; j < static_cast<int>(tabRects.size()); ++j) {
                if (mouseX > tabRects[j].x + tabRects[j].w * 0.5f)
                    dropTarget = j;
            }
        }
        else {
            for (int j = m_draggingTab - 1; j >= 0; --j) {
                if (mouseX < tabRects[j].x + tabRects[j].w * 0.5f &&
                    tabRects[j].x >= homeRightEdge) {
                    dropTarget = j;
                }
            }
        }

        if (dropTarget != m_draggingTab &&
            dropTarget >= 0 &&
            dropTarget < static_cast<int>(tabRects.size())) {
            float lineX = (dropTarget < m_draggingTab)
                ? tabRects[dropTarget].x
                : tabRects[dropTarget].x + tabRects[dropTarget].w;

            float lineY = ImGui::GetWindowPos().y;

            dl->AddLine(
                {lineX, lineY + 2},
                {lineX, lineY + 30},
                IM_COL32(44, 184, 213, 255),
                2.0f
            );
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (dropTarget != m_draggingTab &&
                dropTarget >= 0 &&
                dropTarget < static_cast<int>(m_tabs.size())) {
                auto tab = std::move(m_tabs[m_draggingTab]);

                m_tabs.erase(m_tabs.begin() + m_draggingTab);

                int insertAt = dropTarget > m_draggingTab
                    ? dropTarget - 1
                    : dropTarget;

                insertAt = std::max(0, std::min(insertAt, static_cast<int>(m_tabs.size())));

                m_tabs.insert(m_tabs.begin() + insertAt, std::move(tab));
                m_activeIndex = insertAt;
            }

            m_draggingTab = -1;
        }
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_draggingTab = -1;
    }

    if (ImGui::BeginPopupModal(
            "Unsaved Changes##close",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_pendingCloseIndex >= 0 && m_pendingCloseIndex < static_cast<int>(m_tabs.size())) {
            ImGui::Text(
                "'%s' has unsaved changes.\nDo you want to save before closing?",
                m_tabs[m_pendingCloseIndex]->name.c_str()
            );
        }

        ImGui::Separator();

        if (ImGui::Button("Don't Save", {120, 0})) {
            m_tabs.erase(m_tabs.begin() + m_pendingCloseIndex);

            if (m_activeIndex >= static_cast<int>(m_tabs.size()))
                m_activeIndex = m_tabs.empty() ? -1 : static_cast<int>(m_tabs.size()) - 1;

            m_pendingCloseIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", {80, 0})) {
            m_pendingCloseIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void TabManager::renderHomeTab(ToolManager& toolManager) {
    (void)toolManager;

    ImGuiIO& io = ImGui::GetIO();
    float tabBarH = ImGui::GetFrameHeight() + 32;

    ImGui::SetNextWindowPos({0, tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(1.0f);

    ImGui::Begin("##Home", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar);
    
    bool homeInteractionsBlocked =
        ImGui::IsPopupOpen("Recover Files##home") ||
        ImGui::IsPopupOpen("Clear Recent Projects##home");

    float centerX = io.DisplaySize.x * 0.5f;
    float startY  = io.DisplaySize.y * 0.25f - tabBarH;

    ImGui::SetCursorPos({centerX - 120, startY});
    ImGui::Text("Framenote Studio");

    ImGui::SetCursorPos({centerX - 120, startY + 40});
    ImGui::TextDisabled("Flipnote's soul. Aseprite's precision.");

    ImGui::SetCursorPos({centerX - 120, startY + 90});

    if (ImGui::Button("  New Document  ", {240, 40})) {
        m_showNewDialog = true;
        m_newDocW   = 128;
        m_newDocH   = 128;
        m_newDocFps = 8;
        strncpy(m_newDocName, "untitled", sizeof(m_newDocName));
    }

    ImGui::SetCursorPos({centerX - 120, startY + 140});

    if (ImGui::Button("  Open File...  ", {240, 40})) {
        std::string path = FileDialog::openFile(
            "Framenote Files\0*.framenote\0All Files\0*.*\0",
            "Open Framenote File"
        );

        if (!path.empty()) {
            std::string err;
            auto doc = FileManager::load(path, err);

            if (doc) {
                std::string name = path.substr(path.find_last_of("/\\") + 1);

                openDocument(std::move(doc), name, path);

                if (auto* opened = activeTab()) {
                    recordRecentFile(path, *opened->document);
                }

                m_homeStatusMsg.clear();
            }
            else {
                m_homeStatusMsg = "Open failed: " + err;
            }
        }
    }

    float recentX = centerX - 300.0f;
    float recentY = startY + 210.0f;
    float recentW = 600.0f;

    float footerReservedH = 95.0f;
    float availableRecentH = io.DisplaySize.y - tabBarH - recentY - footerReservedH;

    float recentH = std::min(250.0f, availableRecentH);

    if (recentH < 200.0f)
        recentH = 200.0f;

    ImGui::SetCursorPos({recentX, recentY});
    ImGui::BeginGroup();

    ImGui::Text("Recent Projects");

    ImGui::SameLine();

    int recoveryCount = static_cast<int>(m_recoveryManager.entries().size());

    if (recoveryCount > 0) {
        std::string recoverButtonLabel =
            "Recover (" + std::to_string(recoveryCount) + ")";

        if (ImGui::SmallButton(recoverButtonLabel.c_str())) {
            m_showRecoverDialog = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Open autosaved recovery files");
        }

        ImGui::SameLine();
    }

    bool showThumbs = m_recentFiles.showThumbnails();

    if (ImGui::Checkbox("Thumbnails", &showThumbs)) {
        m_recentFiles.setShowThumbnails(showThumbs);
    }

    ImGui::SameLine();

    bool showPaths = m_recentFiles.showPaths();

    if (ImGui::Checkbox("Paths", &showPaths)) {
        m_recentFiles.setShowPaths(showPaths);
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(m_recentFiles.entries().empty());

    if (ImGui::SmallButton("Clear")) {
        ImGui::OpenPopup("Clear Recent Projects##home");
    }

    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal(
            "Clear Recent Projects##home",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Clear all recent projects?");
        ImGui::Separator();

        if (ImGui::Button("Yes", {80, 0})) {
            m_recentFiles.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", {80, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::BeginChild(
        "##recent-projects-list",
        {recentW, recentH},
        true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    const auto& recent = m_recentFiles.entries();

    if (recent.empty()) {
        ImGui::TextDisabled("(No recent files)");
    }
    else {
        std::string openPath;
        std::string pinPath;
        std::string removePath;
        std::string showInFolderPath;

        for (int i = 0; i < static_cast<int>(recent.size()); ++i) {
            const auto& entry = recent[i];

            bool exists = m_recentFiles.existsOnDisk(entry);

            ImGui::PushID(i);

            ImDrawList* dl = ImGui::GetWindowDrawList();

            float cardW = ImGui::GetContentRegionAvail().x - 10.0f;

            if (cardW < 240.0f)
                cardW = ImGui::GetContentRegionAvail().x;

            float cardH = m_recentFiles.showPaths() ? 128.0f : 108.0f;

            if (!exists)
                cardH += 18.0f;

            ImVec2 cardMin = ImGui::GetCursorScreenPos();
            ImVec2 cardMax = {cardMin.x + cardW, cardMin.y + cardH};

            bool cardHovered =
                !homeInteractionsBlocked &&
                ImGui::IsMouseHoveringRect(cardMin, cardMax, true);

            bool isDarkCard = (Theme::current() == ThemeMode::Dark);

            ImU32 bgColor = exists
                ? (isDarkCard ? IM_COL32(28, 28, 31, 255)  : IM_COL32(225, 225, 228, 255))
                : (isDarkCard ? IM_COL32(45, 28, 28, 255)  : IM_COL32(235, 215, 215, 255));

            if (cardHovered && exists)
                bgColor = isDarkCard ? IM_COL32(36, 38, 43, 255) : IM_COL32(212, 214, 220, 255);

            if (entry.pinned && exists) {
                bgColor = cardHovered
                    ? (isDarkCard ? IM_COL32(39, 43, 48, 255) : IM_COL32(205, 215, 225, 255))
                    : (isDarkCard ? IM_COL32(32, 34, 39, 255) : IM_COL32(215, 220, 230, 255));
            }

            ImU32 borderColor = exists
                ? (isDarkCard ? IM_COL32(70, 72, 82, 255)  : IM_COL32(180, 182, 192, 255))
                : (isDarkCard ? IM_COL32(170, 80, 80, 255) : IM_COL32(200, 130, 130, 255));

            if (entry.pinned && exists)
                borderColor = IM_COL32(44, 184, 213, 210);

            dl->AddRectFilled(cardMin, cardMax, bgColor, 6.0f);
            dl->AddRect(cardMin, cardMax, borderColor, 6.0f, 0, 1.3f);

            ImGui::SetCursorScreenPos({cardMin.x + 10.0f, cardMin.y + 10.0f});

            float leftTextX = cardMin.x + 12.0f;

            if (m_recentFiles.showThumbnails()) {
                ImVec2 thumbPos = ImGui::GetCursorScreenPos();
                ImVec2 thumbSize = {48.0f, 48.0f};
                ImVec2 thumbMax = {
                    thumbPos.x + thumbSize.x,
                    thumbPos.y + thumbSize.y
                };

                drawRecentThumbnail(dl, entry, thumbPos, thumbSize, exists);

                ImGui::Dummy(thumbSize);
                ImGui::SameLine();

                leftTextX = thumbMax.x + 12.0f;
            }

            ImGui::SetCursorScreenPos({leftTextX, cardMin.y + 10.0f});
            ImGui::BeginGroup();

            std::string title = entry.name;

            if (entry.pinned)
                title = "* " + title;

            if (!exists)
                title += "  (missing)";

            float titleW = cardMax.x - leftTextX - 24.0f;

            if (titleW < 120.0f)
                titleW = 120.0f;

            bool titleHovered = false;
            bool metadataHovered = false;
            bool pathHovered = false;
            bool lastOpenedHovered = false;
            bool missingTextHovered = false;

            ImGui::BeginDisabled(!exists);

            std::string visibleTitle = fitTextMiddleEllipsis(title, titleW);

            ImGui::TextUnformatted(visibleTitle.c_str());

            titleHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            if (titleHovered) {
                if (!m_recentFiles.showPaths()) {
                    ImGui::SetTooltip("%s", entry.path.c_str());
                }
                else if (visibleTitle != title) {
                    ImGui::SetTooltip("%s", title.c_str());
                }
            }

            ImGui::EndDisabled();

            if (entry.canvasWidth > 0 && entry.canvasHeight > 0) {
                ImGui::TextDisabled(
                    "%dx%d | %d fps | %d frame%s",
                    entry.canvasWidth,
                    entry.canvasHeight,
                    entry.fps,
                    entry.frameCount,
                    entry.frameCount == 1 ? "" : "s"
                );

                metadataHovered = ImGui::IsItemHovered();
            }

            if (m_recentFiles.showPaths()) {
                float pathW = cardMax.x - leftTextX - 24.0f;

                if (pathW < 120.0f)
                    pathW = 120.0f;

                std::string visiblePath = fitTextMiddleEllipsis(entry.path, pathW);

                ImGui::TextDisabled("%s", visiblePath.c_str());

                pathHovered = ImGui::IsItemHovered();

                if (pathHovered && visiblePath != entry.path) {
                    ImGui::SetTooltip("%s", entry.path.c_str());
                }
            }

            if (!entry.lastOpened.empty()) {
                ImGui::TextDisabled("Last opened: %s", entry.lastOpened.c_str());
                lastOpenedHovered = ImGui::IsItemHovered();
            }

            if (!exists) {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.45f, 1.0f),
                    "File not found"
                );

                missingTextHovered = ImGui::IsItemHovered();
            }

            ImGui::Spacing();

            bool openButtonHovered = false;
            bool pinButtonHovered = false;
            bool removeButtonHovered = false;

            ImGui::BeginDisabled(!exists);

            if (ImGui::SmallButton("Open")) {
                openPath = entry.path;
            }

            openButtonHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::SmallButton(entry.pinned ? "Unpin" : "Pin")) {
                pinPath = entry.path;
            }

            pinButtonHovered = ImGui::IsItemHovered();

            ImGui::SameLine();

            if (ImGui::SmallButton("Remove")) {
                removePath = entry.path;
            }

            removeButtonHovered = ImGui::IsItemHovered();

            ImGui::SameLine();

            if (ImGui::SmallButton("Show")) {
                showInFolderPath = entry.path;
            }

            bool folderButtonHovered = ImGui::IsItemHovered();

            if (folderButtonHovered) {
                ImGui::SetTooltip("Show in folder");
            }

            ImGui::EndGroup();

            bool hoveringCardOnly =
                !homeInteractionsBlocked &&
                cardHovered &&
                !titleHovered &&
                !metadataHovered &&
                !pathHovered &&
                !lastOpenedHovered &&
                !missingTextHovered &&
                !openButtonHovered &&
                !pinButtonHovered &&
                !removeButtonHovered &&
                !folderButtonHovered;

            if (hoveringCardOnly) {
                if (exists) {
                    ImGui::SetTooltip("Open %s", entry.name.c_str());

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        openPath = entry.path;
                    }
                }
                else {
                    ImGui::SetTooltip("File not found:\n%s", entry.path.c_str());
                }
            }

            ImGui::SetCursorScreenPos({cardMin.x, cardMax.y + 8.0f});
            ImGui::Dummy({cardW, 1.0f});

            ImGui::PopID();
        }

        if (!openPath.empty()) {
            std::string err;
            auto doc = FileManager::load(openPath, err);

            if (doc) {
                std::string name = openPath.substr(openPath.find_last_of("/\\") + 1);

                openDocument(std::move(doc), name, openPath);

                if (auto* opened = activeTab()) {
                    recordRecentFile(openPath, *opened->document);
                }

                m_homeStatusMsg.clear();
            }
            else {
                m_homeStatusMsg = "Open failed: " + err;
            }
        }
        else if (!pinPath.empty()) {
            m_recentFiles.togglePinned(pinPath);
        }
        else if (!removePath.empty()) {
            m_recentFiles.removePath(removePath);
        }
        else if (!showInFolderPath.empty()) {
            showFileInFolder(showInFolderPath);
        }
    }

    if (!m_homeStatusMsg.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(
            ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
            "%s",
            m_homeStatusMsg.c_str()
        );
    }

    ImGui::EndChild();

    ImGui::EndGroup();

    float socialY = io.DisplaySize.y - tabBarH - 60;

    ImGui::SetCursorPos({centerX - 120, socialY});
    ImGui::TextDisabled("Made by Roombie");

    ImGui::SetCursorPos({centerX - 120, socialY + 24});

    if (ImGui::SmallButton("YouTube"))
        SDL_OpenURL("https://www.youtube.com/@Roombie");

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("youtube.com/@Roombie");

    ImGui::SameLine();

    if (ImGui::SmallButton("Twitter / X"))
        SDL_OpenURL("https://x.com/Roombie_");

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("x.com/Roombie_");

    ImGui::SameLine();

    if (ImGui::SmallButton("Itch.io"))
        SDL_OpenURL("https://roombiedev.itch.io/");

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("roombiedev.itch.io");

    if (m_showRecoverDialog) {
        ImGui::OpenPopup("Recover Files##home");
        m_showRecoverDialog = false;
    }

    if (ImGui::BeginPopupModal(
            "Recover Files##home",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("Recovered autosave files");
        ImGui::TextDisabled("Open a recovery, then save it normally to keep it.");
        ImGui::Separator();

        const auto& recoveries = m_recoveryManager.entries();

        if (recoveries.empty()) {
            ImGui::TextDisabled("(No recovery files)");
        }
        else {
            std::string openRecoveryId;
            std::string removeRecoveryId;

            ImGui::BeginChild(
                "##recoveries-list",
                {560.0f, 260.0f},
                true,
                ImGuiWindowFlags_AlwaysVerticalScrollbar
            );

            for (int i = 0; i < static_cast<int>(recoveries.size()); ++i) {
                const auto& entry = recoveries[i];

                ImGui::PushID(i);

                ImDrawList* dl = ImGui::GetWindowDrawList();

                ImVec2 thumbPos = ImGui::GetCursorScreenPos();
                ImVec2 thumbSize = {48.0f, 48.0f};

                drawRecoveryThumbnail(dl, entry, thumbPos, thumbSize);

                ImGui::Dummy(thumbSize);
                ImGui::SameLine();

                ImGui::BeginGroup();

                ImGui::Text("%s", entry.displayName.c_str());

                if (entry.canvasWidth > 0 && entry.canvasHeight > 0) {
                    ImGui::TextDisabled(
                        "%dx%d | %d fps | %d frame%s",
                        entry.canvasWidth,
                        entry.canvasHeight,
                        entry.fps,
                        entry.frameCount,
                        entry.frameCount == 1 ? "" : "s"
                    );
                }

                if (!entry.savedAt.empty()) {
                    ImGui::TextDisabled("Autosaved: %s", entry.savedAt.c_str());
                }

                if (!entry.sourcePath.empty()) {
                    ImGui::TextDisabled("Original: %s", entry.sourcePath.c_str());
                }
                else {
                    ImGui::TextDisabled("Original: unsaved document");
                }

                if (ImGui::SmallButton("Open Recovery")) {
                    openRecoveryId = entry.id;
                }

                ImGui::SameLine();

                if (ImGui::SmallButton("Remove")) {
                    removeRecoveryId = entry.id;
                }

                ImGui::EndGroup();

                ImGui::Separator();

                ImGui::PopID();
            }

            ImGui::EndChild();

            if (!openRecoveryId.empty()) {
                for (const auto& entry : recoveries) {
                    if (entry.id != openRecoveryId)
                        continue;

                    std::string err;
                    auto doc = FileManager::load(entry.recoveryPath, err);

                    if (doc) {
                        if (!entry.sourcePath.empty())
                            doc->setFilePath(entry.sourcePath);
                        else
                            doc->setFilePath("");

                        doc->markDirty();

                        std::string name = entry.displayName.empty()
                            ? "recovered"
                            : entry.displayName;

                        name += " (recovered)";

                        openDocument(std::move(doc), name, entry.sourcePath);

                        if (auto* opened = activeTab()) {
                            opened->recoveryId = entry.id;
                            opened->recoveryPath = entry.recoveryPath;
                        }

                        m_recoverStatusMsg.clear();
                        ImGui::CloseCurrentPopup();
                    }
                    else {
                        m_recoverStatusMsg = "Recover failed: " + err;
                    }

                    break;
                }
            }
            else if (!removeRecoveryId.empty()) {
                m_recoveryManager.remove(removeRecoveryId);
            }
        }

        if (!m_recoverStatusMsg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(
                ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                "%s",
                m_recoverStatusMsg.c_str()
            );
        }

        ImGui::Separator();

        ImGui::BeginDisabled(m_recoveryManager.entries().empty());

        if (ImGui::Button("Clear All", {90, 0})) {
            m_recoveryManager.clearAll();
        }

        ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 12.0f);

        if (ImGui::Button("Close", {90, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}

void TabManager::setupDefaultDockLayout(ImGuiID dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockMain   = dockspaceId;
    ImGuiID dockLeft   = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left,  0.14f, nullptr, &dockMain);
    ImGuiID dockRight  = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.06f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down,  0.22f, nullptr, &dockMain);

    ImGui::DockBuilderGetNode(dockLeft)->LocalFlags   |= ImGuiDockNodeFlags_AutoHideTabBar;
    ImGui::DockBuilderGetNode(dockRight)->LocalFlags  |= ImGuiDockNodeFlags_AutoHideTabBar;
    ImGui::DockBuilderGetNode(dockBottom)->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;
    ImGui::DockBuilderGetNode(dockMain)->LocalFlags   |= ImGuiDockNodeFlags_AutoHideTabBar;

    ImGui::DockBuilderDockWindow("Palette",  dockLeft);
    ImGui::DockBuilderDockWindow("Tools",    dockRight);
    ImGui::DockBuilderDockWindow("Timeline", dockBottom);
    ImGui::DockBuilderDockWindow("Canvas",   dockMain);

    ImGui::DockBuilderFinish(dockspaceId);
}

void TabManager::renderDocumentTab(DocumentTab& tab, ToolManager& toolManager) {
    ImGuiIO& io   = ImGui::GetIO();
    float tabBarH = ImGui::GetFrameHeight() + 32;

    ImGui::SetNextWindowPos({0, tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav;

    ImGui::Begin("##DockHost", nullptr, hostFlags);

    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, {0, 0}, ImGuiDockNodeFlags_None);

    if (!m_dockInitialized) {
        setupDefaultDockLayout(dockspaceId);
        m_dockInitialized = true;
    }

    ImGui::End();

    ToolsPanel(&toolManager, m_icons).render();

    PalettePanel(
        tab.document.get(),
        tab.paletteEditingSecondary,
        tab.paletteSelection,
        tab.paletteGestureActive,
        tab.paletteGestureSelecting,
        tab.paletteGestureStartedOnSelected,
        tab.paletteGestureStartIndex
    ).render();

    TimelinePanel(tab.document.get(), tab.timeline.get(), m_icons).render();

    CanvasPanel(
        tab.document.get(),
        tab.timeline.get(),
        &toolManager,
        tab.renderer.get(),
        tab.canvasZoom,
        tab.canvasPanX,
        tab.canvasPanY,
        *tab.history,
        tab.canvasStrokeActive,
        tab.canvasStrokeFrameIndex
    ).render();
}

void TabManager::newDocument(const std::string& name, int w, int h, int fps) {
    auto tab = DocumentTab::createBlank(m_sdlRenderer, name, w, h, fps);
    m_tabs.push_back(std::move(tab));
    m_activeIndex = static_cast<int>(m_tabs.size()) - 1;
}

DocumentTab* TabManager::activeTab() {
    if (m_activeIndex < 0 || m_activeIndex >= static_cast<int>(m_tabs.size()))
        return nullptr;

    return m_tabs[m_activeIndex].get();
}

void TabManager::recordRecentFile(const std::string& path, const Document& doc) {
    m_recentFiles.addOrUpdate(path, doc);
}

void TabManager::autosaveDirtyTabs(double nowSeconds) {
    constexpr double AutosaveIntervalSeconds = 30.0;

    for (auto& tab : m_tabs) {
        if (!tab || !tab->document)
            continue;

        if (!tab->document->isDirty())
            continue;

        if (nowSeconds - tab->lastAutosaveTime < AutosaveIntervalSeconds)
            continue;

        std::string err;

        if (m_recoveryManager.autosave(*tab, err)) {
            tab->lastAutosaveTime = nowSeconds;
        }
    }
}

void TabManager::clearRecoveryForActiveTab() {
    DocumentTab* tab = activeTab();

    if (!tab)
        return;

    if (!tab->recoveryId.empty()) {
        m_recoveryManager.clearForRecoveryId(tab->recoveryId);
        tab->recoveryId.clear();
        tab->recoveryPath.clear();
    }
}

void TabManager::clearRecoveryForPath(const std::string& path) {
    m_recoveryManager.clearForSourcePath(path);
}

void TabManager::openDocument(
    std::unique_ptr<Document> doc,
    const std::string& name,
    const std::string& path
) {
    (void)path;

    auto tab = std::make_unique<DocumentTab>();

    tab->name     = name;
    tab->document = std::move(doc);
    tab->timeline = std::make_unique<Timeline>();
    tab->timeline->setFrameCount(tab->document->frameCount());
    tab->timeline->setFps(tab->document->fps());
    tab->history  = std::make_unique<History>();
    tab->renderer = std::make_unique<CanvasRenderer>(
        m_sdlRenderer,
        tab->document->canvasSize().width,
        tab->document->canvasSize().height
    );

    m_tabs.push_back(std::move(tab));
    m_activeIndex = static_cast<int>(m_tabs.size()) - 1;
}

bool TabManager::hasUnsavedTabs() const {
    for (const auto& tab : m_tabs) {
        if (tab->document->isDirty())
            return true;
    }

    return false;
}

} // namespace Framenote