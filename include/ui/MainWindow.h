#pragma once

#include "core/Document.h"
#include "core/Timeline.h"
#include "tools/ToolManager.h"
#include "renderer/CanvasRenderer.h"
#include <string>

namespace Framenote {

class MainWindow {
public:
    MainWindow(Document*       document,
               Timeline*       timeline,
               ToolManager*    toolManager,
               CanvasRenderer* canvasRenderer);

    void render();

private:
    void renderMenuBar();
    void renderDockspace();
    void renderCanvasSizeDialog();

    Document*       m_document;
    Timeline*       m_timeline;
    ToolManager*    m_toolManager;
    CanvasRenderer* m_canvasRenderer;

    // Modal state
    bool        m_showAbout            = false;
    bool        m_showCanvasSizeDialog = false;
    int         m_newCanvasW           = 128;
    int         m_newCanvasH           = 128;
    std::string m_statusMsg;
};

} // namespace Framenote
