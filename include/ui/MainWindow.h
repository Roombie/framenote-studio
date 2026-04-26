#pragma once

#include "ui/TabManager.h"
#include "tools/ToolManager.h"
#include <string>

namespace Framenote {

class MainWindow {
public:
    MainWindow(TabManager* tabManager, ToolManager* toolManager);
    void render();

private:
    void renderMenuBar();
    void renderCanvasSizeDialog();
    void renderExportDialog();
    void renderOnionOpacityDialog();

    enum class ExportType { GIF, PNG, PNGSequence };

    TabManager*  m_tabManager;
    ToolManager* m_toolManager;

    bool        m_showAbout            = false;
    bool        m_showOnionDialog        = false;
    bool        m_showCanvasSizeDialog = false;
    bool        m_showExportDialog     = false;
    ExportType  m_exportType           = ExportType::GIF;
    int         m_newCanvasW           = 128;
    int         m_newCanvasH           = 128;
    std::string m_statusMsg;
};

} // namespace Framenote