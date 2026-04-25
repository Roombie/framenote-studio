#include "ui/TabManager.h"
#include "ui/CanvasPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ToolsPanel.h"
#include "ui/PalettePanel.h"
#include <imgui.h>
#include <cstring>
#include "io/FileDialog.h"
#include "io/FileManager.h"

namespace Framenote {

TabManager::TabManager(SDL_Renderer* renderer)
    : m_sdlRenderer(renderer)
{}

void TabManager::render(ToolManager& toolManager) {
    renderTabBar();

    if (m_activeIndex == -1) {
        renderHomeTab(toolManager);
    } else if (m_activeIndex < (int)m_tabs.size()) {
        renderDocumentTab(*m_tabs[m_activeIndex], toolManager);
    }

    // New document dialog
    if (m_showNewDialog) {
        ImGui::OpenPopup("New Document##dlg");
        m_showNewDialog = false;
    }

    if (ImGui::BeginPopupModal("New Document##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::InputText("Name##new", m_newDocName, sizeof(m_newDocName));
        ImGui::Separator();
        ImGui::Text("Canvas size:");
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("W##new", &m_newDocW);
        ImGui::SameLine(); ImGui::Text("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("H##new", &m_newDocH);

        if (m_newDocW <    1) m_newDocW =    1;
        if (m_newDocH <    1) m_newDocH =    1;
        if (m_newDocW > 4096) m_newDocW = 4096;
        if (m_newDocH > 4096) m_newDocH = 4096;

        ImGui::Separator();
        ImGui::Text("Presets:");
        ImGui::SameLine(); if (ImGui::SmallButton("16"))  { m_newDocW=16;  m_newDocH=16;  }
        ImGui::SameLine(); if (ImGui::SmallButton("32"))  { m_newDocW=32;  m_newDocH=32;  }
        ImGui::SameLine(); if (ImGui::SmallButton("64"))  { m_newDocW=64;  m_newDocH=64;  }
        ImGui::SameLine(); if (ImGui::SmallButton("128")) { m_newDocW=128; m_newDocH=128; }
        ImGui::SameLine(); if (ImGui::SmallButton("256")) { m_newDocW=256; m_newDocH=256; }
        ImGui::Separator();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("FPS##new", &m_newDocFps);
        if (m_newDocFps < 1)  m_newDocFps = 1;
        if (m_newDocFps > 60) m_newDocFps = 60;
        ImGui::Separator();

        if (ImGui::Button("Create", {100, 0})) {
            newDocument(m_newDocName[0] ? m_newDocName : "untitled",
                        m_newDocW, m_newDocH, m_newDocFps);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {100, 0}))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void TabManager::renderTabBar() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Full-width tab bar at the top
    ImGui::SetNextWindowPos({0, ImGui::GetFrameHeight()}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ImGui::GetIO().DisplaySize.x, 32}, ImGuiCond_Always);
    ImGui::Begin("##TabBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Home tab
    bool homeActive = (m_activeIndex == -1);
    if (homeActive)
        ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button("  Home  "))
        m_activeIndex = -1;
    if (homeActive)
        ImGui::PopStyleColor();

    // Document tabs
    for (int i = 0; i < (int)m_tabs.size(); ++i) {
        ImGui::SameLine();
        bool active = (m_activeIndex == i);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);

        // Tab label with close button
        std::string label = m_tabs[i]->name;
        if (m_tabs[i]->document->isDirty()) label += " *";
        label += "##tab" + std::to_string(i);

        if (ImGui::Button(label.c_str()))
            m_activeIndex = i;

        if (active)
            ImGui::PopStyleColor();

        // Close button
        ImGui::SameLine();
        std::string closeId = "x##close" + std::to_string(i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (ImGui::SmallButton(closeId.c_str())) {
            m_tabs.erase(m_tabs.begin() + i);
            if (m_activeIndex >= (int)m_tabs.size())
                m_activeIndex = m_tabs.empty() ? -1 : (int)m_tabs.size() - 1;
            ImGui::PopStyleColor();
            ImGui::End();
            return;
        }
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

void TabManager::renderHomeTab(ToolManager& toolManager) {
    ImGuiIO& io = ImGui::GetIO();
    float tabBarH = ImGui::GetFrameHeight() + 32;

    ImGui::SetNextWindowPos({0, tabBarH}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y - tabBarH},
                              ImGuiCond_Always);
    ImGui::Begin("##Home", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Center content
    float centerX = io.DisplaySize.x * 0.5f;
    float startY  = io.DisplaySize.y * 0.25f - tabBarH;

    ImGui::SetCursorPos({centerX - 120, startY});
    ImGui::Text("Framenote Studio");

    ImGui::SetCursorPos({centerX - 120, startY + 40});
    ImGui::TextDisabled("Flipnote's soul. Aseprite's precision.");

    ImGui::SetCursorPos({centerX - 120, startY + 90});
    if (ImGui::Button("  New Document  ", {240, 40})) {
        m_showNewDialog = true;
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

    ImGui::End();
}

void TabManager::renderDocumentTab(DocumentTab& tab, ToolManager& toolManager) {
    float tabBarH = ImGui::GetFrameHeight() + 32;

    // Pass tab's own document/timeline/renderer to panels
    ToolsPanel(&toolManager).render();
    PalettePanel(tab.document.get()).render();
    TimelinePanel(tab.document.get(), tab.timeline.get()).render();
    CanvasPanel(tab.document.get(), tab.timeline.get(),
                &toolManager, tab.renderer.get()).render();
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

} // namespace Framenote

// Additional method implementations

namespace Framenote {

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

} // namespace Framenote
