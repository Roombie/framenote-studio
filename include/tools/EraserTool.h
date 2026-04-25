#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class EraserTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Eraser; }
    const char* name() const override { return "Eraser"; }

    void onPress  (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onDrag   (Document& doc, int frameIndex, const ToolEvent& e) override;
    void onRelease(Document& doc, int frameIndex, const ToolEvent& e) override;

private:
    void erasePixel(Document& doc, int frameIndex, int x, int y);
    void eraseLine (Document& doc, int frameIndex, int x0, int y0, int x1, int y1);

    int  m_lastX = -1;
    int  m_lastY = -1;
};

} // namespace Framenote
