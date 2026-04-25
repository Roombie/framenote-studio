#pragma once

#include <vector>
#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "core/DocumentTab.h"
#include "tools/ToolManager.h"

namespace Framenote {

class TabManager {
public:
    explicit TabManager(SDL_Renderer* renderer);

    // Render the tab bar + home tab or active document
    void render(ToolManager& toolManager);

    // Open a new blank document tab
    void newDocument(const std::string& name, int w, int h, int fps);

    // Active document tab (nullptr if home is active)
    DocumentTab* activeTab();
    void         openDocument(std::unique_ptr<Document> doc, const std::string& name, const std::string& path);
    void         showNewDialog() { m_showNewDialog = true; strncpy(m_newDocName, "untitled", sizeof(m_newDocName)); }
    bool         isHomeActive() const { return m_activeIndex == -1; }

private:
    void renderHomeTab(ToolManager& toolManager);
    void renderDocumentTab(DocumentTab& tab, ToolManager& toolManager);
    void renderTabBar();

    SDL_Renderer*                            m_sdlRenderer;
    std::vector<std::unique_ptr<DocumentTab>> m_tabs;
    int                                       m_activeIndex = -1; // -1 = home

    // New document dialog state (moved here from MainWindow)
    bool        m_showNewDialog = false;
    int         m_newDocW       = 128;
    int         m_newDocH       = 128;
    int         m_newDocFps     = 8;
    char        m_newDocName[64] = "untitled";
};

} // namespace Framenote
