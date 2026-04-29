#include "io/FileManager.h"
#include "core/Frame.h"
#include <nlohmann/json.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <sstream>
#include <vector>

#include <algorithm>
#include <cstdio>

using json = nlohmann::json;

namespace Framenote {

// ── PNG encode/decode helpers ─────────────────────────────────────────────────
//
// Internal Frame memory uses SDL_PIXELFORMAT_ARGB8888 on little-endian machines,
// which is byte-ordered as BGRA. PNG/stb expects RGBA. These helpers always
// crop to the visible canvas size and convert channel order explicitly.

static std::vector<uint8_t> frameToVisibleRGBA(const Frame& frame) {
    const int w = frame.width();
    const int h = frame.height();

    const int bufW = frame.bufferWidth();
    const int bufH = frame.bufferHeight();

    std::vector<uint8_t> rgba(static_cast<size_t>(w * h * 4), 0);
    const auto& src = frame.pixels();

    const int copyW = std::min(w, bufW);
    const int copyH = std::min(h, bufH);

    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            size_t srcIdx = static_cast<size_t>((y * bufW + x) * 4);
            size_t dstIdx = static_cast<size_t>((y * w    + x) * 4);

            // Internal BGRA -> external RGBA
            rgba[dstIdx + 0] = src[srcIdx + 2]; // R
            rgba[dstIdx + 1] = src[srcIdx + 1]; // G
            rgba[dstIdx + 2] = src[srcIdx + 0]; // B
            rgba[dstIdx + 3] = src[srcIdx + 3]; // A
        }
    }

    return rgba;
}

static std::vector<uint8_t> rgbaToInternalBGRA(const uint8_t* rgba,
                                               int w, int h) {
    std::vector<uint8_t> bgra(static_cast<size_t>(w * h * 4), 0);

    for (int i = 0; i < w * h; ++i) {
        bgra[i * 4 + 0] = rgba[i * 4 + 2]; // B
        bgra[i * 4 + 1] = rgba[i * 4 + 1]; // G
        bgra[i * 4 + 2] = rgba[i * 4 + 0]; // R
        bgra[i * 4 + 3] = rgba[i * 4 + 3]; // A
    }

    return bgra;
}

static std::vector<uint8_t> encodePNG(const Frame& frame) {
    std::vector<uint8_t> out;

    auto writeFunc = [](void* ctx, void* data, int size) {
        auto* buf = static_cast<std::vector<uint8_t>*>(ctx);
        auto* bytes = static_cast<uint8_t*>(data);
        buf->insert(buf->end(), bytes, bytes + size);
    };

    auto rgba = frameToVisibleRGBA(frame);

    stbi_write_png_to_func(
        writeFunc,
        &out,
        frame.width(),
        frame.height(),
        4,
        rgba.data(),
        frame.width() * 4
    );

    return out;
}

static bool decodePNG(const std::vector<uint8_t>& pngData, Frame& outFrame) {
    int w = 0;
    int h = 0;
    int channels = 0;

    uint8_t* data = stbi_load_from_memory(
        pngData.data(),
        static_cast<int>(pngData.size()),
        &w,
        &h,
        &channels,
        4
    );

    (void)channels;

    if (!data)
        return false;

    auto bgra = rgbaToInternalBGRA(data, w, h);

    stbi_image_free(data);

    outFrame.restoreBuffer(std::move(bgra), w, h);
    outFrame.setVisibleSize(w, h);

    return true;
}

// ── Base64 encode/decode (no external dep needed) ─────────────────────────────

static const char B64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::vector<uint8_t>& in) {
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out += B64_CHARS[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }
    if (valb > -6) out += B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F];
    while (out.size() % 4) out += '=';
    return out;
}

static std::vector<uint8_t> base64Decode(const std::string& in) {
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(uint8_t)B64_CHARS[i]] = i;

    std::vector<uint8_t> out;
    int val = 0, valb = -8;
    for (uint8_t c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return out;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool FileManager::save(const Document& doc, const std::string& path,
                       std::string& outError) {
    json j;
    j["version"]          = FORMAT_VERSION;
    j["canvas"]["width"]  = doc.canvasSize().width;
    j["canvas"]["height"] = doc.canvasSize().height;
    j["fps"]              = doc.fps();

    // Palette
    json palette = json::array();
    for (int i = 0; i < doc.palette().size(); ++i) {
        auto c = doc.palette().color(i);
        char hex[10];
        snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
        palette.push_back(std::string(hex));
    }
    j["palette"] = palette;

    // Frames — each encoded as base64 PNG
    json frames = json::array();
    for (int i = 0; i < doc.frameCount(); ++i) {
        auto png = encodePNG(doc.frame(i));
        frames.push_back({ {"pixels", base64Encode(png)} });
    }
    j["frames"] = frames;

    std::ofstream file(path);
    if (!file.is_open()) {
        outError = "Cannot open file for writing: " + path;
        return false;
    }
    file << j.dump(2);
    return true;
}

std::unique_ptr<Document> FileManager::load(const std::string& path,
                                             std::string& outError) {
    std::ifstream file(path);
    if (!file.is_open()) {
        outError = "Cannot open file: " + path;
        return nullptr;
    }

    json j;
    try { j = json::parse(file); }
    catch (const json::exception& e) {
        outError = std::string("JSON parse error: ") + e.what();
        return nullptr;
    }

    int w = j["canvas"]["width"];
    int h = j["canvas"]["height"];
    auto doc = std::make_unique<Document>(w, h);
    doc->setFps(j["fps"].get<int>());

    // Palette
    auto& palette = j["palette"];
    for (int i = 0; i < std::min((int)palette.size(), doc->palette().size()); ++i) {
        std::string hex = palette[i];
        Color c;
        sscanf(hex.c_str(), "#%02hhX%02hhX%02hhX%02hhX", &c.r, &c.g, &c.b, &c.a);
        doc->palette().color(i) = c;
    }

    // Frames
    for (auto& fj : j["frames"]) {
        int idx = doc->addFrame();

        auto pngData = base64Decode(fj["pixels"].get<std::string>());

        if (!decodePNG(pngData, doc->frame(idx))) {
            outError = "Failed to decode frame PNG data";
            return nullptr;
        }

        doc->frame(idx).setVisibleSize(w, h);
    }

    doc->setFilePath(path);
    doc->clearDirty();
    return doc;
}

} // namespace Framenote