#include "tools/ToolManager.h"
#include "tools/PencilTool.h"
#include "tools/EraserTool.h"
#include "tools/FillTool.h"
#include "tools/EyedropperTool.h"

namespace Framenote {

ToolManager::ToolManager() {
    m_tools[ToolType::Pencil]     = std::make_unique<PencilTool>();
    m_tools[ToolType::Eraser]     = std::make_unique<EraserTool>();
    m_tools[ToolType::Fill]       = std::make_unique<FillTool>();
    m_tools[ToolType::Eyedropper] = std::make_unique<EyedropperTool>();
    m_activeType = ToolType::Pencil;
}

void ToolManager::selectTool(ToolType type) {
    if (m_tools.count(type))
        m_activeType = type;
}

Tool* ToolManager::activeTool() {
    auto it = m_tools.find(m_activeType);
    return it != m_tools.end() ? it->second.get() : nullptr;
}

} // namespace Framenote
