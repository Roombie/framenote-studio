#pragma once

#include <memory>
#include <string>
#include "core/Document.h"
#include "core/Timeline.h"
#include "core/History.h"
#include "renderer/CanvasRenderer.h"

namespace Framenote {

// A DocumentTab owns everything needed for one open document
struct DocumentTab {
    std::string              name;           // display name in tab
    std::unique_ptr<Document>      document;
    std::unique_ptr<Timeline>      timeline;
    std::unique_ptr<History>       history;
    std::unique_ptr<CanvasRenderer> renderer;

    bool requestClose = false; // set true when user clicks X on tab

    // Create a blank document tab
    static std::unique_ptr<DocumentTab> createBlank(
        SDL_Renderer* sdlRenderer,
        const std::string& name = "untitled",
        int w = 128, int h = 128, int fps = 8);
};

} // namespace Framenote