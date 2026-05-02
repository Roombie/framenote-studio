#pragma once

#include "tools/ToolManager.h"

#include <cstdint>

namespace Framenote {

class EllipseTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Ellipse; }
    const char* name() const override { return "Ellipse"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

    // Preview state — read by CanvasPanel.
    bool isDrawing() const { return m_drawing; }
    int  startX()    const { return m_startX; }
    int  startY()    const { return m_startY; }
    int  endX()      const { return m_endX; }
    int  endY()      const { return m_endY; }

    uint32_t previewColor() const { return m_color; }

private:
    void drawEllipse(
        Document& doc,
        int frameIndex,
        int x0,
        int y0,
        int x1,
        int y1,
        bool filled,
        uint32_t color
    );

private:
    bool     m_drawing = false;
    int      m_startX  = 0;
    int      m_startY  = 0;
    int      m_endX    = 0;
    int      m_endY    = 0;
    uint32_t m_color   = 0xFF000000;
};

} // namespace Framenote