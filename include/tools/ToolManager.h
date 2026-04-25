#pragma once

#include <memory>
#include <unordered_map>
#include "core/Document.h"
#include "core/History.h"

namespace Framenote {

struct ToolEvent {
    float canvasX;
    float canvasY;
    bool  leftDown;
    bool  rightDown;
};

enum class ToolType {
    Pencil,
    Eraser,
    Fill,
    Eyedropper,
};

class Tool {
public:
    virtual ~Tool() = default;
    virtual ToolType    type() const = 0;
    virtual const char* name() const = 0;
    virtual void onPress  (Document& doc, int frameIndex, const ToolEvent& e) {}
    virtual void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) {}
    virtual void onRelease(Document& doc, int frameIndex, const ToolEvent& e) {}
};

class ToolManager {
public:
    ToolManager();

    void       selectTool(ToolType type);
    Tool*      activeTool();
    ToolType   activeToolType() const { return m_activeType; }

    // History access — CanvasPanel uses this for undo/redo
    History&       history()       { return m_history; }
    const History& history() const { return m_history; }

    // Called by CanvasPanel before onPress to snapshot current frame
    void snapshotBefore(Document& doc, int frameIndex);

private:
    std::unordered_map<ToolType, std::unique_ptr<Tool>> m_tools;
    ToolType m_activeType = ToolType::Pencil;
    History  m_history;
};

} // namespace Framenote
