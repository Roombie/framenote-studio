#include "io/RecoveryManager.h"

#include "io/FileManager.h"

#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include "core/Frame.h"
#include <cstdint>
using json = nlohmann::json;

namespace Framenote {

std::vector<uint32_t> RecoveryManager::buildThumbnail(
    const Document& doc,
    int& outWidth,
    int& outHeight
) {
    constexpr int ThumbSize = 32;

    outWidth = ThumbSize;
    outHeight = ThumbSize;

    std::vector<uint32_t> pixels(ThumbSize * ThumbSize, 0x00000000);

    if (doc.frameCount() <= 0)
        return pixels;

    int srcW = doc.canvasSize().width;
    int srcH = doc.canvasSize().height;

    if (srcW <= 0 || srcH <= 0)
        return pixels;

    const Frame& frame = doc.frame(0);

    float scaleX = static_cast<float>(ThumbSize) / static_cast<float>(srcW);
    float scaleY = static_cast<float>(ThumbSize) / static_cast<float>(srcH);
    float scale = std::min(scaleX, scaleY);

    int drawW = static_cast<int>(srcW * scale);
    int drawH = static_cast<int>(srcH * scale);

    if (drawW < 1) drawW = 1;
    if (drawH < 1) drawH = 1;
    if (drawW > ThumbSize) drawW = ThumbSize;
    if (drawH > ThumbSize) drawH = ThumbSize;

    int offsetX = (ThumbSize - drawW) / 2;
    int offsetY = (ThumbSize - drawH) / 2;

    for (int y = 0; y < drawH; ++y) {
        int srcY = (y * srcH) / drawH;

        if (srcY < 0) srcY = 0;
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < drawW; ++x) {
            int srcX = (x * srcW) / drawW;

            if (srcX < 0) srcX = 0;
            if (srcX >= srcW) srcX = srcW - 1;

            pixels[(offsetY + y) * ThumbSize + (offsetX + x)] =
                frame.getPixel(srcX, srcY);
        }
    }

    return pixels;
}

RecoveryManager::RecoveryManager() {
    load();
}

std::string RecoveryManager::baseDir() const {
    char* prefPath = SDL_GetPrefPath("Roombie", "FramenoteStudio");

    std::string result;

    if (prefPath) {
        result = prefPath;
        SDL_free(prefPath);
    }
    else {
        result = ".";
    }

    std::filesystem::create_directories(result);

    return result;
}

std::string RecoveryManager::recoveryDir() const {
    std::filesystem::path dir = std::filesystem::path(baseDir()) / "recovery";
    std::filesystem::create_directories(dir);
    return dir.string();
}

std::string RecoveryManager::indexPath() const {
    return (std::filesystem::path(recoveryDir()) / "recovery_index.json").string();
}

std::string RecoveryManager::fileNameFromPath(const std::string& path) {
    std::filesystem::path p(path);

    if (!p.filename().string().empty())
        return p.filename().string();

    return path;
}

std::string RecoveryManager::nowString() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    std::ostringstream ss;
    ss << std::put_time(&localTime, "%Y-%m-%d %H:%M");

    return ss.str();
}

std::string RecoveryManager::makeId() {
    auto now = std::chrono::high_resolution_clock::now();
    auto ticks = now.time_since_epoch().count();

    static std::mt19937_64 rng{std::random_device{}()};
    uint64_t randomPart = rng();

    std::ostringstream ss;
    ss << std::hex << ticks << "_" << randomPart;

    return ss.str();
}

void RecoveryManager::load() {
    m_entries.clear();

    std::ifstream file(indexPath(), std::ios::binary);

    if (!file.is_open())
        return;

    try {
        json j = json::parse(file);

        if (!j.contains("recoveries") || !j["recoveries"].is_array())
            return;

        for (const auto& item : j["recoveries"]) {
            if (!item.is_object())
                continue;

            RecoveryEntry entry;

            entry.id           = item.value("id", "");
            entry.recoveryPath = item.value("recoveryPath", "");
            entry.displayName  = item.value("displayName", "");
            entry.sourcePath   = item.value("sourcePath", "");
            entry.savedAt      = item.value("savedAt", "");

            entry.canvasWidth  = item.value("canvasWidth", 0);
            entry.canvasHeight = item.value("canvasHeight", 0);
            entry.fps          = item.value("fps", 0);
            entry.frameCount   = item.value("frameCount", 0);

            entry.thumbnailWidth  = item.value("thumbnailWidth", 0);
            entry.thumbnailHeight = item.value("thumbnailHeight", 0);

            if (item.contains("thumbnailPixels") &&
                item["thumbnailPixels"].is_array()) {
                for (const auto& px : item["thumbnailPixels"]) {
                    if (px.is_number_unsigned()) {
                        uint64_t value = px.get<uint64_t>();

                        if (value <= 0xFFFFFFFFull) {
                            entry.thumbnailPixels.push_back(
                                static_cast<uint32_t>(value)
                            );
                        }
                    }
                    else if (px.is_number_integer()) {
                        int64_t value = px.get<int64_t>();

                        if (value >= 0 && value <= 0xFFFFFFFFll) {
                            entry.thumbnailPixels.push_back(
                                static_cast<uint32_t>(value)
                            );
                        }
                    }
                }
            }

            if (!entry.hasThumbnail()) {
                entry.thumbnailWidth = 0;
                entry.thumbnailHeight = 0;
                entry.thumbnailPixels.clear();
            }

            if (!entry.id.empty() && !entry.recoveryPath.empty())
                m_entries.push_back(entry);
        }

        pruneMissingFiles();
    }
    catch (...) {
        m_entries.clear();
    }
}

void RecoveryManager::save() const {
    json j;
    j["recoveries"] = json::array();

    for (const auto& entry : m_entries) {
        json item = {
            {"id", entry.id},
            {"recoveryPath", entry.recoveryPath},
            {"displayName", entry.displayName},
            {"sourcePath", entry.sourcePath},
            {"savedAt", entry.savedAt},
            {"canvasWidth", entry.canvasWidth},
            {"canvasHeight", entry.canvasHeight},
            {"fps", entry.fps},
            {"frameCount", entry.frameCount}
        };

        if (entry.hasThumbnail()) {
            item["thumbnailWidth"] = entry.thumbnailWidth;
            item["thumbnailHeight"] = entry.thumbnailHeight;
            item["thumbnailPixels"] = json::array();

            for (uint32_t px : entry.thumbnailPixels) {
                item["thumbnailPixels"].push_back(px);
            }
        }

        j["recoveries"].push_back(item);
    }

    std::ofstream file(indexPath(), std::ios::binary);

    if (!file.is_open())
        return;

    file << j.dump(2);
}

bool RecoveryManager::autosave(DocumentTab& tab, std::string& outError) {
    if (!tab.document)
        return false;

    if (!tab.document->isDirty())
        return false;

    if (tab.recoveryId.empty())
        tab.recoveryId = makeId();

    if (tab.recoveryPath.empty()) {
        tab.recoveryPath =
            (std::filesystem::path(recoveryDir()) /
             (tab.recoveryId + ".framenote")).string();
    }

    if (!FileManager::save(*tab.document, tab.recoveryPath, outError))
        return false;

    RecoveryEntry entry;

    entry.id = tab.recoveryId;
    entry.recoveryPath = tab.recoveryPath;
    entry.displayName = tab.name.empty() ? "untitled" : tab.name;
    entry.sourcePath = tab.document->filePath();
    entry.savedAt = nowString();

    entry.canvasWidth  = tab.document->canvasSize().width;
    entry.canvasHeight = tab.document->canvasSize().height;
    entry.fps          = tab.document->fps();
    entry.frameCount   = tab.document->frameCount();
    entry.thumbnailPixels = buildThumbnail(
        *tab.document,
        entry.thumbnailWidth,
        entry.thumbnailHeight
    );

    upsertEntry(entry);
    save();

    return true;
}

void RecoveryManager::upsertEntry(const RecoveryEntry& entry) {
    auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [&entry](const RecoveryEntry& existing) {
            return existing.id == entry.id;
        }
    );

    if (it != m_entries.end()) {
        *it = entry;
    }
    else {
        m_entries.push_back(entry);
    }

    std::stable_sort(
        m_entries.begin(),
        m_entries.end(),
        [](const RecoveryEntry& a, const RecoveryEntry& b) {
            return a.savedAt > b.savedAt;
        }
    );
}

void RecoveryManager::remove(const std::string& id) {
    auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [&id](const RecoveryEntry& entry) {
            return entry.id == id;
        }
    );

    if (it != m_entries.end()) {
        if (!it->recoveryPath.empty()) {
            std::error_code ec;
            std::filesystem::remove(it->recoveryPath, ec);
        }

        m_entries.erase(it);
        save();
    }
}

void RecoveryManager::clearAll() {
    for (const auto& entry : m_entries) {
        if (!entry.recoveryPath.empty()) {
            std::error_code ec;
            std::filesystem::remove(entry.recoveryPath, ec);
        }
    }

    m_entries.clear();
    save();
}

void RecoveryManager::clearForRecoveryId(const std::string& id) {
    if (!id.empty())
        remove(id);
}

void RecoveryManager::clearForSourcePath(const std::string& sourcePath) {
    if (sourcePath.empty())
        return;

    std::vector<std::string> idsToRemove;

    for (const auto& entry : m_entries) {
        if (entry.sourcePath == sourcePath)
            idsToRemove.push_back(entry.id);
    }

    for (const auto& id : idsToRemove)
        remove(id);
}

void RecoveryManager::pruneMissingFiles() {
    bool changed = false;

    m_entries.erase(
        std::remove_if(
            m_entries.begin(),
            m_entries.end(),
            [&changed](const RecoveryEntry& entry) {
                if (entry.recoveryPath.empty())
                    return true;

                if (!std::filesystem::exists(entry.recoveryPath)) {
                    changed = true;
                    return true;
                }

                return false;
            }
        ),
        m_entries.end()
    );

    if (changed)
        save();
}

} // namespace Framenote