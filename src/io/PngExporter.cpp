#include "io/PngExporter.h"
#include "core/Frame.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <stb_image_write.h>

namespace Framenote {

// Convert the visible canvas region from internal BGRA memory to PNG RGBA,
// with optional nearest-neighbour upscale.
static std::vector<uint8_t> toVisibleRGBA(const Frame& frame,
                                           int visibleW, int visibleH,
                                           int scale) {
    const int bufW   = frame.bufferWidth();
    const int bufH   = frame.bufferHeight();
    const int outW   = visibleW * scale;
    const int outH   = visibleH * scale;
    const int copyW  = std::min(visibleW, bufW);
    const int copyH  = std::min(visibleH, bufH);
    const auto& src  = frame.pixels();

    std::vector<uint8_t> rgba(static_cast<size_t>(outW * outH * 4), 0);

    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            size_t srcIdx = static_cast<size_t>((y * bufW + x) * 4);

            // Internal BGRA -> RGBA
            uint8_t r = src[srcIdx + 2];
            uint8_t g = src[srcIdx + 1];
            uint8_t b = src[srcIdx + 0];
            uint8_t a = src[srcIdx + 3];

            // Stamp scale×scale block in output
            for (int sy = 0; sy < scale; ++sy) {
                for (int sx = 0; sx < scale; ++sx) {
                    size_t dstIdx = static_cast<size_t>(
                        ((y * scale + sy) * outW + (x * scale + sx)) * 4);
                    rgba[dstIdx + 0] = r;
                    rgba[dstIdx + 1] = g;
                    rgba[dstIdx + 2] = b;
                    rgba[dstIdx + 3] = a;
                }
            }
        }
    }

    return rgba;
}

bool PngExporter::exportFrame(const Document& doc, int frameIndex,
                               const std::string& path, std::string& outError,
                               int scale) {
    if (frameIndex < 0 || frameIndex >= doc.frameCount()) {
        outError = "Invalid frame index";
        return false;
    }
    if (scale < 1) scale = 1;

    const Frame& frame = doc.frame(frameIndex);
    int w = doc.canvasSize().width;
    int h = doc.canvasSize().height;

    auto rgba = toVisibleRGBA(frame, w, h, scale);

    int outW = w * scale;
    int outH = h * scale;

    if (!stbi_write_png(path.c_str(), outW, outH, 4, rgba.data(), outW * 4)) {
        outError = "Failed to write PNG: " + path;
        return false;
    }

    return true;
}

bool PngExporter::exportSequence(const Document& doc,
                                  const std::string& basePath,
                                  std::string& outError,
                                  int scale) {
    for (int i = 0; i < doc.frameCount(); ++i) {
        char numbered[16];
        std::snprintf(numbered, sizeof(numbered), "_%04d.png", i + 1);
        std::string path = basePath + numbered;
        if (!exportFrame(doc, i, path, outError, scale))
            return false;
    }
    return true;
}

} // namespace Framenote