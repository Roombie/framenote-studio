#pragma once

#include "core/DocumentTab.h"

#include <string>
#include <vector>

namespace Framenote {

struct RecoveryEntry {
    std::string id;
    std::string recoveryPath;
    std::string displayName;
    std::string sourcePath;
    std::string savedAt;

    int canvasWidth  = 0;
    int canvasHeight = 0;
    int fps          = 0;
    int frameCount   = 0;
};

class RecoveryManager {
public:
    RecoveryManager();

    void load();
    void save() const;

    bool autosave(DocumentTab& tab, std::string& outError);

    void remove(const std::string& id);
    void clearAll();
    void clearForRecoveryId(const std::string& id);
    void clearForSourcePath(const std::string& sourcePath);

    const std::vector<RecoveryEntry>& entries() const {
        return m_entries;
    }

    bool hasEntries() const {
        return !m_entries.empty();
    }

private:
    std::string baseDir() const;
    std::string recoveryDir() const;
    std::string indexPath() const;

    static std::string nowString();
    static std::string makeId();
    static std::string fileNameFromPath(const std::string& path);

    void upsertEntry(const RecoveryEntry& entry);
    void pruneMissingFiles();

    std::vector<RecoveryEntry> m_entries;
};

} // namespace Framenote