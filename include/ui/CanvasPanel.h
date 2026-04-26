#pragma once

#include <imgui.h>
#include "core/Document.h"
#include "core/Timeline.h"
#include "core/History.h"
#include "tools/ToolManager.h"
#include "renderer/CanvasRenderer.h"

namespace Framenote {

class CanvasPanel {
public:
    CanvasPanel(Document*       document,
                Timeline*       timeline,
                ToolManager*    toolManager,
                CanvasRenderer* renderer,
                float&          zoom,
                float&          panX,
                float&          panY,
                History&        history);

    void render();

private:
    void handleInput(float originX, float originY, float canvasW, float canvasH);

    Document*       m_document;
    Timeline*       m_timeline;
    ToolManager*    m_toolManager;
    CanvasRenderer* m_renderer;

    float&   m_zoom;
    float&   m_panX;
    float&   m_panY;
    History& m_history;
};

} // namespace Framenote