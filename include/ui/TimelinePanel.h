#pragma once

#include "core/Document.h"
#include "core/Timeline.h"

namespace Framenote {

// Bottom panel: frame thumbnails, add/delete buttons, play controls, FPS slider.
class TimelinePanel {
public:
    TimelinePanel(Document* document, Timeline* timeline);

    void render();

private:
    void renderPlaybackControls();
    void renderFrameStrip();

    Document* m_document;
    Timeline* m_timeline;

    static constexpr float THUMB_SIZE = 48.0f;  // frame thumbnail size in px
};

} // namespace Framenote
