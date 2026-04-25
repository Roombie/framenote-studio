#pragma once

#include <imgui.h>
#include "core/Document.h"
#include "core/Timeline.h"
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
                float&          panY);

    void render();

private:
    void handleInput(float originX, float originY, float canvasW, float canvasH);

    Document*       m_document;
    Timeline*       m_timeline;
    ToolManager*    m_toolManager;
    CanvasRenderer* m_renderer;

    float& m_zoom;
    float& m_panX;
    float& m_panY;
};

} // namespace Framenote
