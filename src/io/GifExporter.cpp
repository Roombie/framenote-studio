#include "io/GifExporter.h"

// gif-h — single header GIF encoder
// https://github.com/charlietangora/gif-h
#define GIF_FLIP_VERT
#include <gif.h>

#include <vector>

namespace Framenote {

bool GifExporter::exportGif(const Document& doc,
                             const std::string& outputPath,
                             const Options& opts,
                             std::string& outError) {
    int w = doc.canvasSize().width  * opts.scale;
    int h = doc.canvasSize().height * opts.scale;
    int delay = 100 / opts.fps; // GIF delay is in 1/100ths of a second

    GifWriter writer = {};
    if (!GifBegin(&writer, outputPath.c_str(), w, h, delay)) {
        outError = "Could not open output file: " + outputPath;
        return false;
    }

    std::vector<uint8_t> frameBuffer(static_cast<size_t>(w * h * 4));

    for (int i = 0; i < doc.frameCount(); ++i) {
        const Frame& frame = doc.frame(i);
        const auto&  src   = frame.pixels();

        if (opts.scale == 1) {
            // Direct copy
            frameBuffer.assign(src.begin(), src.end());
        } else {
            // Nearest-neighbor upscale
            int srcW = doc.canvasSize().width;
            for (int py = 0; py < h; ++py) {
                for (int px = 0; px < w; ++px) {
                    int srcX = px / opts.scale;
                    int srcY = py / opts.scale;
                    size_t srcIdx  = static_cast<size_t>((srcY * srcW + srcX) * 4);
                    size_t destIdx = static_cast<size_t>((py   * w    + px  ) * 4);
                    frameBuffer[destIdx    ] = src[srcIdx    ];
                    frameBuffer[destIdx + 1] = src[srcIdx + 1];
                    frameBuffer[destIdx + 2] = src[srcIdx + 2];
                    frameBuffer[destIdx + 3] = src[srcIdx + 3];
                }
            }
        }

        GifWriteFrame(&writer, frameBuffer.data(), w, h, delay);
    }

    GifEnd(&writer);
    return true;
}

} // namespace Framenote
