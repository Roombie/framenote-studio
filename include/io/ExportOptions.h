#pragma once

namespace Framenote {

enum class ExportBackgroundMode {
    Transparent = 0,
    White,
    Black
};

struct ExportRasterOptions {
    int scale = 1;

    ExportBackgroundMode background = ExportBackgroundMode::Transparent;

    bool useFrameRange = false;
    int startFrame = 0; // zero-based
    int endFrame = 0;   // zero-based, inclusive
};

} // namespace Framenote