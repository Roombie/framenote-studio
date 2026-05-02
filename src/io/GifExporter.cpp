#include "io/GifExporter.h"

#include "core/Frame.h"

#define GIF_FLIP_VERT
#include <gif.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

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
    const GifExporter::Options& opts,
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

bool gifBeginWithLoopOption(
    GifWriter* writer,
    const char* filename,
    uint32_t width,
    uint32_t height,
    uint32_t delay,
    bool loop,
    int32_t bitDepth = 8,
    bool dither = false
) {
    (void)bitDepth;
    (void)dither;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    writer->f = nullptr;
    fopen_s(&writer->f, filename, "wb");
#else
    writer->f = std::fopen(filename, "wb");
#endif

    if (!writer->f)
        return false;

    writer->firstFrame = true;
    writer->oldImage = static_cast<uint8_t*>(GIF_MALLOC(width * height * 4));

    if (!writer->oldImage) {
        std::fclose(writer->f);
        writer->f = nullptr;
        return false;
    }

    fputs("GIF89a", writer->f);

    fputc(width & 0xff, writer->f);
    fputc((width >> 8) & 0xff, writer->f);
    fputc(height & 0xff, writer->f);
    fputc((height >> 8) & 0xff, writer->f);

    fputc(0xf0, writer->f);
    fputc(0, writer->f);
    fputc(0, writer->f);

    fputc(0, writer->f);
    fputc(0, writer->f);
    fputc(0, writer->f);

    fputc(0, writer->f);
    fputc(0, writer->f);
    fputc(0, writer->f);

    if (delay != 0 && loop) {
        fputc(0x21, writer->f);
        fputc(0xff, writer->f);
        fputc(11, writer->f);
        fputs("NETSCAPE2.0", writer->f);
        fputc(3, writer->f);

        fputc(1, writer->f);
        fputc(0, writer->f);
        fputc(0, writer->f);

        fputc(0, writer->f);
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

} // namespace

bool GifExporter::exportGif(
    const Document& doc,
    const std::string& outputPath,
    const Options& opts,
    std::string& outError
) {
    const int scale = std::clamp(opts.scale, 1, 64);
    const int fps = std::clamp(opts.fps, 1, 60);

    int startFrame = 0;
    int endFrame = 0;

    if (!normalizeFrameRange(doc, opts, startFrame, endFrame, outError))
        return false;

    const int canvasW = doc.canvasSize().width;
    const int canvasH = doc.canvasSize().height;

    const int w = canvasW * scale;
    const int h = canvasH * scale;

    const int delay = std::max(
        1,
        static_cast<int>(std::round(100.0 / static_cast<double>(fps)))
    );

    GifWriter writer = {};

    if (!gifBeginWithLoopOption(
            &writer,
            outputPath.c_str(),
            static_cast<uint32_t>(w),
            static_cast<uint32_t>(h),
            static_cast<uint32_t>(delay),
            opts.loop)) {
        outError = "Could not open output file: " + outputPath;
        return false;
    }

    std::vector<uint8_t> frameBuffer;

    for (int i = startFrame; i <= endFrame; ++i) {
        writeScaledVisibleRGBA(
            doc.frame(i),
            canvasW,
            canvasH,
            scale,
            opts.background,
            frameBuffer
        );

        if (!GifWriteFrame(
                &writer,
                frameBuffer.data(),
                static_cast<uint32_t>(w),
                static_cast<uint32_t>(h),
                static_cast<uint32_t>(delay))) {
            GifEnd(&writer);
            outError = "Failed to write GIF frame.";
            return false;
        }
    }

    GifEnd(&writer);
    return true;
}

} // namespace Framenote