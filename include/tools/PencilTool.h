#pragma once

#include "tools/ToolManager.h"
#include "tools/SelectionPixelClip.h"
#include <cstdint>

namespace Framenote {

class Selection;

class PencilTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Pencil; }
    const char* name() const override { return "Pencil"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

private:
    void drawBrush(
        Document& doc,
        int frameIndex,
        int x,
        int y,
        int size,
        const Selection* selection
    );

    void drawLine(
        Document& doc,
        int frameIndex,
        int x0,
        int y0,
        int x1,
        int y1,
        int size,
        const Selection* selection
    );

    int      m_lastX = -1;
    int      m_lastY = -1;
    bool     m_drawing = false;
    uint32_t m_drawColor = 0xFF000000;
};

} // namespace Framenote