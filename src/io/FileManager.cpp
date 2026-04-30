#include "io/FileManager.h"
#include "core/Frame.h"

#include <nlohmann/json.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
                                               int w,
                                               int h) {
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

// ── Base64 encode/decode ─────────────────────────────────────────────────────

static const char B64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::vector<uint8_t>& in) {
    std::string out;

    int val = 0;
    int valb = -6;

    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;

        while (valb >= 0) {
            out += B64_CHARS[(val >> valb) & 0x3F];
            valb -= 6;
        }
    }

    if (valb > -6) {
        out += B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F];
    }

    while (out.size() % 4) {
        out += '=';
    }

    return out;
}

static std::vector<uint8_t> base64Decode(const std::string& in) {
    std::vector<int> table(256, -1);

    for (int i = 0; i < 64; ++i) {
        table[static_cast<uint8_t>(B64_CHARS[i])] = i;
    }

    std::vector<uint8_t> out;

    int val = 0;
    int valb = -8;

    for (uint8_t c : in) {
        if (table[c] == -1)
            break;

        val = (val << 6) + table[c];
        valb += 6;

        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return out;
}

// ── JSON helpers ──────────────────────────────────────────────────────────────

static bool readInt(const json& obj, const char* key, int& out) {
    if (!obj.contains(key))
        return false;

    if (!obj[key].is_number_integer())
        return false;

    out = obj[key].get<int>();
    return true;
}

static bool readHexColor(const std::string& hex, Color& outColor) {
    Color c {0, 0, 0, 255};

    int result = std::sscanf(
        hex.c_str(),
        "#%02hhX%02hhX%02hhX%02hhX",
        &c.r,
        &c.g,
        &c.b,
        &c.a
    );

    if (result != 4)
        return false;

    outColor = c;
    return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool FileManager::save(const Document& doc,
                       const std::string& path,
                       std::string& outError) {
    // Enforce .framenote extension
    std::string finalPath = path;
    if (finalPath.size() < 11 ||
        finalPath.substr(finalPath.size() - 11) != ".framenote") {
        finalPath += ".framenote";
    }

    json j;

    j["version"]          = FORMAT_VERSION;
    j["canvas"]["width"]  = doc.canvasSize().width;
    j["canvas"]["height"] = doc.canvasSize().height;
    j["fps"]              = doc.fps();

    // Palette
    json palette = json::array();
    for (int i = 0; i < doc.palette().size(); ++i) {
        Color c = doc.palette().color(i);
        char hex[10];
        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X",
                      c.r, c.g, c.b, c.a);
        palette.push_back(std::string(hex));
    }
    j["palette"] = palette;

    // Frames — each encoded as base64 PNG
    json frames = json::array();
    for (int i = 0; i < doc.frameCount(); ++i) {
        auto png = encodePNG(doc.frame(i));
        if (png.empty()) {
            outError = "Failed to encode frame " + std::to_string(i) + " as PNG";
            return false;
        }
        frames.push_back({{"pixels", base64Encode(png)}});
    }
    j["frames"] = frames;

    // Atomic save: write to a temp file first, then rename.
    // This ensures a failed write never corrupts the existing file.
    std::string tmpPath = finalPath + ".tmp";

    {
        std::ofstream tmp(tmpPath, std::ios::binary | std::ios::trunc);
        if (!tmp.is_open()) {
            outError = "Cannot open temp file for writing: " + tmpPath;
            return false;
        }
        tmp << j.dump(2);
        if (!tmp.good()) {
            tmp.close();
            std::remove(tmpPath.c_str());
            outError = "Failed while writing file: " + tmpPath;
            return false;
        }
    } // flush and close before rename

    // Replace the target file atomically
    if (std::rename(tmpPath.c_str(), finalPath.c_str()) != 0) {
        std::remove(tmpPath.c_str());
        outError = "Failed to finalize save (rename failed): " + finalPath;
        return false;
    }

    return true;
}

std::unique_ptr<Document> FileManager::load(const std::string& path,
                                            std::string& outError) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        outError = "Cannot open file: " + path;
        return nullptr;
    }

    json j;

    try {
        j = json::parse(file);
    }
    catch (const json::exception& e) {
        outError = std::string("JSON parse error: ") + e.what();
        return nullptr;
    }

    try {
        if (!j.is_object()) {
            outError = "Invalid .framenote file: root must be a JSON object";
            return nullptr;
        }

        // Version validation. Missing version is treated as version 1 for older files.
        int fileVersion = 1;

        if (j.contains("version")) {
            if (!j["version"].is_number_integer()) {
                outError = "Invalid .framenote file: version must be an integer";
                return nullptr;
            }

            fileVersion = j["version"].get<int>();
        }

        if (fileVersion < 1 || fileVersion > FORMAT_VERSION) {
            outError = "Unsupported .framenote format version";
            return nullptr;
        }

        // Canvas validation
        if (!j.contains("canvas") || !j["canvas"].is_object()) {
            outError = "Invalid .framenote file: missing canvas data";
            return nullptr;
        }

        int w = 0;
        int h = 0;

        if (!readInt(j["canvas"], "width", w) ||
            !readInt(j["canvas"], "height", h)) {
            outError = "Invalid .framenote file: canvas width/height must be integers";
            return nullptr;
        }

        if (w <= 0 || h <= 0 || w > 8192 || h > 8192) {
            outError = "Invalid .framenote file: canvas size is out of range";
            return nullptr;
        }

        auto doc = std::make_unique<Document>(w, h);

        // FPS validation
        if (j.contains("fps")) {
            if (!j["fps"].is_number_integer()) {
                outError = "Invalid .framenote file: fps must be an integer";
                return nullptr;
            }
            int fps = j["fps"].get<int>();
            if (fps < 1 || fps > 120) {
                outError = "Invalid .framenote file: fps out of range (1-120)";
                return nullptr;
            }
            doc->setFps(fps);
        }

        // Palette
        if (j.contains("palette")) {
            if (!j["palette"].is_array()) {
                outError = "Invalid .framenote file: palette must be an array";
                return nullptr;
            }

            std::vector<Color> loadedColors;

            for (const auto& item : j["palette"]) {
                if (!item.is_string())
                    continue;

                Color c;

                if (readHexColor(item.get<std::string>(), c)) {
                    loadedColors.push_back(c);
                }
            }

            if (!loadedColors.empty()) {
                doc->palette().setColors(std::move(loadedColors));
            }
        }

        // Frames
        if (!j.contains("frames") || !j["frames"].is_array()) {
            outError = "Invalid .framenote file: frames must be an array";
            return nullptr;
        }

        for (const auto& fj : j["frames"]) {
            if (!fj.is_object() ||
                !fj.contains("pixels") ||
                !fj["pixels"].is_string()) {
                outError = "Invalid .framenote file: frame is missing pixel data";
                return nullptr;
            }

            int idx = doc->addFrame();

            auto pngData = base64Decode(fj["pixels"].get<std::string>());

            if (pngData.empty()) {
                outError = "Invalid .framenote file: empty frame PNG data";
                return nullptr;
            }

            Frame& loadedFrame = doc->frame(idx);

            if (!decodePNG(pngData, loadedFrame)) {
                outError = "Failed to decode frame PNG data";
                return nullptr;
            }

            // The document canvas size is authoritative.
            // If the decoded PNG buffer is smaller, expand it so drawing still works.
            loadedFrame.expandBuffer(w, h);
            loadedFrame.setVisibleSize(w, h);
        }

        // Always guarantee at least one frame.
        if (doc->frameCount() == 0) {
            doc->addFrame();
        }

        doc->setFilePath(path);
        doc->clearDirty();

        return doc;
    }
    catch (const json::exception& e) {
        outError = std::string("Invalid .framenote file: ") + e.what();
        return nullptr;
    }
    catch (const std::exception& e) {
        outError = std::string("Failed to load .framenote file: ") + e.what();
        return nullptr;
    }
}

} // namespace Framenote