#include "io/GifExporter.h"
#include "core/Frame.h"

// gif-h — single header GIF encoder
// https://github.com/charlietangora/gif-h
#define GIF_FLIP_VERT
#include <gif.h>

#include <algorithm>
#include <vector>

namespace Framenote {

static void writeScaledVisibleRGBA(const Frame& frame,
                                   int canvasW,
                                   int canvasH,
                                   int scale,
                                   std::vector<uint8_t>& out) {
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

            size_t srcIdx  = static_cast<size_t>((srcY * bufW + srcX) * 4);
            size_t destIdx = static_cast<size_t>((py   * outW + px  ) * 4);

            // Internal BGRA -> GIF encoder RGBA
            out[destIdx + 0] = src[srcIdx + 2]; // R
            out[destIdx + 1] = src[srcIdx + 1]; // G
            out[destIdx + 2] = src[srcIdx + 0]; // B
            out[destIdx + 3] = src[srcIdx + 3]; // A
        }
    }
}

bool GifExporter::exportGif(const Document& doc,
                             const std::string& outputPath,
                             const Options& opts,
                             std::string& outError) {
    const int scale = std::max(1, opts.scale);
    const int fps   = std::max(1, opts.fps);

    const int canvasW = doc.canvasSize().width;
    const int canvasH = doc.canvasSize().height;

    const int w = canvasW * scale;
    const int h = canvasH * scale;

    // GIF delay is in 1/100ths of a second.
    // Clamp to at least 1 so high FPS values do not become zero-delay animations.
    const int delay = std::max(1, 100 / fps);

    GifWriter writer = {};

    if (!GifBegin(&writer, outputPath.c_str(), w, h, delay)) {
        outError = "Could not open output file: " + outputPath;
        return false;
    }

    std::vector<uint8_t> frameBuffer;

    for (int i = 0; i < doc.frameCount(); ++i) {
        writeScaledVisibleRGBA(doc.frame(i), canvasW, canvasH, scale, frameBuffer);
        GifWriteFrame(&writer, frameBuffer.data(), w, h, delay);
    }

    GifEnd(&writer);
    return true;
}

} // namespace Framenote