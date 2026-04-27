#pragma once
#include "tools/ToolManager.h"
#include "IconLoader.h"

namespace Framenote {

class ToolsPanel {
public:
    ToolsPanel(ToolManager* toolManager, ToolIcons* icons)
        : m_toolManager(toolManager), m_icons(icons) {}

    void render();

private:
    ToolManager* m_toolManager;
    ToolIcons*   m_icons;
};

} // namespace Framenote
