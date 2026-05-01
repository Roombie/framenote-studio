#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class MoveTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Move; }
    const char* name() const override { return "Move"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

    // Stamp float back to canvas — called on tool switch, Enter, Escape
    void commitFloat(Document& doc, int frameIndex, const ToolEvent& e);

    bool isMoving() const { return m_dragging; }

private:
    void liftFloat(Document& doc, int frameIndex, const ToolEvent& e);
    void stampFloat(Document& doc, int frameIndex, const ToolEvent& e);

    bool m_dragging  = false;
    bool m_lifted    = false;  // true once liftFloat has been called
    bool m_erased    = false;  // true once original pixels erased from frame
    int  m_lastX     = 0;
    int  m_lastY     = 0;
};

} // namespace Framenote