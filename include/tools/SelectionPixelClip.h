#pragma once

#include "core/Selection.h"

namespace Framenote {
namespace SelectionPixelClip {

inline bool canModifyPixel(const Selection* selection, int x, int y) {
    if (!selection || selection->isEmpty())
        return true;

    if (x < 0 || y < 0 ||
        x >= selection->width() ||
        y >= selection->height()) {
        return false;
    }

    return selection->isSelected(x, y);
}

} // namespace SelectionPixelClip
} // namespace Framenote