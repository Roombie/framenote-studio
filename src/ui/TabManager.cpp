#include "ui/TabManager.h"
#include "ui/CanvasPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ToolsPanel.h"
#include "ui/PalettePanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>
#include <SDL3/SDL.h>
#include "io/FileDialog.h"
#include "io/FileManager.h"

namespace Framenote {

TabManager::TabManager(SDL_Renderer* renderer, ToolIcons* icons)
    : m_sdlRenderer(renderer), m_icons(icons)
{}

void TabManager::render(ToolManager& toolManager) {
    renderTabBar();

    // Route rendering to home or the active document tab
    if (m_activeIndex == -1) {
        renderHomeTab(toolManager);
    } else if (m_activeIndex < (int)m_tabs.size()) {
        renderDocumentTab(*m_tabs[m_activeIndex], toolManager);
    }

    // New document dialog — opened via flag so it can be triggered from
    // multiple places (Home button, File menu) without calling OpenPopup twice
    if (m_showNewDialog) {
        ImGui::OpenPopup("New Document##dlg");
        m_showNewDialog = false;
    }

    if (ImGui::BeginPopupModal("New Document##dlg", nullptr,
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

        // Clamp to sane limits
        if (m_newDocW <    1) m_newDocW =    1;
        if (m_newDocH <    1) m_newDocH =    1;
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

        // Custom FPS selector
        ImGui::Separator();

        if (ImGui::SmallButton("-##new_fps")) m_newDocFps--;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(55.0f);
        ImGui::InputInt("##new_fps_value", &m_newDocFps, 0, 0);
        ImGui::SameLine();
        if (ImGui::SmallButton("+##new_fps")) m_newDocFps++;
        ImGui::SameLine();
        ImGui::Text("FPS");

        // Clamp FPS to valid limits
        if (m_newDocFps < 1)  m_newDocFps = 1;
        if (m_newDocFps > 60) m_newDocFps = 60;

        ImGui::Separator();

        if (ImGui::Button("Create", {100, 0})) {
            newDocument(m_newDocName[0] ? m_newDocName : "untitled",
                        m_newDocW, m_newDocH, m_newDocFps);
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

    // Full-width fixed tab bar sits between the menu bar and the content area
    ImGui::SetNextWindowPos({0, ImGui::GetFrameHeight()}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, 32}, ImGuiCond_Always);

    ImGui::Begin("##TabBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // We draw the ghost tab and drop indicator directly onto the window drawlist
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Home tab — always first, never draggable
    bool homeActive = (m_activeIndex == -1);

    if (homeActive)
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);

    if (ImGui::Button("  Home  "))
        m_activeIndex = -1;

    if (homeActive)
        ImGui::PopStyleColor();

    // Record the right edge of the Home tab — used as the minimum boundary
    // during drag so document tabs can never be reordered before Home
    float homeRightEdge = ImGui::GetItemRectMax().x + 4;

    // Collect screen-space rects of all document tabs for drop-target math
    struct TabRect { float x; float w; };
    std::vector<TabRect> tabRects;

    // Document tabs
    for (int i = 0; i < (int)m_tabs.size(); ++i) {
        ImGui::SameLine();

        bool active   = (m_activeIndex == i);
        bool dragging = (m_draggingTab == i);

        if (dragging)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 0.4f));
        else if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);

        std::string label = m_tabs[i]->name;
        if (m_tabs[i]->document->isDirty()) label += " *";
        label += "##tab" + std::to_string(i);

        float tabX = ImGui::GetCursorScreenPos().x;
        ImGui::Button(label.c_str());
        float tabW = ImGui::GetItemRectSize().x;
        tabRects.push_back({tabX, tabW});

        if (dragging || active)
            ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_activeIndex = i;

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f))
            if (m_draggingTab < 0) m_draggingTab = i;

        // Close button
        ImGui::SameLine();
        std::string closeId = "x##close" + std::to_string(i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (ImGui::SmallButton(closeId.c_str())) {
            if (m_tabs[i]->document->isDirty()) {
                m_pendingCloseIndex = i;
                ImGui::OpenPopup("Unsaved Changes##close");
            } else {
                m_tabs.erase(m_tabs.begin() + i);
                if (m_activeIndex >= (int)m_tabs.size())
                    m_activeIndex = m_tabs.empty() ? -1 : (int)m_tabs.size() - 1;
                if (m_draggingTab >= (int)m_tabs.size())
                    m_draggingTab = -1;
                ImGui::PopStyleColor();
                ImGui::End();
                return;
            }
        }

        ImGui::PopStyleColor();
    }

    // Drag-and-drop reordering
    if (m_draggingTab >= 0 && m_draggingTab < (int)m_tabs.size()) {
        float mouseX = io.MousePos.x;

        std::string ghostLabel = m_tabs[m_draggingTab]->name;
        if (m_tabs[m_draggingTab]->document->isDirty()) ghostLabel += " *";

        ImVec2 textSize = ImGui::CalcTextSize(ghostLabel.c_str());
        float ghostW = textSize.x + 16;
        float ghostX = mouseX - ghostW * 0.5f;
        float ghostY = ImGui::GetWindowPos().y + 4;

        dl->AddRectFilled(
            {ghostX, ghostY}, {ghostX + ghostW, ghostY + 24},
            IM_COL32(44, 184, 213, 180), 4.0f);
        dl->AddText(
            {ghostX + 8, ghostY + 4},
            IM_COL32(255, 255, 255, 255), ghostLabel.c_str());

        int dropTarget = m_draggingTab;

        if (mouseX > tabRects[m_draggingTab].x + tabRects[m_draggingTab].w * 0.5f) {
            for (int j = m_draggingTab + 1; j < (int)tabRects.size(); ++j)
                if (mouseX > tabRects[j].x + tabRects[j].w * 0.5f) dropTarget = j;
        } else {
            for (int j = m_draggingTab - 1; j >= 0; --j)
                if (mouseX < tabRects[j].x + tabRects[j].w * 0.5f &&
                    tabRects[j].x >= homeRightEdge) dropTarget = j;
        }

        if (dropTarget != m_draggingTab && dropTarget >= 0 &&
            dropTarget < (int)tabRects.size()) {
            float lineX = (dropTarget < m_draggingTab)
                ? tabRects[dropTarget].x
                : tabRects[dropTarget].x + tabRects[dropTarget].w;
            float lineY = ImGui::GetWindowPos().y;
            dl->AddLine({lineX, lineY + 2}, {lineX, lineY + 30},
                IM_COL32(44, 184, 213, 255), 2.0f);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (dropTarget != m_draggingTab && dropTarget >= 0 &&
                dropTarget < (int)m_tabs.size()) {
                auto tab = std::move(m_tabs[m_draggingTab]);
                m_tabs.erase(m_tabs.begin() + m_draggingTab);
                int insertAt = dropTarget > m_draggingTab ? dropTarget - 1 : dropTarget;
                insertAt = std::max(0, std::min(insertAt, (int)m_tabs.size()));
                m_tabs.insert(m_tabs.begin() + insertAt, std::move(tab));
                m_activeIndex = insertAt;
            }
            m_draggingTab = -1;
        }
    } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_draggingTab = -1;
    }

    // Unsaved changes confirmation dialog
    if (ImGui::BeginPopupModal("Unsaved Changes##close", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_pendingCloseIndex >= 0 && m_pendingCloseIndex < (int)m_tabs.size())
            ImGui::Text("'%s' has unsaved changes.\nDo you want to save before closing?",
                m_tabs[m_pendingCloseIndex]->name.c_str());
        ImGui::Separator();
        if (ImGui::Button("Don't Save", {120, 0})) {
            m_tabs.erase(m_tabs.begin() + m_pendingCloseIndex);
            if (m_activeIndex >= (int)m_tabs.size())
                m_activeIndex = m_tabs.empty() ? -1 : (int)m_tabs.size() - 1;
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
            "Open Framenote File");
        if (!path.empty()) {
            std::string err;
            auto doc = FileManager::load(path, err);
            if (doc) {
                std::string name = path.substr(path.find_last_of("/\\") + 1);
                openDocument(std::move(doc), name, path);
            }
        }
    }

    ImGui::SetCursorPos({centerX - 120, startY + 210});
    ImGui::TextDisabled("Recent files");

    ImGui::SetCursorPos({centerX - 120, startY + 230});
    ImGui::TextDisabled("(No recent files)");

    float socialY = io.DisplaySize.y - tabBarH - 60;
    ImGui::SetCursorPos({centerX - 120, socialY});
    ImGui::TextDisabled("Made by Roombie");

    ImGui::SetCursorPos({centerX - 120, socialY + 24});
    if (ImGui::SmallButton("YouTube")) SDL_OpenURL("https://www.youtube.com/@Roombie");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("youtube.com/@Roombie");
    ImGui::SameLine();
    if (ImGui::SmallButton("Twitter / X")) SDL_OpenURL("https://x.com/Roombie_");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("x.com/Roombie_");
    ImGui::SameLine();
    if (ImGui::SmallButton("Itch.io")) SDL_OpenURL("https://roombiedev.itch.io/");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("roombiedev.itch.io");

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

    // Auto-hide tab bars on single-panel slots to avoid wasted space
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
    ImGuiIO& io      = ImGui::GetIO();
    float tabBarH    = ImGui::GetFrameHeight() + 32;

    // Full-screen host window for the DockSpace — no decorations, no input
    ImGui::SetNextWindowPos({0, tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar      |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoScrollbar     |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav;

    ImGui::Begin("##DockHost", nullptr, hostFlags);

    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, {0, 0}, ImGuiDockNodeFlags_None);

    // Build the default layout on first run only — imgui.ini persists it after
    if (!m_dockInitialized) {
        setupDefaultDockLayout(dockspaceId);
        m_dockInitialized = true;
    }

    ImGui::End();

    // Render each panel — they are now dockable windows
    ToolsPanel(&toolManager, m_icons).render();
    PalettePanel(tab.document.get(), tab.paletteEditingSecondary).render();
    TimelinePanel(tab.document.get(), tab.timeline.get(), m_icons).render();

    CanvasPanel(
        tab.document.get(),
        tab.timeline.get(),
        &toolManager,
        tab.renderer.get(),
        tab.canvasZoom,
        tab.canvasPanX,
        tab.canvasPanY,
        *tab.history
    ).render();
}

void TabManager::newDocument(const std::string& name, int w, int h, int fps) {
    auto tab = DocumentTab::createBlank(m_sdlRenderer, name, w, h, fps);
    m_tabs.push_back(std::move(tab));
    m_activeIndex = (int)m_tabs.size() - 1;
}

DocumentTab* TabManager::activeTab() {
    if (m_activeIndex < 0 || m_activeIndex >= (int)m_tabs.size())
        return nullptr;
    return m_tabs[m_activeIndex].get();
}

void TabManager::openDocument(std::unique_ptr<Document> doc,
                              const std::string& name,
                              const std::string& path) {
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
        tab->document->canvasSize().height);
    m_tabs.push_back(std::move(tab));
    m_activeIndex = (int)m_tabs.size() - 1;
}

bool TabManager::hasUnsavedTabs() const {
    for (const auto& tab : m_tabs)
        if (tab->document->isDirty()) return true;
    return false;
}

} // namespace Framenote