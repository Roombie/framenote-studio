#pragma once

#include <string>
#include <memory>
#include <vector>

#include "core/Document.h"

namespace Framenote {

// Handles reading and writing .framenote files.
// Format: JSON manifest + PNG frame data.
class FileManager {
public:
    // Returns nullptr on failure, sets outError.
    static std::unique_ptr<Document> load(const std::string& path,
                                          std::string& outError);

    // Returns false on failure, sets outError.
    static bool save(const Document& doc,
                     const std::string& path,
                     std::string& outError);

    // Imports a PNG as a new unsaved one-frame document.
    static std::unique_ptr<Document> loadPngAsDocument(
        const std::string& path,
        std::string& outError
    );

    // Imports multiple PNG files as one unsaved animation document.
    // Files are sorted by filename before importing.
    static std::unique_ptr<Document> loadPngSequenceAsDocument(
        const std::vector<std::string>& paths,
        int fps,
        std::string& outError
    );

    static constexpr const char* FILE_EXTENSION = ".framenote";
    static constexpr int         FORMAT_VERSION = 1;
};

} // namespace Framenote