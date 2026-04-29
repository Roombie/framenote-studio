#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class PencilTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Pencil; }
    const char* name() const override { return "Pencil"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

private:
    // Draw a square brush of e.brushSize centered on (x, y)
    void drawBrush(Document& doc, int frameIndex, int x, int y, int size);
    void drawLine (Document& doc, int frameIndex,
                   int x0, int y0, int x1, int y1, int size);

    int  m_lastX    = -1;
    int  m_lastY    = -1;
    bool m_drawing  = false;
};

} // namespace Framenote