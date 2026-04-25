#include "io/PngExporter.h"
#include "core/Frame.h"
#include <cstdio>

#include <stb_image_write.h>

namespace Framenote {

// Convert our internal ARGB little-endian pixel format to RGBA for PNG output
static std::vector<uint8_t> toRGBA(const Frame& frame) {
    int w = frame.bufferWidth();
    int h = frame.bufferHeight();
    std::vector<uint8_t> rgba(static_cast<size_t>(w * h * 4));
    const auto& src = frame.pixels();

    for (int i = 0; i < w * h; ++i) {
        // Our format in memory: [B, G, R, A] (little-endian ARGB8888)
        rgba[i * 4 + 0] = src[i * 4 + 2]; // R
        rgba[i * 4 + 1] = src[i * 4 + 1]; // G
        rgba[i * 4 + 2] = src[i * 4 + 0]; // B
        rgba[i * 4 + 3] = src[i * 4 + 3]; // A
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
    auto rgba = toRGBA(frame);

    int w = doc.canvasSize().width;
    int h = doc.canvasSize().height;

    if (!stbi_write_png(path.c_str(), w, h, 4, rgba.data(), w * 4)) {
        outError = "Failed to write PNG: " + path;
        return false;
    }
    return true;
}

bool PngExporter::exportSequence(const Document& doc,
                                  const std::string& basePath,
                                  std::string& outError) {
    // basePath = "C:/my_animation/frame" → produces frame_0001.png etc
    for (int i = 0; i < doc.frameCount(); ++i) {
        char numbered[16];
        snprintf(numbered, sizeof(numbered), "_%04d.png", i + 1);
        std::string path = basePath + numbered;
        if (!exportFrame(doc, i, path, outError))
            return false;
    }
    return true;
}

} // namespace Framenote