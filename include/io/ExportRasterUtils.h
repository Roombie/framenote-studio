#pragma once

#include "core/Document.h"
#include "core/Frame.h"
#include "io/ExportOptions.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Framenote {
namespace ExportRasterUtils {

bool normalizeFrameRange(
    const Document& doc,
    const ExportRasterOptions& opts,
    int& startFrame,
    int& endFrame,
    std::string& outError
);

void writeScaledVisibleRGBA(
    const Frame& frame,
    int canvasW,
    int canvasH,
    int scale,
    ExportBackgroundMode background,
    std::vector<uint8_t>& out
);

} // namespace ExportRasterUtils
} // namespace Framenote