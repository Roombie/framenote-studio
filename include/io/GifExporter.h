#pragma once

#include <string>

#include "core/Document.h"
#include "io/ExportOptions.h"

namespace Framenote {

class GifExporter {
public:
    struct Options {
        int fps = 8;
        bool loop = true;
        int scale = 1;

        ExportBackgroundMode background = ExportBackgroundMode::Transparent;

        bool useFrameRange = false;
        int startFrame = 0; // zero-based
        int endFrame = 0;   // zero-based, inclusive
    };

    static bool exportGif(
        const Document& doc,
        const std::string& outputPath,
        const Options& opts,
        std::string& outError
    );
};

} // namespace Framenote