#include "tools/ToolManager.h"
#include "tools/PencilTool.h"
#include "tools/EraserTool.h"
#include "tools/FillTool.h"
#include "tools/EyedropperTool.h"
#include "tools/LineTool.h"
#include "tools/RectangleTool.h"
#include "tools/EllipseTool.h"
#include "tools/SelectionTool.h"
#include "tools/MoveTool.h"

namespace Framenote {

ToolManager::ToolManager() {
    m_tools[ToolType::Pencil]     = std::make_unique<PencilTool>();
    m_tools[ToolType::Eraser]     = std::make_unique<EraserTool>();
    m_tools[ToolType::Fill]       = std::make_unique<FillTool>();
    m_tools[ToolType::Eyedropper] = std::make_unique<EyedropperTool>();
    m_tools[ToolType::Line]       = std::make_unique<LineTool>();
    m_tools[ToolType::Rectangle]  = std::make_unique<RectangleTool>();
    m_tools[ToolType::Ellipse]    = std::make_unique<EllipseTool>();
    m_tools[ToolType::Select]     = std::make_unique<SelectionTool>();
    m_tools[ToolType::Move]       = std::make_unique<MoveTool>();
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

Tool* ToolManager::getTool(ToolType type) {
    auto it = m_tools.find(type);
    return it != m_tools.end() ? it->second.get() : nullptr;
}

} // namespace Framenote