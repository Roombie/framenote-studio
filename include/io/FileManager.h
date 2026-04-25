#pragma once

#include <string>
#include <memory>
#include "core/Document.h"

namespace Framenote {

// Handles reading and writing .framenote files.
// Format: JSON manifest + PNG frame data, zipped into a .framenote container.
class FileManager {
public:
    // Returns nullptr on failure, sets outError
    static std::unique_ptr<Document> load(const std::string& path,
                                          std::string& outError);

    // Returns false on failure, sets outError
    static bool save(const Document& doc, const std::string& path,
                     std::string& outError);

    static constexpr const char* FILE_EXTENSION = ".framenote";
    static constexpr int         FORMAT_VERSION  = 1;
};

} // namespace Framenote
