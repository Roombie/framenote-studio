#include "io/ExportRasterUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace Framenote {
namespace ExportRasterUtils {
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

} // namespace

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
    }
    else {
        startFrame = 0;
        endFrame = frameCount - 1;
    }

    if (startFrame < 0 ||
        endFrame < 0 ||
        startFrame >= frameCount ||
        endFrame >= frameCount ||
        startFrame > endFrame) {
        outError = "Invalid export frame range.";
        return false;
    }

    return true;
}

void writeScaledVisibleRGBA(
    const Frame& frame,
    int canvasW,
    int canvasH,
    int scale,
    ExportBackgroundMode background,
    std::vector<uint8_t>& out
) {
    scale = std::clamp(scale, 1, 64);

    const int outW = canvasW * scale;
    const int outH = canvasH * scale;

    const int bufW = frame.bufferWidth();
    const int bufH = frame.bufferHeight();

    const auto& src = frame.pixels();

    out.assign(static_cast<size_t>(outW * outH * 4), 0);

    for (int py = 0; py < outH; ++py) {
        int srcY = py / scale;

        if (srcY < 0 || srcY >= canvasH || srcY >= bufH)
            continue;

        for (int px = 0; px < outW; ++px) {
            int srcX = px / scale;

            if (srcX < 0 || srcX >= canvasW || srcX >= bufW)
                continue;

            size_t srcIdx = static_cast<size_t>((srcY * bufW + srcX) * 4);
            size_t dstIdx = static_cast<size_t>((py * outW + px) * 4);

            // Internal frame memory is BGRA.
            uint8_t r = src[srcIdx + 2];
            uint8_t g = src[srcIdx + 1];
            uint8_t b = src[srcIdx + 0];
            uint8_t a = src[srcIdx + 3];

            applyBackground(r, g, b, a, background);

            out[dstIdx + 0] = r;
            out[dstIdx + 1] = g;
            out[dstIdx + 2] = b;
            out[dstIdx + 3] = a;
        }
    }

    if (background != ExportBackgroundMode::Transparent) {
        uint8_t bg = background == ExportBackgroundMode::White ? 255 : 0;

        for (size_t i = 0; i < out.size(); i += 4) {
            if (out[i + 3] != 0)
                continue;

            out[i + 0] = bg;
            out[i + 1] = bg;
            out[i + 2] = bg;
            out[i + 3] = 255;
        }
    }
}

} // namespace ExportRasterUtils
} // namespace Framenote