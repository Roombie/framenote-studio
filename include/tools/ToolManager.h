#pragma once

#include <memory>
#include <unordered_map>
#include "core/Document.h"

namespace Framenote {

struct ToolEvent {
    float canvasX;
    float canvasY;
    bool  leftDown;
    bool  rightDown;
    int   brushSize = 1;  // pixels — square brush, centered on cursor
};

enum class ToolType {
    Pencil,
    Eraser,
    Fill,
    Eyedropper,
};

// Base class for all drawing tools. Implements the strategy pattern —
// ToolManager holds one instance of each and swaps the active one.
class Tool {
public:
    virtual ~Tool() = default;
    virtual ToolType    type() const = 0;
    virtual const char* name() const = 0;

    // Called on mouse press, drag, and release over the canvas.
    // Implementations should call doc.markDirty() after modifying pixels.
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

    int  brushSize() const          { return m_brushSize; }
    void setBrushSize(int size)     { m_brushSize = size < 1 ? 1 : (size > 64 ? 64 : size); }

private:
    std::unordered_map<ToolType, std::unique_ptr<Tool>> m_tools;
    ToolType m_activeType = ToolType::Pencil;
    int      m_brushSize  = 1;
};

} // namespace Framenote