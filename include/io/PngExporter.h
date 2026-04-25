#pragma once
#include <string>
#include "core/Document.h"

namespace Framenote {

class PngExporter {
public:
    // Export single frame as PNG
    static bool exportFrame(const Document& doc, int frameIndex,
                            const std::string& path, std::string& outError);

    // Export all frames as numbered PNGs: name_0001.png, name_0002.png...
    static bool exportSequence(const Document& doc,
                               const std::string& basePath,
                               std::string& outError);
};

} // namespace Framenote
