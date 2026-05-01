#pragma once

#include <vector>
#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include <imgui.h>
#include "core/DocumentTab.h"
#include "tools/ToolManager.h"
#include "ui/IconLoader.h"
#include "io/RecentFiles.h"
#include "io/RecoveryManager.h"

namespace Framenote {

class TabManager {
public:
    explicit TabManager(SDL_Renderer* renderer, ToolIcons* icons = nullptr);

    void render(ToolManager& toolManager);
    void newDocument(const std::string& name, int w, int h, int fps);

    DocumentTab* activeTab();
    void         openDocument(std::unique_ptr<Document> doc,
                              const std::string& name,
                              const std::string& path);
    bool         hasUnsavedTabs() const;
    bool         isHomeActive() const { return m_activeIndex == -1; }

    void showNewDialog() {
        m_showNewDialog = true;
        m_newDocW   = 128;
        m_newDocH   = 128;
        m_newDocFps = 8;
        strncpy(m_newDocName, "untitled", sizeof(m_newDocName));
    }
    
    void recordRecentFile(const std::string& path, const Document& doc);

    void autosaveDirtyTabs(double nowSeconds);
    void clearRecoveryForActiveTab();
    void clearRecoveryForPath(const std::string& path);

private:
    void renderHomeTab(ToolManager& toolManager);
    void renderDocumentTab(DocumentTab& tab, ToolManager& toolManager);
    void renderTabBar();
    void setupDefaultDockLayout(ImGuiID dockspaceId);

    SDL_Renderer*                             m_sdlRenderer;
    std::vector<std::unique_ptr<DocumentTab>> m_tabs;
    int                                       m_activeIndex       = -1;
    int                                       m_pendingCloseIndex = -1;
    int                                       m_draggingTab       = -1;

    bool m_showNewDialog   = false;
    bool m_dockInitialized = false;  // true after first-run layout is built
    int  m_newDocW         = 128;
    int  m_newDocH         = 128;
    int  m_newDocFps       = 8;
    char m_newDocName[64]  = "untitled";
    ToolIcons* m_icons     = nullptr;

    RecentFiles m_recentFiles;
    std::string m_homeStatusMsg;

    RecoveryManager m_recoveryManager;

    bool m_showRecoverDialog = false;
    std::string m_recoverStatusMsg;

    int m_homeTabVisualIndex = 0;
    int m_draggingTabVisualIndex = -1;
    int  m_pressedTabVisualIndex = -1;
    bool m_tabDragStarted = false;
};

} // namespace Framenote