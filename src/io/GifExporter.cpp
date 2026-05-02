#include "io/GifExporter.h"

#include "io/ExportRasterUtils.h"

#define GIF_FLIP_VERT
#include <gif.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace Framenote {
namespace {

ExportRasterOptions toRasterOptions(const GifExporter::Options& opts) {
    ExportRasterOptions raster;
    raster.scale = opts.scale;
    raster.background = opts.background;
    raster.useFrameRange = opts.useFrameRange;
    raster.startFrame = opts.startFrame;
    raster.endFrame = opts.endFrame;
    return raster;
}

bool gifBeginWithLoopOption(
    GifWriter* writer,
    const char* filename,
    uint32_t width,
    uint32_t height,
    uint32_t delay,
    bool loop
) {
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

    // Minimal global color table.
    fputc(0, writer->f);
    fputc(0, writer->f);
    fputc(0, writer->f);

    fputc(0, writer->f);
    fputc(0, writer->f);
    fputc(0, writer->f);

    if (loop) {
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

} // namespace

bool GifExporter::exportGif(
    const Document& doc,
    const std::string& outputPath,
    const Options& opts,
    std::string& outError
) {
    const int scale = std::clamp(opts.scale, 1, 64);
    const int fps = std::clamp(opts.fps, 1, 60);

    ExportRasterOptions rasterOpts = toRasterOptions(opts);
    rasterOpts.scale = scale;

    int startFrame = 0;
    int endFrame = 0;

    if (!ExportRasterUtils::normalizeFrameRange(
            doc,
            rasterOpts,
            startFrame,
            endFrame,
            outError)) {
        return false;
    }

    int canvasW = doc.canvasSize().width;
    int canvasH = doc.canvasSize().height;

    int outW = canvasW * scale;
    int outH = canvasH * scale;

    int delay = std::max(
        1,
        static_cast<int>(std::round(100.0 / static_cast<double>(fps)))
    );

    GifWriter writer = {};

    if (!gifBeginWithLoopOption(
            &writer,
            outputPath.c_str(),
            static_cast<uint32_t>(outW),
            static_cast<uint32_t>(outH),
            static_cast<uint32_t>(delay),
            opts.loop)) {
        outError = "Could not open output file: " + outputPath;
        return false;
    }

    std::vector<uint8_t> frameBuffer;

    for (int i = startFrame; i <= endFrame; ++i) {
        ExportRasterUtils::writeScaledVisibleRGBA(
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
                static_cast<uint32_t>(outW),
                static_cast<uint32_t>(outH),
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