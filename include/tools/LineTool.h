#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class LineTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Line; }
    const char* name() const override { return "Line"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

    // Preview state — read by CanvasPanel to draw the overlay
    bool  isDrawing() const { return m_drawing; }
    int   startX()    const { return m_startX; }
    int   startY()    const { return m_startY; }
    int   endX()      const { return m_endX; }
    int   endY()      const { return m_endY; }

private:
    void drawLine(Document& doc, int frameIndex,
                  int x0, int y0, int x1, int y1,
                  int brushSize, uint32_t color);
    void drawBrush(Document& doc, int frameIndex,
                   int x, int y, int brushSize, uint32_t color);

    bool     m_drawing  = false;
    int      m_startX   = 0;
    int      m_startY   = 0;
    int      m_endX     = 0;
    int      m_endY     = 0;
    uint32_t m_color    = 0xFF000000;
};

} // namespace Framenote