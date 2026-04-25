#pragma once

#include "tools/ToolManager.h"

namespace Framenote {

// Left sidebar: tool buttons (Pencil, Eraser, Fill, Eyedropper).
// Stateless beyond which tool is active.
class ToolsPanel {
public:
    explicit ToolsPanel(ToolManager* toolManager);

    void render();

private:
    ToolManager* m_toolManager;
};

} // namespace Framenote
