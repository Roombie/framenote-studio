#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class SelectionTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Select; }
    const char* name() const override { return "Select"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

    void commitFloat(Document& doc, int frameIndex, const ToolEvent& e);
    void deselect(const ToolEvent& e);

    // Preview state
    bool isSelecting() const { return m_mode == Mode::Selecting; }
    bool isMoving()    const { return m_mode == Mode::Moving; }

    int startX() const { return m_startX; }
    int startY() const { return m_startY; }
    int endX()   const { return m_endX; }
    int endY()   const { return m_endY; }

private:
    enum class Mode {
        Idle,
        PendingSelecting,
        Selecting,
        Moving
    };

    void liftFloat(Document& doc, int frameIndex, const ToolEvent& e);
    void stampFloat(Document& doc, int frameIndex, const ToolEvent& e);

    Mode m_mode      = Mode::Idle;
    int  m_startX    = 0;
    int  m_startY    = 0;
    int  m_endX      = 0;
    int  m_endY      = 0;
    int  m_dragLastX = 0;
    int  m_dragLastY = 0;
};

} // namespace Framenote