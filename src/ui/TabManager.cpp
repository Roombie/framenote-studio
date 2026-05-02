#include "ui/TabManager.h"
#include "ui/CanvasPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ToolsPanel.h"
#include "ui/PalettePanel.h"
#include "ui/Theme.h"
#include "ui/ModalUtils.h"
#include "core/Selection.h"
#include "tools/SelectionTool.h"
#include "tools/MoveTool.h"

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
    ImVec2 thumbMax = {
        thumbPos.x + thumbSize.x,
        thumbPos.y + thumbSize.y
    };

    dl->AddRectFilled(
        thumbPos,
        thumbMax,
        exists ? IM_COL32(35, 35, 42, 255) : IM_COL32(55, 35, 35, 255),
        5.0f
    );

    float checkerSize = 6.0f;

    for (float y = thumbPos.y; y < thumbMax.y; y += checkerSize) {
        for (float x = thumbPos.x; x < thumbMax.x; x += checkerSize) {
            int cx = static_cast<int>((x - thumbPos.x) / checkerSize);
            int cy = static_cast<int>((y - thumbPos.y) / checkerSize);

            bool light = ((cx + cy) % 2) == 0;

            ImU32 checkerColor = light
                ? IM_COL32(80, 80, 85, 255)
                : IM_COL32(55, 55, 60, 255);

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
                ? IM_COL32(220, 220, 230, 255)
                : IM_COL32(255, 120, 120, 255),
            label
        );
    }

    dl->AddRect(
        thumbPos,
        thumbMax,
        exists ? IM_COL32(95, 98, 115, 255) : IM_COL32(210, 95, 95, 255),
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
    ImVec2 thumbMax = {
        thumbPos.x + thumbSize.x,
        thumbPos.y + thumbSize.y
    };

    dl->AddRectFilled(
        thumbPos,
        thumbMax,
        IM_COL32(35, 35, 42, 255),
        5.0f
    );

    float checkerSize = 6.0f;

    for (float y = thumbPos.y; y < thumbMax.y; y += checkerSize) {
        for (float x = thumbPos.x; x < thumbMax.x; x += checkerSize) {
            int cx = static_cast<int>((x - thumbPos.x) / checkerSize);
            int cy = static_cast<int>((y - thumbPos.y) / checkerSize);

            bool light = ((cx + cy) % 2) == 0;

            ImU32 checkerColor = light
                ? IM_COL32(80, 80, 85, 255)
                : IM_COL32(55, 55, 60, 255);

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
            IM_COL32(220, 220, 230, 255),
            label
        );
    }

    dl->AddRect(
        thumbPos,
        thumbMax,
        IM_COL32(95, 98, 115, 255),
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

static void commitFloatingIfNeeded(DocumentTab& tab, ToolManager& toolManager) {
    if (!tab.hasFloating)
        return;

    if (!tab.document || !tab.timeline)
        return;

    ToolEvent ce;
    ce.selection = tab.selection.get();
    ce.tab = &tab;

    int frameIndex = tab.timeline->currentFrame();

    if (tab.floatingSource == FloatingSource::Selection) {
        auto* selectionTool = static_cast<SelectionTool*>(
            toolManager.getTool(ToolType::Select)
        );

        if (selectionTool) {
            selectionTool->commitFloat(
                *tab.document,
                frameIndex,
                ce
            );
        }
    }
    else if (tab.floatingSource == FloatingSource::CanvasMove) {
        auto* moveTool = static_cast<MoveTool*>(
            toolManager.getTool(ToolType::Move)
        );

        if (moveTool) {
            moveTool->commitFloat(
                *tab.document,
                frameIndex,
                ce
            );
        }
    }
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

    struct TabItem {
        bool isHome = false;
        int docIndex = -1;
    };

    struct TabRect {
        float x = 0.0f;
        float w = 0.0f;
    };

    const int totalVisualTabs = static_cast<int>(m_tabs.size()) + 1;

    m_homeTabVisualIndex = std::max(
        0,
        std::min(m_homeTabVisualIndex, totalVisualTabs - 1)
    );

    std::vector<TabItem> visualItems;
    std::vector<TabRect> tabRects;

    visualItems.reserve(totalVisualTabs);
    tabRects.reserve(totalVisualTabs);

    auto docIndexToVisualIndex = [&](int docIndex) -> int {
        if (docIndex < 0)
            return -1;

        return docIndex < m_homeTabVisualIndex
            ? docIndex
            : docIndex + 1;
    };

    auto eraseDocumentTab = [&](int docIndex) {
        if (docIndex < 0 || docIndex >= static_cast<int>(m_tabs.size()))
            return;

        int visualIndex = docIndexToVisualIndex(docIndex);

        m_tabs.erase(m_tabs.begin() + docIndex);

        if (visualIndex >= 0 && visualIndex < m_homeTabVisualIndex)
            m_homeTabVisualIndex--;

        m_homeTabVisualIndex = std::max(
            0,
            std::min(m_homeTabVisualIndex, static_cast<int>(m_tabs.size()))
        );

        if (m_activeIndex == docIndex) {
            if (m_tabs.empty())
                m_activeIndex = -1;
            else
                m_activeIndex = std::min(docIndex, static_cast<int>(m_tabs.size()) - 1);
        }
        else if (m_activeIndex > docIndex) {
            m_activeIndex--;
        }

        m_draggingTabVisualIndex = -1;
    };

    auto rebuildFromVisualOrder = [&](const std::vector<TabItem>& orderedItems) {
        bool homeWasActive = (m_activeIndex == -1);

        DocumentTab* activeDoc = nullptr;

        if (m_activeIndex >= 0 && m_activeIndex < static_cast<int>(m_tabs.size())) {
            activeDoc = m_tabs[m_activeIndex].get();
        }

        std::vector<std::unique_ptr<DocumentTab>> reorderedTabs;
        reorderedTabs.reserve(m_tabs.size());

        int newHomeIndex = 0;
        int newActiveIndex = -1;

        for (int visualIndex = 0; visualIndex < static_cast<int>(orderedItems.size()); ++visualIndex) {
            const TabItem& item = orderedItems[visualIndex];

            if (item.isHome) {
                newHomeIndex = visualIndex;
                continue;
            }

            if (item.docIndex < 0 || item.docIndex >= static_cast<int>(m_tabs.size()))
                continue;

            DocumentTab* docPtr = m_tabs[item.docIndex].get();

            reorderedTabs.push_back(std::move(m_tabs[item.docIndex]));

            if (docPtr == activeDoc) {
                newActiveIndex = static_cast<int>(reorderedTabs.size()) - 1;
            }
        }

        m_tabs = std::move(reorderedTabs);
        m_homeTabVisualIndex = newHomeIndex;
        m_activeIndex = homeWasActive ? -1 : newActiveIndex;
    };

    for (int visualIndex = 0; visualIndex < totalVisualTabs; ++visualIndex) {
        if (visualIndex > 0)
            ImGui::SameLine();

        const bool isHome = (visualIndex == m_homeTabVisualIndex);

        TabItem item;
        item.isHome = isHome;
        item.docIndex = isHome
            ? -1
            : (visualIndex < m_homeTabVisualIndex ? visualIndex : visualIndex - 1);

        visualItems.push_back(item);

        bool active = isHome
            ? (m_activeIndex == -1)
            : (m_activeIndex == item.docIndex);

        bool dragging = (m_draggingTabVisualIndex == visualIndex);

        if (dragging) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.4f));
        }
        else if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
        }

        std::string label;

        if (isHome) {
            label = "  Home  ##tab_home";
        }
        else {
            label = m_tabs[item.docIndex]->name;

            if (m_tabs[item.docIndex]->document->isDirty())
                label += " *";

            label += "##tab" + std::to_string(item.docIndex);
        }

        float tabX = ImGui::GetCursorScreenPos().x;

        ImGui::Button(label.c_str());

        float tabW = ImGui::GetItemRectSize().x;
        tabRects.push_back({tabX, tabW});

        if (dragging || active)
            ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            m_pressedTabVisualIndex = visualIndex;
            m_tabDragStarted = false;
        }

        constexpr float TabDragThreshold = 1.5f;

        if (m_pressedTabVisualIndex == visualIndex &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, TabDragThreshold)) {
            if (m_draggingTabVisualIndex < 0) {
                m_draggingTabVisualIndex = visualIndex;
                m_tabDragStarted = true;
            }
        }

        if (!isHome) {
            ImGui::SameLine();

            std::string closeId = "x##close" + std::to_string(item.docIndex);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            if (ImGui::SmallButton(closeId.c_str())) {
                if (m_tabs[item.docIndex]->document->isDirty()) {
                    m_pendingCloseIndex = item.docIndex;
                    ImGui::OpenPopup("Unsaved Changes##close");
                }
                else {
                    eraseDocumentTab(item.docIndex);

                    ImGui::PopStyleColor();
                    ImGui::End();
                    return;
                }
            }

            ImGui::PopStyleColor();
        }
    }

    if (m_pressedTabVisualIndex >= 0 &&
        m_draggingTabVisualIndex < 0 &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        
        if (m_pressedTabVisualIndex < static_cast<int>(visualItems.size())) {
            const TabItem& pressedItem = visualItems[m_pressedTabVisualIndex];

            m_activeIndex = pressedItem.isHome
                ? -1
                : pressedItem.docIndex;
        }

        m_pressedTabVisualIndex = -1;
        m_tabDragStarted = false;
    }

    if (m_draggingTabVisualIndex >= 0 &&
        m_draggingTabVisualIndex < static_cast<int>(visualItems.size())) {
        float mouseX = io.MousePos.x;

        const TabItem& draggingItem = visualItems[m_draggingTabVisualIndex];

        std::string ghostLabel;

        if (draggingItem.isHome) {
            ghostLabel = "Home";
        }
        else if (draggingItem.docIndex >= 0 &&
                 draggingItem.docIndex < static_cast<int>(m_tabs.size())) {
            ghostLabel = m_tabs[draggingItem.docIndex]->name;

            if (m_tabs[draggingItem.docIndex]->document->isDirty())
                ghostLabel += " *";
        }

        ImVec2 textSize = ImGui::CalcTextSize(ghostLabel.c_str());

        float ghostW = textSize.x + 16.0f;
        float ghostX = mouseX - ghostW * 0.5f;
        float ghostY = ImGui::GetWindowPos().y + 4.0f;

        dl->AddRectFilled(
            {ghostX, ghostY},
            {ghostX + ghostW, ghostY + 24.0f},
            IM_COL32(44, 184, 213, 180),
            4.0f
        );

        dl->AddText(
            {ghostX + 8.0f, ghostY + 4.0f},
            IM_COL32(255, 255, 255, 255),
            ghostLabel.c_str()
        );

        int insertionIndex = 0;

        for (int i = 0; i < static_cast<int>(tabRects.size()); ++i) {
            float center = tabRects[i].x + tabRects[i].w * 0.5f;

            if (mouseX > center)
                insertionIndex = i + 1;
            else
                break;
        }

        int finalInsertIndex = insertionIndex;

        if (finalInsertIndex > m_draggingTabVisualIndex)
            finalInsertIndex--;

        finalInsertIndex = std::max(
            0,
            std::min(finalInsertIndex, static_cast<int>(visualItems.size()) - 1)
        );

        if (finalInsertIndex != m_draggingTabVisualIndex) {
            float lineX = 0.0f;

            if (insertionIndex <= 0) {
                lineX = tabRects.front().x;
            }
            else if (insertionIndex >= static_cast<int>(tabRects.size())) {
                lineX = tabRects.back().x + tabRects.back().w;
            }
            else {
                lineX = tabRects[insertionIndex].x;
            }

            float lineY = ImGui::GetWindowPos().y;

            dl->AddLine(
                {lineX, lineY + 2.0f},
                {lineX, lineY + 30.0f},
                IM_COL32(44, 184, 213, 255),
                2.0f
            );
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (finalInsertIndex != m_draggingTabVisualIndex) {
                std::vector<TabItem> reorderedItems = visualItems;

                TabItem movedItem = reorderedItems[m_draggingTabVisualIndex];
                reorderedItems.erase(reorderedItems.begin() + m_draggingTabVisualIndex);

                int insertAt = insertionIndex;

                if (insertAt > m_draggingTabVisualIndex)
                    insertAt--;

                insertAt = std::max(
                    0,
                    std::min(insertAt, static_cast<int>(reorderedItems.size()))
                );

                reorderedItems.insert(reorderedItems.begin() + insertAt, movedItem);

                rebuildFromVisualOrder(reorderedItems);
            }

            m_draggingTabVisualIndex = -1;
        }
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_draggingTabVisualIndex = -1;
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
            eraseDocumentTab(m_pendingCloseIndex);

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

    // Open the New Document modal from within ##Home so it is part of
    // this window's popup stack.
    if (m_showNewDialog) {
        ImGui::OpenPopup("New Document##dlg");
        m_showNewDialog = false;
    }

    ModalUtils::centerNextWindowOnAppearing();

    if (ImGui::BeginPopupModal(
            "New Document##dlg",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ModalUtils::keepCurrentWindowInsideMainViewport();

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

    auto isHomeModalActive = [&]() {
        return ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    };

    float centerX = io.DisplaySize.x * 0.5f;
    float startY  = io.DisplaySize.y * 0.25f - tabBarH;

    ImGui::SetCursorPos({centerX - 120, startY});
    ImGui::Text("Framenote Studio");

    ImGui::SetCursorPos({centerX - 120, startY + 40});
    ImGui::TextDisabled("Flipnote's soul. Aseprite's precision.");

    ImGui::SetCursorPos({centerX - 120, startY + 90});

    ImGui::BeginDisabled(isHomeModalActive());

    if (ImGui::Button("  New Document  ", {240, 40})) {
        m_showNewDialog = true;
        m_newDocW   = 128;
        m_newDocH   = 128;
        m_newDocFps = 8;
        strncpy(m_newDocName, "untitled", sizeof(m_newDocName));
    }

    ImGui::EndDisabled();

    ImGui::SetCursorPos({centerX - 120, startY + 140});

    ImGui::BeginDisabled(isHomeModalActive());

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

    ImGui::EndDisabled();

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

        ImGui::BeginDisabled(isHomeModalActive());

        if (ImGui::SmallButton(recoverButtonLabel.c_str())) {
            m_showRecoverDialog = true;
        }

        bool recoverHovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

        ImGui::EndDisabled();

        if (!isHomeModalActive() && recoverHovered) {
            ImGui::SetTooltip("Open autosaved recovery files");
        }

        ImGui::SameLine();
    }

    bool showThumbs = m_recentFiles.showThumbnails();

    ImGui::BeginDisabled(isHomeModalActive());

    if (ImGui::Checkbox("Thumbnails", &showThumbs)) {
        m_recentFiles.setShowThumbnails(showThumbs);
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    bool showPaths = m_recentFiles.showPaths();

    ImGui::BeginDisabled(isHomeModalActive());

    if (ImGui::Checkbox("Paths", &showPaths)) {
        m_recentFiles.setShowPaths(showPaths);
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(m_recentFiles.entries().empty() || isHomeModalActive());

    if (ImGui::SmallButton("Clear")) {
        ImGui::OpenPopup("Clear Recent Projects##home");
    }

    ImGui::EndDisabled();

    ModalUtils::centerNextWindowOnAppearing();

    if (ImGui::BeginPopupModal(
            "Clear Recent Projects##home",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ModalUtils::keepCurrentWindowInsideMainViewport();

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

    // Recompute after the New/Recover/Clear controls had a chance to set/open popups.
    bool homeInteractionsBlocked = isHomeModalActive();

    ImGui::BeginChild(
        "##recent-projects-list",
        {recentW, recentH},
        true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    const bool recentListCanReceiveMouse =
        !homeInteractionsBlocked &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

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
                exists &&
                ImGui::IsMouseHoveringRect(cardMin, cardMax, true);

            const bool darkTheme = Theme::current() == ThemeMode::Dark;

            ImU32 bgColor = 0;
            ImU32 borderColor = 0;

            if (darkTheme) {
                bgColor = exists
                    ? IM_COL32(28, 28, 31, 255)
                    : IM_COL32(45, 28, 28, 255);

                if (cardHovered && exists)
                    bgColor = IM_COL32(36, 38, 43, 255);

                if (entry.pinned && exists) {
                    bgColor = cardHovered
                        ? IM_COL32(39, 43, 48, 255)
                        : IM_COL32(32, 34, 39, 255);
                }

                borderColor = exists
                    ? IM_COL32(70, 72, 82, 255)
                    : IM_COL32(170, 80, 80, 255);
            }
            else {
                bgColor = exists
                    ? IM_COL32(246, 244, 238, 255)
                    : IM_COL32(255, 235, 235, 255);

                if (cardHovered && exists)
                    bgColor = IM_COL32(236, 233, 225, 255);

                if (entry.pinned && exists) {
                    bgColor = cardHovered
                        ? IM_COL32(228, 238, 241, 255)
                        : IM_COL32(236, 246, 249, 255);
                }

                borderColor = exists
                    ? IM_COL32(185, 181, 170, 255)
                    : IM_COL32(210, 115, 115, 255);
            }

            if (entry.pinned && exists && !homeInteractionsBlocked)
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

            titleHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            if (!homeInteractionsBlocked && titleHovered) {
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

                if (!homeInteractionsBlocked &&
                    pathHovered &&
                    visiblePath != entry.path) {
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

            // Open
            ImGui::BeginDisabled(homeInteractionsBlocked || !exists);

            if (ImGui::SmallButton("Open")) {
                openPath = entry.path;
            }

            openButtonHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            ImGui::EndDisabled();

            ImGui::SameLine();

            // Pin / Unpin
            ImGui::BeginDisabled(homeInteractionsBlocked);

            if (ImGui::SmallButton(entry.pinned ? "Unpin" : "Pin")) {
                pinPath = entry.path;
            }

            pinButtonHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            ImGui::EndDisabled();

            ImGui::SameLine();

            // Remove
            ImGui::BeginDisabled(homeInteractionsBlocked);

            if (ImGui::SmallButton("Remove")) {
                removePath = entry.path;
            }

            removeButtonHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            ImGui::EndDisabled();

            ImGui::SameLine();

            // Show in folder
            ImGui::BeginDisabled(homeInteractionsBlocked);

            if (ImGui::SmallButton("Show")) {
                showInFolderPath = entry.path;
            }

            bool folderButtonHovered =
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);

            if (!homeInteractionsBlocked && folderButtonHovered) {
                ImGui::SetTooltip("Show in folder");
            }

            ImGui::EndDisabled();

            ImGui::EndGroup();

            bool hoveringCardOnly =
                recentListCanReceiveMouse &&
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

        if (!homeInteractionsBlocked) {
            if (!openPath.empty()) {
                std::string err;
                auto doc = FileManager::load(openPath, err);

                if (doc) {
                    std::string name =
                        openPath.substr(openPath.find_last_of("/\\") + 1);

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

    ImGui::BeginDisabled(isHomeModalActive());

    if (ImGui::SmallButton("YouTube"))
        SDL_OpenURL("https://www.youtube.com/@Roombie");

    if (!isHomeModalActive() && ImGui::IsItemHovered())
        ImGui::SetTooltip("youtube.com/@Roombie");

    ImGui::SameLine();

    if (ImGui::SmallButton("Twitter / X"))
        SDL_OpenURL("https://x.com/Roombie_");

    if (!isHomeModalActive() && ImGui::IsItemHovered())
        ImGui::SetTooltip("x.com/Roombie_");

    ImGui::SameLine();

    if (ImGui::SmallButton("Itch.io"))
        SDL_OpenURL("https://roombiedev.itch.io/");

    if (!isHomeModalActive() && ImGui::IsItemHovered())
        ImGui::SetTooltip("roombiedev.itch.io");

    ImGui::EndDisabled();

    if (m_showRecoverDialog) {
        ImGui::OpenPopup("Recover Files##home");
        m_showRecoverDialog = false;
    }

    ModalUtils::centerNextWindowOnAppearing();

    if (ImGui::BeginPopupModal(
            "Recover Files##home",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ModalUtils::keepCurrentWindowInsideMainViewport();

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

    ToolType toolBefore = toolManager.activeToolType();

    ToolsPanel(&toolManager, m_icons).render();

    ToolType toolAfter = toolManager.activeToolType();

    if (toolAfter != toolBefore) {
        commitFloatingIfNeeded(tab, toolManager);
    }

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
        tab.canvasStrokeFrameIndex,
        tab.selection.get(),
        &tab
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
    tab->selection = std::make_unique<Selection>(
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