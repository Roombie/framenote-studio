#include "tools/EyedropperTool.h"

namespace Framenote {

void EyedropperTool::onPress(Document& doc, int frameIndex, const ToolEvent& e) {
    int x = static_cast<int>(e.canvasX);
    int y = static_cast<int>(e.canvasY);
    uint32_t rgba = doc.frame(frameIndex).getPixel(x, y);

    // Find closest palette color or just select the raw color
    // For now, find the closest match in the palette
    uint8_t r = (rgba >> 16) & 0xFF;
    uint8_t g = (rgba >>  8) & 0xFF;
    uint8_t b =  rgba        & 0xFF;
    uint8_t a = (rgba >> 24) & 0xFF;

    if (a == 0) return; // Don't pick transparent

    int bestIdx = 1;
    int bestDist = INT_MAX;
    for (int i = 1; i < doc.palette().size(); ++i) {
        const Color& c = doc.palette().color(i);
        int dr = c.r - r;
        int dg = c.g - g;
        int db = c.b - b;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    if (e.rightDown && !e.leftDown)
        doc.palette().selectSecondaryIndex(bestIdx);
    else
        doc.palette().selectPrimaryIndex(bestIdx);
}

} // namespace Framenote