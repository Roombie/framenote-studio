#pragma once
#include <string>

namespace Framenote {

// Native Windows file dialogs — no extra libraries needed
class FileDialog {
public:
    // Returns empty string if user cancelled
    static std::string openFile(const char* filter, const char* title = "Open");
    static std::string saveFile(const char* filter, const char* defaultExt, const char* title = "Save");
};

} // namespace Framenote
