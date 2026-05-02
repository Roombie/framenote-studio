#include "tools/FillTool.h"

#include "tools/SelectionPixelClip.h"

#include <queue>
#include <utility>

namespace Framenote {

void FillTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    auto& frame = doc.frame(frameIndex);

    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);

    int w = frame.width();
    int h = frame.height();

    if (x < 0 || y < 0 || x >= w || y >= h)
        return;

    if (!SelectionPixelClip::canModifyPixel(e.selection, x, y))
        return;

    uint32_t targetColor = frame.getPixel(x, y);

    uint32_t fillColor =
        (e.rightDown && !e.leftDown)
            ? doc.palette().secondaryColor().toRGBA()
            : doc.palette().primaryColor().toRGBA();

    if (targetColor == fillColor)
        return;

    std::queue<std::pair<int, int>> queue;
    queue.push({x, y});

    while (!queue.empty()) {
        auto [cx, cy] = queue.front();
        queue.pop();

        if (cx < 0 || cx >= w || cy < 0 || cy >= h)
            continue;

        if (!SelectionPixelClip::canModifyPixel(e.selection, cx, cy))
            continue;

        if (frame.getPixel(cx, cy) != targetColor)
            continue;

        frame.setPixel(cx, cy, fillColor);

        queue.push({cx + 1, cy});
        queue.push({cx - 1, cy});
        queue.push({cx, cy + 1});
        queue.push({cx, cy - 1});
    }

    doc.markDirty();
}

} // namespace Framenote