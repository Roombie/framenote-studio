#include "io/PngExporter.h"

#include "core/Frame.h"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <vector>

#include <stb_image_write.h>

namespace Framenote {
namespace {

uint8_t compositeChannel(uint8_t src, uint8_t alpha, uint8_t bg) {
    return static_cast<uint8_t>(
        (static_cast<int>(src) * alpha +
         static_cast<int>(bg) * (255 - alpha) + 127) / 255
    );
}

void applyBackground(
    uint8_t& r,
    uint8_t& g,
    uint8_t& b,
    uint8_t& a,
    ExportBackgroundMode background
) {
    if (background == ExportBackgroundMode::Transparent)
        return;

    uint8_t bg = background == ExportBackgroundMode::White ? 255 : 0;

    r = compositeChannel(r, a, bg);
    g = compositeChannel(g, a, bg);
    b = compositeChannel(b, a, bg);
    a = 255;
}

bool normalizeFrameRange(
    const Document& doc,
    const ExportRasterOptions& opts,
    int& startFrame,
    int& endFrame,
    std::string& outError
) {
    int frameCount = doc.frameCount();

    if (frameCount <= 0) {
        outError = "Document has no frames to export.";
        return false;
    }

    if (opts.useFrameRange) {
        startFrame = opts.startFrame;
        endFrame = opts.endFrame;
    } else {
        startFrame = 0;
        endFrame = frameCount - 1;
    }

    if (startFrame < 0 || endFrame < 0 ||
        startFrame >= frameCount || endFrame >= frameCount ||
        startFrame > endFrame) {
        outError = "Invalid export frame range.";
        return false;
    }

    return true;
}

std::vector<uint8_t> toVisibleRGBA(
    const Frame& frame,
    int visibleW,
    int visibleH,
    int scale,
    ExportBackgroundMode background
) {
    const int bufW = frame.bufferWidth();
    const int bufH = frame.bufferHeight();

    const int outW = visibleW * scale;
    const int outH = visibleH * scale;

    const int copyW = std::min(visibleW, bufW);
    const int copyH = std::min(visibleH, bufH);

    const auto& src = frame.pixels();

    std::vector<uint8_t> rgba(static_cast<size_t>(outW * outH * 4), 0);

    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            size_t srcIdx = static_cast<size_t>((y * bufW + x) * 4);

            uint8_t r = src[srcIdx + 2];
            uint8_t g = src[srcIdx + 1];
            uint8_t b = src[srcIdx + 0];
            uint8_t a = src[srcIdx + 3];

            applyBackground(r, g, b, a, background);

            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    size_t dstIdx = static_cast<size_t>(
                        ((y * scale + sy) * outW + (x * scale + sx)) * 4
                    );

                    rgba[dstIdx + 0] = r;
                    rgba[dstIdx + 1] = g;
                    rgba[dstIdx + 2] = b;
                    rgba[dstIdx + 3] = a;
                }
            }
        }
    }

    if (background != ExportBackgroundMode::Transparent) {
        uint8_t bg = background == ExportBackgroundMode::White ? 255 : 0;

        for (size_t i = 0; i < rgba.size(); i += 4) {
            if (rgba[i + 3] != 0)
                continue;

            rgba[i + 0] = bg;
            rgba[i + 1] = bg;
            rgba[i + 2] = bg;
            rgba[i + 3] = 255;
        }
    }

    return rgba;
}

} // namespace

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

    const Frame& frame = doc.frame(frameIndex);

    int w = doc.canvasSize().width;
    int h = doc.canvasSize().height;

    auto rgba = toVisibleRGBA(frame, w, h, scale, opts.background);

    int outW = w * scale;
    int outH = h * scale;

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

    if (!normalizeFrameRange(doc, opts, startFrame, endFrame, outError))
        return false;

    for (int i = startFrame; i <= endFrame; ++i) {
        char numbered[16];
        std::snprintf(numbered, sizeof(numbered), "_%04d.png", i + 1);

        std::string path = basePath + numbered;

        if (!exportFrame(doc, i, path, outError, opts))
            return false;
    }

    return true;
}

} // namespace Framenote