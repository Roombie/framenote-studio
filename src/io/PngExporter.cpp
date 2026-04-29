#include "io/PngExporter.h"
#include "core/Frame.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <stb_image_write.h>

namespace Framenote {

// Convert the visible canvas region from internal BGRA memory to PNG RGBA.
// This uses frame.bufferWidth() as the source stride because a frame may keep
// a larger backing buffer after the canvas was resized smaller.
static std::vector<uint8_t> toVisibleRGBA(const Frame& frame, int visibleW, int visibleH) {
    const int bufW = frame.bufferWidth();
    const int bufH = frame.bufferHeight();

    std::vector<uint8_t> rgba(static_cast<size_t>(visibleW * visibleH * 4), 0);
    const auto& src = frame.pixels();

    const int copyW = std::min(visibleW, bufW);
    const int copyH = std::min(visibleH, bufH);

    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            size_t srcIdx = static_cast<size_t>((y * bufW     + x) * 4);
            size_t dstIdx = static_cast<size_t>((y * visibleW + x) * 4);

            // Internal BGRA -> PNG RGBA
            rgba[dstIdx + 0] = src[srcIdx + 2]; // R
            rgba[dstIdx + 1] = src[srcIdx + 1]; // G
            rgba[dstIdx + 2] = src[srcIdx + 0]; // B
            rgba[dstIdx + 3] = src[srcIdx + 3]; // A
        }
    }

    return rgba;
}

bool PngExporter::exportFrame(const Document& doc, int frameIndex,
                              const std::string& path, std::string& outError) {
    if (frameIndex < 0 || frameIndex >= doc.frameCount()) {
        outError = "Invalid frame index";
        return false;
    }

    const Frame& frame = doc.frame(frameIndex);

    int w = doc.canvasSize().width;
    int h = doc.canvasSize().height;

    auto rgba = toVisibleRGBA(frame, w, h);

    if (!stbi_write_png(path.c_str(), w, h, 4, rgba.data(), w * 4)) {
        outError = "Failed to write PNG: " + path;
        return false;
    }

    return true;
}

bool PngExporter::exportSequence(const Document& doc,
                                  const std::string& basePath,
                                  std::string& outError) {
    // basePath = "C:/my_animation/frame" -> produces frame_0001.png etc.
    for (int i = 0; i < doc.frameCount(); ++i) {
        char numbered[16];
        std::snprintf(numbered, sizeof(numbered), "_%04d.png", i + 1);

        std::string path = basePath + numbered;

        if (!exportFrame(doc, i, path, outError))
            return false;
    }

    return true;
}

} // namespace Framenote