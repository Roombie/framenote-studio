#include "io/FileManager.h"
#include <nlohmann/json.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <sstream>
#include <vector>

using json = nlohmann::json;

namespace Framenote {

// ── PNG encode/decode helpers ─────────────────────────────────────────────────

static std::vector<uint8_t> encodePNG(const Frame& frame) {
    std::vector<uint8_t> out;
    auto writeFunc = [](void* ctx, void* data, int size) {
        auto* buf = static_cast<std::vector<uint8_t>*>(ctx);
        auto* bytes = static_cast<uint8_t*>(data);
        buf->insert(buf->end(), bytes, bytes + size);
    };
    stbi_write_png_to_func(writeFunc, &out,
        frame.width(), frame.height(), 4,
        frame.pixels().data(), frame.width() * 4);
    return out;
}

static bool decodePNG(const std::vector<uint8_t>& pngData, Frame& outFrame) {
    int w, h, channels;
    uint8_t* data = stbi_load_from_memory(
        pngData.data(), static_cast<int>(pngData.size()),
        &w, &h, &channels, 4);
    if (!data) return false;

    // Resize frame if needed and copy pixels
    size_t count = static_cast<size_t>(w * h * 4);
    auto& pixels = outFrame.pixels();
    pixels.assign(data, data + count);
    stbi_image_free(data);
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
    for (int i = 0; i < Palette::size(); ++i) {
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
    for (int i = 0; i < std::min((int)palette.size(), Palette::size()); ++i) {
        std::string hex = palette[i];
        Color c;
        sscanf(hex.c_str(), "#%02hhX%02hhX%02hhX%02hhX", &c.r, &c.g, &c.b, &c.a);
        doc->palette().color(i) = c;
    }

    // Frames
    for (auto& fj : j["frames"]) {
        int idx = doc->addFrame();
        auto pngData = base64Decode(fj["pixels"].get<std::string>());
        decodePNG(pngData, doc->frame(idx));
    }

    doc->setFilePath(path);
    doc->clearDirty();
    return doc;
}

} // namespace Framenote
