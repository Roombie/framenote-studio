#pragma once

#include <memory>
#include <unordered_map>
#include "core/Document.h"

namespace Framenote {

// ── Tool input event (no SDL dependency) ──────────────────────────────────────
struct ToolEvent {
    float canvasX;    // pixel coordinate on the canvas
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

// ── Abstract base tool ────────────────────────────────────────────────────────
class Tool {
public:
    virtual ~Tool() = default;

    virtual ToolType type() const = 0;
    virtual const char* name() const = 0;

    // Called on mouse press, drag, and release respectively
    virtual void onPress  (Document& doc, int frameIndex, const ToolEvent& e) {}
    virtual void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) {}
    virtual void onRelease(Document& doc, int frameIndex, const ToolEvent& e) {}
};

// ── Tool manager ──────────────────────────────────────────────────────────────
class ToolManager {
public:
    ToolManager();   // Registers all built-in tools

    void       selectTool(ToolType type);
    Tool*      activeTool();
    ToolType   activeToolType() const { return m_activeType; }

private:
    std::unordered_map<ToolType, std::unique_ptr<Tool>> m_tools;
    ToolType m_activeType = ToolType::Pencil;
};

} // namespace Framenote
