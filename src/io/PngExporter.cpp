#include "io/PngExporter.h"

#include "io/ExportRasterUtils.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <stb_image_write.h>

namespace Framenote {

bool PngExporter::exportFrame(
    const Document& doc,
    int frameIndex,
    const std::string& path,
    std::string& outError,
    int scale
) {
    ExportRasterOptions opts;
    opts.scale = scale;

    return exportFrame(doc, frameIndex, path, outError, opts);
}

bool PngExporter::exportFrame(
    const Document& doc,
    int frameIndex,
    const std::string& path,
    std::string& outError,
    const ExportRasterOptions& opts
) {
    if (frameIndex < 0 || frameIndex >= doc.frameCount()) {
        outError = "Invalid frame index.";
        return false;
    }

    int scale = std::clamp(opts.scale, 1, 64);

    int canvasW = doc.canvasSize().width;
    int canvasH = doc.canvasSize().height;

    std::vector<uint8_t> rgba;

    ExportRasterUtils::writeScaledVisibleRGBA(
        doc.frame(frameIndex),
        canvasW,
        canvasH,
        scale,
        opts.background,
        rgba
    );

    int outW = canvasW * scale;
    int outH = canvasH * scale;

    if (!stbi_write_png(path.c_str(), outW, outH, 4, rgba.data(), outW * 4)) {
        outError = "Failed to write PNG: " + path;
        return false;
    }

    return true;
}

bool PngExporter::exportSequence(
    const Document& doc,
    const std::string& basePath,
    std::string& outError,
    int scale
) {
    ExportRasterOptions opts;
    opts.scale = scale;

    return exportSequence(doc, basePath, outError, opts);
}

bool PngExporter::exportSequence(
    const Document& doc,
    const std::string& basePath,
    std::string& outError,
    const ExportRasterOptions& opts
) {
    int startFrame = 0;
    int endFrame = 0;

    if (!ExportRasterUtils::normalizeFrameRange(
            doc,
            opts,
            startFrame,
            endFrame,
            outError)) {
        return false;
    }

    for (int i = startFrame; i <= endFrame; ++i) {
        char suffix[16];
        std::snprintf(suffix, sizeof(suffix), "_%04d.png", i + 1);

        std::string path = basePath + suffix;

        if (!exportFrame(doc, i, path, outError, opts))
            return false;
    }

    return true;
}

} // namespace Framenote