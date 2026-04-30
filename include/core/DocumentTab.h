#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Document.h"
#include "core/Timeline.h"
#include "core/History.h"
#include "renderer/CanvasRenderer.h"

namespace Framenote {

struct DocumentTab {
    std::string                      name;
    std::unique_ptr<Document>        document;
    std::unique_ptr<Timeline>        timeline;
    std::unique_ptr<History>         history;
    std::unique_ptr<CanvasRenderer>  renderer;

    bool requestClose = false;

    // Canvas view state — persisted across frames
    float canvasZoom = 4.0f;
    float canvasPanX = 0.0f;
    float canvasPanY = 0.0f;

    // Canvas stroke state — persisted across frames
    bool canvasStrokeActive = false;
    int  canvasStrokeFrameIndex = -1;

    // Palette UI state — persisted across frames
    bool paletteEditingSecondary = false;

    // Palette multi-selection state — persisted across frames
    std::vector<int> paletteSelection;
    bool paletteGestureActive = false;
    bool paletteGestureSelecting = false;
    bool paletteGestureStartedOnSelected = false;
    int  paletteGestureStartIndex = -1;

    // Recovery/autosave state — persisted only during the app session
    std::string recoveryId;
    std::string recoveryPath;
    double lastAutosaveTime = 0.0;

    static std::unique_ptr<DocumentTab> createBlank(
        SDL_Renderer* sdlRenderer,
        const std::string& name = "untitled",
        int w = 128,
        int h = 128,
        int fps = 8
    );
};

} // namespace Framenote