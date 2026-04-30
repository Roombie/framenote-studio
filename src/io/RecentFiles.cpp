#include "io/RecentFiles.h"

#include "core/Frame.h"

#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

using json = nlohmann::json;

namespace Framenote {

std::vector<uint32_t> RecentFiles::buildThumbnail(
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

        if (srcY < 0)
            srcY = 0;

        if (srcY >= srcH)
            srcY = srcH - 1;

        for (int x = 0; x < drawW; ++x) {
            int srcX = (x * srcW) / drawW;

            if (srcX < 0)
                srcX = 0;

            if (srcX >= srcW)
                srcX = srcW - 1;

            pixels[(offsetY + y) * ThumbSize + (offsetX + x)] =
                frame.getPixel(srcX, srcY);
        }
    }

    return pixels;
}

RecentFiles::RecentFiles() {
    load();
}

std::string RecentFiles::configPath() const {
    char* prefPath = SDL_GetPrefPath("Roombie", "FramenoteStudio");

    std::string basePath;

    if (prefPath) {
        basePath = prefPath;
        SDL_free(prefPath);
    }
    else {
        basePath = ".";
    }

    std::filesystem::create_directories(basePath);

    return (std::filesystem::path(basePath) / "recent_files.json").string();
}

std::string RecentFiles::fileNameFromPath(const std::string& path) {
    std::filesystem::path p(path);

    if (!p.filename().string().empty())
        return p.filename().string();

    return path;
}

std::string RecentFiles::nowString() {
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

void RecentFiles::load() {
    m_entries.clear();

    std::ifstream file(configPath(), std::ios::binary);

    if (!file.is_open())
        return;

    try {
        json j = json::parse(file);

        if (j.contains("showThumbnails") && j["showThumbnails"].is_boolean())
            m_showThumbnails = j["showThumbnails"].get<bool>();

        if (j.contains("showPaths") && j["showPaths"].is_boolean())
            m_showPaths = j["showPaths"].get<bool>();

        if (j.contains("recentFiles") && j["recentFiles"].is_array()) {
            for (const auto& item : j["recentFiles"]) {
                if (!item.is_object())
                    continue;

                RecentFileEntry entry;

                entry.path = item.value("path", "");
                entry.name = item.value("name", fileNameFromPath(entry.path));
                entry.lastOpened = item.value("lastOpened", "");

                entry.canvasWidth  = item.value("canvasWidth", 0);
                entry.canvasHeight = item.value("canvasHeight", 0);
                entry.fps          = item.value("fps", 0);
                entry.frameCount   = item.value("frameCount", 0);
                entry.pinned       = item.value("pinned", false);

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

                if (!entry.path.empty())
                    m_entries.push_back(entry);
            }
        }

        sortEntries();
        trimToLimit();
    }
    catch (...) {
        m_entries.clear();
    }
}

void RecentFiles::save() const {
    json j;

    j["showThumbnails"] = m_showThumbnails;
    j["showPaths"]      = m_showPaths;

    json recent = json::array();

    for (const auto& entry : m_entries) {
        json item = {
            {"path", entry.path},
            {"name", entry.name},
            {"lastOpened", entry.lastOpened},
            {"canvasWidth", entry.canvasWidth},
            {"canvasHeight", entry.canvasHeight},
            {"fps", entry.fps},
            {"frameCount", entry.frameCount},
            {"pinned", entry.pinned}
        };

        if (entry.hasThumbnail()) {
            item["thumbnailWidth"] = entry.thumbnailWidth;
            item["thumbnailHeight"] = entry.thumbnailHeight;
            item["thumbnailPixels"] = json::array();

            for (uint32_t px : entry.thumbnailPixels) {
                item["thumbnailPixels"].push_back(px);
            }
        }

        recent.push_back(item);
    }

    j["recentFiles"] = recent;

    std::ofstream file(configPath(), std::ios::binary);

    if (!file.is_open())
        return;

    file << j.dump(2);
}

void RecentFiles::addOrUpdate(const std::string& path, const Document& doc) {
    if (path.empty())
        return;

    auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [&path](const RecentFileEntry& entry) {
            return entry.path == path;
        }
    );

    bool oldPinned = false;

    if (it != m_entries.end()) {
        oldPinned = it->pinned;
        m_entries.erase(it);
    }

    RecentFileEntry entry;

    entry.path = path;
    entry.name = fileNameFromPath(path);
    entry.lastOpened = nowString();

    entry.canvasWidth  = doc.canvasSize().width;
    entry.canvasHeight = doc.canvasSize().height;
    entry.fps          = doc.fps();
    entry.frameCount   = doc.frameCount();
    entry.pinned       = oldPinned;

    entry.thumbnailPixels = buildThumbnail(
        doc,
        entry.thumbnailWidth,
        entry.thumbnailHeight
    );

    m_entries.push_back(entry);

    sortEntries();
    trimToLimit();
    save();
}

void RecentFiles::removePath(const std::string& path) {
    m_entries.erase(
        std::remove_if(
            m_entries.begin(),
            m_entries.end(),
            [&path](const RecentFileEntry& entry) {
                return entry.path == path;
            }
        ),
        m_entries.end()
    );

    save();
}

void RecentFiles::clear() {
    m_entries.clear();
    save();
}

void RecentFiles::togglePinned(const std::string& path) {
    for (auto& entry : m_entries) {
        if (entry.path == path) {
            entry.pinned = !entry.pinned;
            break;
        }
    }

    sortEntries();
    save();
}

void RecentFiles::setShowThumbnails(bool value) {
    m_showThumbnails = value;
    save();
}

void RecentFiles::setShowPaths(bool value) {
    m_showPaths = value;
    save();
}

bool RecentFiles::existsOnDisk(const RecentFileEntry& entry) const {
    if (entry.path.empty())
        return false;

    return std::filesystem::exists(entry.path);
}

void RecentFiles::sortEntries() {
    std::stable_sort(
        m_entries.begin(),
        m_entries.end(),
        [](const RecentFileEntry& a, const RecentFileEntry& b) {
            if (a.pinned != b.pinned)
                return a.pinned > b.pinned;

            return a.lastOpened > b.lastOpened;
        }
    );
}

void RecentFiles::trimToLimit() {
    if (static_cast<int>(m_entries.size()) <= MAX_RECENT_FILES)
        return;

    m_entries.resize(MAX_RECENT_FILES);
}

} // namespace Framenote