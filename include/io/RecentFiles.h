#pragma once

#include "core/Document.h"

#include <string>
#include <vector>

namespace Framenote {

struct RecentFileEntry {
    std::string path;
    std::string name;
    std::string lastOpened;

    int canvasWidth  = 0;
    int canvasHeight = 0;
    int fps          = 0;
    int frameCount   = 0;

    bool pinned = false;
};

class RecentFiles {
public:
    RecentFiles();

    void load();
    void save() const;

    void addOrUpdate(const std::string& path, const Document& doc);
    void removePath(const std::string& path);
    void clear();

    void togglePinned(const std::string& path);

    bool showThumbnails() const { return m_showThumbnails; }
    bool showPaths() const { return m_showPaths; }

    void setShowThumbnails(bool value);
    void setShowPaths(bool value);

    const std::vector<RecentFileEntry>& entries() const { return m_entries; }

    bool existsOnDisk(const RecentFileEntry& entry) const;

private:
    std::string configPath() const;
    static std::string fileNameFromPath(const std::string& path);
    static std::string nowString();

    void sortEntries();
    void trimToLimit();

    std::vector<RecentFileEntry> m_entries;

    bool m_showThumbnails = true;
    bool m_showPaths      = true;

    static constexpr int MAX_RECENT_FILES = 20;
};

} // namespace Framenote