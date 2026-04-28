#pragma once

#include "core/Document.h"
#include "core/Timeline.h"
#include "ui/IconLoader.h"
#include <imgui.h>

namespace Framenote {

class TimelinePanel {
public:
    TimelinePanel(Document* document, Timeline* timeline,
                  ToolIcons* icons = nullptr);

    void render();

private:
    void renderPlaybackControls();
    void renderFrameStrip();

    // Helper: renders an icon button with text fallback
    bool iconButton(const char* id, SDL_Texture* icon,
                    const char* fallback, ImVec2 size, const char* tooltip);

    Document*  m_document;
    Timeline*  m_timeline;
    ToolIcons* m_icons;

    static constexpr float THUMB_SIZE = 48.0f;
};

} // namespace Framenote