#pragma once

#include "tools/ToolManager.h"
#include <cstdint>

namespace Framenote {

class Selection;

class RectangleTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Rectangle; }
    const char* name() const override { return "Rectangle"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

    bool isDrawing() const { return m_drawing; }
    int  startX()    const { return m_startX; }
    int  startY()    const { return m_startY; }
    int  endX()      const { return m_endX; }
    int  endY()      const { return m_endY; }
    bool filled()    const { return m_filled; }

private:
    void drawRect(
        Document& doc,
        int frameIndex,
        int x0,
        int y0,
        int x1,
        int y1,
        bool fill,
        uint32_t color,
        const Selection* selection
    );

    bool     m_drawing = false;
    bool     m_filled = false;
    int      m_startX = 0;
    int      m_startY = 0;
    int      m_endX = 0;
    int      m_endY = 0;
    uint32_t m_color = 0xFF000000;
};

} // namespace Framenote