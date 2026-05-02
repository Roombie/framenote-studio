#pragma once

#include <string>

#include "core/Document.h"
#include "io/ExportOptions.h"

namespace Framenote {

class PngExporter {
public:
    static bool exportFrame(
        const Document& doc,
        int frameIndex,
        const std::string& path,
        std::string& outError,
        int scale = 1
    );

    static bool exportFrame(
        const Document& doc,
        int frameIndex,
        const std::string& path,
        std::string& outError,
        const ExportRasterOptions& opts
    );

    static bool exportSequence(
        const Document& doc,
        const std::string& basePath,
        std::string& outError,
        int scale = 1
    );

    static bool exportSequence(
        const Document& doc,
        const std::string& basePath,
        std::string& outError,
        const ExportRasterOptions& opts
    );
};

} // namespace Framenote