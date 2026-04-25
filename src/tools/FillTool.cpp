#include "tools/FillTool.h"
#include <queue>

namespace Framenote {

void FillTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    auto& frame = doc.frame(frameIndex);
    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);

    uint32_t targetColor = frame.getPixel(x, y);
    uint32_t fillColor   = doc.palette().selectedColor().toRGBA();

    if (targetColor == fillColor) return; // nothing to do

    // BFS flood fill — iterative to avoid stack overflow on large canvases
    std::queue<std::pair<int,int>> queue;
    queue.push({x, y});

    int w = frame.bufferWidth();
    int h = frame.bufferHeight();

    while (!queue.empty()) {
        auto [cx, cy] = queue.front();
        queue.pop();

        if (cx < 0 || cx >= w || cy < 0 || cy >= h) continue;
        if (frame.getPixel(cx, cy) != targetColor)   continue;

        frame.setPixel(cx, cy, fillColor);

        queue.push({cx + 1, cy});
        queue.push({cx - 1, cy});
        queue.push({cx, cy + 1});
        queue.push({cx, cy - 1});
    }

    doc.markDirty();
}

} // namespace Framenote
