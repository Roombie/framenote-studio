#pragma once

#include <string>
#include "core/Document.h"

namespace Framenote {

class GifExporter {
public:
    struct Options {
        int fps         = 8;
        bool loop       = true;
        int  scale      = 1;   // 1x, 2x, 4x upscale for small canvases
    };

    // Export the full animation as a GIF file.
    // Returns false on failure, sets outError.
    static bool exportGif(const Document& doc,
                          const std::string& outputPath,
                          const Options& opts,
                          std::string& outError);
};

} // namespace Framenote
