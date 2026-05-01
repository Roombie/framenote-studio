#include "ui/CanvasPanel.h"
#include "core/Palette.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"
#include "tools/LineTool.h"
#include "tools/RectangleTool.h"
#include "tools/EllipseTool.h"
#include "tools/SelectionTool.h"
#include "tools/MoveTool.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <SDL3/SDL.h>

namespace Framenote {

static float fnClamp(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

CanvasPanel::CanvasPanel(Document* document, Timeline* timeline,
                         ToolManager* toolManager, CanvasRenderer* renderer,
                         float& zoom, float& panX, float& panY,
                         History& history,
                         bool& strokeActive,
                         int& strokeFrameIndex,
                         Selection* selection,
                         DocumentTab* tab)
    : m_document(document), m_timeline(timeline)
    , m_toolManager(toolManager), m_renderer(renderer)
    , m_zoom(zoom), m_panX(panX), m_panY(panY)
    , m_history(history)
    , m_strokeActive(strokeActive)
    , m_strokeFrameIndex(strokeFrameIndex)
    , m_selection(selection)
    , m_tab(tab)
{}

void CanvasPanel::render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos({180, 62}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({
        io.DisplaySize.x - 240,
        io.DisplaySize.y - 200
    }, ImGuiCond_FirstUseEver);

    ImGui::Begin("Canvas", nullptr, flags);

    ImVec2 panelSize = ImGui::GetContentRegionAvail();

    int cw = m_document->canvasSize().width;
    int ch = m_document->canvasSize().height;

    float canvasW = cw * m_zoom;
    float canvasH = ch * m_zoom;

    ImVec2 panelPos = ImGui::GetCursorScreenPos();

    float originX = panelPos.x + (panelSize.x - canvasW) * 0.5f + m_panX;
    float originY = panelPos.y + (panelSize.y - canvasH) * 0.5f + m_panY;

    // Upload frame.
    auto& frame = m_document->frame(m_timeline->currentFrame());
    m_renderer->uploadFrame(frame);

    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        auto& prev = m_document->frame(m_timeline->currentFrame() - 1);
        m_renderer->uploadOnionFrame(prev, m_timeline->onionSkinOpacity());
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Draw pre-rendered checkerboard texture.
    ImTextureID checker = (ImTextureID)(intptr_t)m_renderer->checkerboardTexture();

    dl->AddImage(
        checker,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

    // Draw onion skin.
    if (m_timeline->onionSkinEnabled() && m_timeline->currentFrame() > 0) {
        ImTextureID onion = (ImTextureID)(intptr_t)m_renderer->onionTexture();

        uint8_t alpha = static_cast<uint8_t>(
            m_timeline->onionSkinOpacity() * 255.0f
        );

        ImU32 tint = IM_COL32(100, 180, 255, alpha);

        dl->AddImage(
            onion,
            {originX, originY},
            {originX + canvasW, originY + canvasH},
            {0, 0},
            {1, 1},
            tint
        );
    }

    // Draw current frame.
    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();

    dl->AddImage(
        tid,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

    // Canvas border.
    dl->AddRect(
        {originX, originY},
        {originX + canvasW, originY + canvasH},
        IM_COL32(80, 80, 80, 255),
        0.f,
        0,
        2.f
    );

    // Clip canvas overlays so floating pixels and ants do not leak outside.
    dl->PushClipRect(
        ImVec2(originX, originY),
        ImVec2(originX + canvasW, originY + canvasH),
        true
    );

    // ── Floating selection / floating canvas overlay ─────────────────────────
    // Draw floating pixels above the canvas.
    if (m_tab && m_tab->hasFloating) {
        int fw = m_tab->floatW;
        int fh = m_tab->floatH;
        int ox = m_tab->floatOffsetX;
        int oy = m_tab->floatOffsetY;

        for (int py = 0; py < fh; ++py) {
            for (int px = 0; px < fw; ++px) {
                size_t idx = static_cast<size_t>((py * fw + px) * 4);

                uint8_t b = m_tab->floatPixels[idx + 0];
                uint8_t g = m_tab->floatPixels[idx + 1];
                uint8_t r = m_tab->floatPixels[idx + 2];
                uint8_t a = m_tab->floatPixels[idx + 3];

                if (a == 0)
                    continue;

                float sx0 = originX + (ox + px) * m_zoom;
                float sy0 = originY + (oy + py) * m_zoom;

                dl->AddRectFilled(
                    {sx0, sy0},
                    {sx0 + m_zoom, sy0 + m_zoom},
                    IM_COL32(r, g, b, a)
                );
            }
        }
    }

    // ── Marching ants ─────────────────────────────────────────────────────────
    {
        bool hasAnts =
            (m_tab && m_tab->hasFloating) ||
            (m_selection && !m_selection->isEmpty());

        if (hasAnts) {
            float t = static_cast<float>(ImGui::GetTime());
            float dashOffset = std::fmod(t * 6.0f, 8.0f);

            ImU32 antWhite = IM_COL32(255, 255, 255, 230);
            ImU32 antBlack = IM_COL32(0, 0, 0, 230);

            auto drawDash = [&](float x0, float y0, float x1, float y1,
                                float& edgeLen) {
                float len = std::sqrt(
                    (x1 - x0) * (x1 - x0) +
                    (y1 - y0) * (y1 - y0)
                );

                if (len < 0.001f)
                    return;

                float pos = std::fmod(edgeLen + dashOffset, 8.0f);
                float remaining = len;
                float start = 0.0f;

                while (remaining > 0.001f) {
                    float segLen = std::min(
                        remaining,
                        4.0f - std::fmod(pos, 4.0f)
                    );

                    float t0 = start / len;
                    float t1 = (start + segLen) / len;

                    float lx0 = x0 + (x1 - x0) * t0;
                    float ly0 = y0 + (y1 - y0) * t0;
                    float lx1 = x0 + (x1 - x0) * t1;
                    float ly1 = y0 + (y1 - y0) * t1;

                    ImU32 col =
                        (static_cast<int>(pos / 4.0f) % 2 == 0)
                            ? antWhite
                            : antBlack;

                    dl->AddLine({lx0, ly0}, {lx1, ly1}, col, 1.0f);

                    pos += segLen;
                    start += segLen;
                    remaining -= segLen;
                }

                edgeLen += len;
            };

            float edgeLen = 0.0f;

            if (m_tab && m_tab->hasFloating) {
                if (m_selection && !m_selection->isEmpty()) {
                    int sw = m_selection->width();
                    int sh = m_selection->height();

                    // IMPORTANT:
                    // Do not use activeToolType() here.
                    // The floating buffer remembers where it started.
                    //
                    // MoveTool:
                    //   floatStart = 0,0
                    //   floatOffset = current canvas offset
                    //
                    // SelectionTool:
                    //   floatStart = original selection bounds x,y
                    //   floatOffset = current floating selection position
                    //
                    // This keeps ants stable even when switching Select <-> Move.
                    int deltaX = m_tab->floatOffsetX - m_tab->floatStartX;
                    int deltaY = m_tab->floatOffsetY - m_tab->floatStartY;

                    auto isTranslatedSelected = [&](int movedX, int movedY) -> bool {
                        int originalX = movedX - deltaX;
                        int originalY = movedY - deltaY;

                        if (originalX < 0 || originalY < 0 ||
                            originalX >= sw || originalY >= sh)
                            return false;

                        return m_selection->isSelected(originalX, originalY);
                    };

                    for (int y = 0; y < sh; ++y) {
                        for (int x = 0; x < sw; ++x) {
                            if (!m_selection->isSelected(x, y))
                                continue;

                            int tx = x + deltaX;
                            int ty = y + deltaY;

                            float cx0 = originX + tx * m_zoom;
                            float cy0 = originY + ty * m_zoom;
                            float cx1 = cx0 + m_zoom;
                            float cy1 = cy0 + m_zoom;

                            if (!isTranslatedSelected(tx, ty - 1))
                                drawDash(cx0, cy0, cx1, cy0, edgeLen);

                            if (!isTranslatedSelected(tx, ty + 1))
                                drawDash(cx0, cy1, cx1, cy1, edgeLen);

                            if (!isTranslatedSelected(tx - 1, ty))
                                drawDash(cx0, cy0, cx0, cy1, edgeLen);

                            if (!isTranslatedSelected(tx + 1, ty))
                                drawDash(cx1, cy0, cx1, cy1, edgeLen);
                        }
                    }
                } else {
                    // No selection mask exists.
                    // The floating object is the whole canvas/frame.
                    int ox = m_tab->floatOffsetX;
                    int oy = m_tab->floatOffsetY;
                    int fw = m_tab->floatW;
                    int fh = m_tab->floatH;

                    float rx0 = originX + ox * m_zoom;
                    float ry0 = originY + oy * m_zoom;
                    float rx1 = originX + (ox + fw) * m_zoom;
                    float ry1 = originY + (oy + fh) * m_zoom;

                    drawDash(rx0, ry0, rx1, ry0, edgeLen);
                    drawDash(rx1, ry0, rx1, ry1, edgeLen);
                    drawDash(rx1, ry1, rx0, ry1, edgeLen);
                    drawDash(rx0, ry1, rx0, ry0, edgeLen);
                }
            } else if (m_selection && !m_selection->isEmpty()) {
                // No floating pixels.
                // Draw the normal selection mask directly.
                int sw = m_selection->width();
                int sh = m_selection->height();

                auto isSelected = [&](int x, int y) -> bool {
                    if (x < 0 || y < 0 || x >= sw || y >= sh)
                        return false;

                    return m_selection->isSelected(x, y);
                };

                for (int y = 0; y < sh; ++y) {
                    for (int x = 0; x < sw; ++x) {
                        if (!isSelected(x, y))
                            continue;

                        struct Edge {
                            float x0, y0, x1, y1;
                        };

                        Edge edges[4] = {
                            {
                                originX + x * m_zoom,
                                originY + y * m_zoom,
                                originX + (x + 1) * m_zoom,
                                originY + y * m_zoom
                            },
                            {
                                originX + x * m_zoom,
                                originY + (y + 1) * m_zoom,
                                originX + (x + 1) * m_zoom,
                                originY + (y + 1) * m_zoom
                            },
                            {
                                originX + x * m_zoom,
                                originY + y * m_zoom,
                                originX + x * m_zoom,
                                originY + (y + 1) * m_zoom
                            },
                            {
                                originX + (x + 1) * m_zoom,
                                originY + y * m_zoom,
                                originX + (x + 1) * m_zoom,
                                originY + (y + 1) * m_zoom
                            }
                        };

                        bool neighbors[4] = {
                            isSelected(x, y - 1),
                            isSelected(x, y + 1),
                            isSelected(x - 1, y),
                            isSelected(x + 1, y)
                        };

                        for (int ei = 0; ei < 4; ++ei) {
                            if (neighbors[ei])
                                continue;

                            auto& ed = edges[ei];
                            drawDash(ed.x0, ed.y0, ed.x1, ed.y1, edgeLen);
                        }
                    }
                }
            }
        }
    }

    dl->PopClipRect();

    // ── Rubber band selection preview ─────────────────────────────────────────
    if (m_toolManager->activeToolType() == ToolType::Select) {
        auto* selTool = static_cast<SelectionTool*>(m_toolManager->activeTool());

        if (selTool && selTool->isSelecting()) {
            int x0 = std::min(selTool->startX(), selTool->endX());
            int y0 = std::min(selTool->startY(), selTool->endY());
            int x1 = std::max(selTool->startX(), selTool->endX());
            int y1 = std::max(selTool->startY(), selTool->endY());

            float rx0 = originX + x0 * m_zoom;
            float ry0 = originY + y0 * m_zoom;
            float rx1 = originX + (x1 + 1) * m_zoom;
            float ry1 = originY + (y1 + 1) * m_zoom;

            dl->AddRectFilled(
                {rx0, ry0},
                {rx1, ry1},
                IM_COL32(44, 184, 213, 40)
            );

            dl->AddRect(
                {rx0 - 1, ry0 - 1},
                {rx1 + 1, ry1 + 1},
                IM_COL32(0, 0, 0, 180),
                0.f,
                0,
                1.5f
            );

            dl->AddRect(
                {rx0, ry0},
                {rx1, ry1},
                IM_COL32(44, 184, 213, 220),
                0.f,
                0,
                1.0f
            );
        }
    }

    // Input state.
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = {
        ImGui::GetWindowWidth(),
        ImGui::GetWindowHeight()
    };

    ImVec2 mp = io.MousePos;

    bool canvasWindowHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    bool rawInWindow =
        mp.x >= winPos.x &&
        mp.x < winPos.x + winSize.x &&
        mp.y >= winPos.y &&
        mp.y < winPos.y + winSize.y;

    bool rawInCanvas =
        mp.x >= originX &&
        mp.x < originX + canvasW &&
        mp.y >= originY &&
        mp.y < originY + canvasH;

    bool inWindow =
        rawInWindow &&
        (canvasWindowHovered || m_strokeActive);

    bool inCanvas =
        rawInCanvas &&
        (canvasWindowHovered || m_strokeActive);

    bool spaceHeld = ImGui::IsKeyDown(ImGuiKey_Space);
    bool ctrlHeld = io.KeyCtrl;
    bool popupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);

    // ── Brush cursor preview ──────────────────────────────────────────────────
    if (inCanvas && !spaceHeld && !popupOpen) {
        ToolType activeTool = m_toolManager->activeToolType();

        bool showCursor =
            activeTool == ToolType::Pencil ||
            activeTool == ToolType::Eraser;

        if (showCursor) {
            int bsize = m_toolManager->brushSize();
            int half = bsize / 2;

            int px = static_cast<int>((mp.x - originX) / m_zoom);
            int py = static_cast<int>((mp.y - originY) / m_zoom);

            float rx0 = originX + (px - half) * m_zoom;
            float ry0 = originY + (py - half) * m_zoom;
            float rx1 = originX + (px - half + bsize) * m_zoom;
            float ry1 = originY + (py - half + bsize) * m_zoom;

            if (activeTool == ToolType::Pencil) {
                Color c =
                    (io.MouseDown[1] && !io.MouseDown[0])
                        ? m_document->palette().secondaryColor()
                        : m_document->palette().primaryColor();

                if (c.a > 0) {
                    dl->AddRectFilled(
                        {rx0, ry0},
                        {rx1, ry1},
                        IM_COL32(c.r, c.g, c.b, 255)
                    );
                }
            }

            dl->AddRect(
                {rx0 - 1, ry0 - 1},
                {rx1 + 1, ry1 + 1},
                IM_COL32(0, 0, 0, 180),
                0.f,
                0,
                1.5f
            );

            dl->AddRect(
                {rx0, ry0},
                {rx1, ry1},
                IM_COL32(255, 255, 255, 220),
                0.f,
                0,
                1.0f
            );
        }
    }

    // Cursor behavior.
    {
        ToolType ct = m_toolManager->activeToolType();

        bool brushTool =
            ct == ToolType::Pencil ||
            ct == ToolType::Eraser;

        bool activelyDrawing =
            inCanvas &&
            brushTool &&
            !spaceHeld &&
            !popupOpen &&
            (io.MouseDown[0] || io.MouseDown[1]);

        if (spaceHeld && inWindow) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        } else if (activelyDrawing) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        } else if (canvasWindowHovered) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        }
    }

    bool imguiCapturing = false;

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    if (!ctrlHeld && !io.WantTextInput && !popupOpen) {
        auto commitFloatIfNeeded = [&]() {
            if (!m_tab || !m_tab->hasFloating)
                return;

            ToolEvent ce;
            ce.selection = m_selection;
            ce.tab = m_tab;

            if (m_toolManager->activeToolType() == ToolType::Select) {
                auto* selectionTool = static_cast<SelectionTool*>(
                    m_toolManager->getTool(ToolType::Select)
                );

                if (selectionTool) {
                    selectionTool->commitFloat(
                        *m_document,
                        m_timeline->currentFrame(),
                        ce
                    );
                }
            } else {
                auto* moveTool = static_cast<MoveTool*>(
                    m_toolManager->getTool(ToolType::Move)
                );

                if (moveTool) {
                    moveTool->commitFloat(
                        *m_document,
                        m_timeline->currentFrame(),
                        ce
                    );
                }
            }
        };

        if (ImGui::IsKeyPressed(ImGuiKey_B, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Pencil);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Eraser);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Fill);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_I, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Eyedropper);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_L, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Line);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Rectangle);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Ellipse);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_M, false)) {
            commitFloatIfNeeded();
            m_toolManager->selectTool(ToolType::Select);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            m_toolManager->selectTool(ToolType::Move);
        }

        // Escape — commit float then deselect.
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            ToolEvent ce;
            ce.selection = m_selection;
            ce.tab = m_tab;

            if (m_toolManager->activeToolType() == ToolType::Select) {
                auto* t = static_cast<SelectionTool*>(
                    m_toolManager->getTool(ToolType::Select)
                );

                if (t)
                    t->commitFloat(
                        *m_document,
                        m_timeline->currentFrame(),
                        ce
                    );
            } else {
                auto* t = static_cast<MoveTool*>(
                    m_toolManager->getTool(ToolType::Move)
                );

                if (t)
                    t->commitFloat(
                        *m_document,
                        m_timeline->currentFrame(),
                        ce
                    );
            }

            if (m_selection)
                m_selection->clear();
        }

        // Enter — commit float or toggle play.
        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)) {
            if (m_tab && m_tab->hasFloating) {
                ToolEvent ce;
                ce.selection = m_selection;
                ce.tab = m_tab;

                if (m_toolManager->activeToolType() == ToolType::Select) {
                    auto* t = static_cast<SelectionTool*>(
                        m_toolManager->getTool(ToolType::Select)
                    );

                    if (t)
                        t->commitFloat(
                            *m_document,
                            m_timeline->currentFrame(),
                            ce
                        );
                } else {
                    auto* t = static_cast<MoveTool*>(
                        m_toolManager->getTool(ToolType::Move)
                    );

                    if (t)
                        t->commitFloat(
                            *m_document,
                            m_timeline->currentFrame(),
                            ce
                        );
                }
            } else {
                m_timeline->isPlaying()
                    ? m_timeline->pause()
                    : m_timeline->play();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false)) {
            m_toolManager->setBrushSize(m_toolManager->brushSize() - 1);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
            m_toolManager->setBrushSize(m_toolManager->brushSize() + 1);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
            m_timeline->nextFrame();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
            m_timeline->prevFrame();
        }

        if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false)) {
            if (m_timeline->isPlaying())
                m_timeline->pause();

            m_timeline->setCurrentFrame(0);
        }

        if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_RightArrow, false)) {
            if (m_timeline->isPlaying())
                m_timeline->pause();

            m_timeline->setCurrentFrame(m_timeline->frameCount() - 1);
        }
    }

    // ── Zoom with + / - keys ─────────────────────────────────────────────────
    if (inWindow && !io.WantTextInput && !popupOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom < 2.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);

            m_zoom = fnClamp(m_zoom + step, 0.25f, 64.f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) ||
            ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false)) {
            float oldZoom = m_zoom;
            float step = m_zoom <= 2.f ? 0.25f : (m_zoom <= 8.f ? 0.5f : 1.f);

            m_zoom = fnClamp(m_zoom - step, 0.25f, 64.f);
            m_panX = m_panX * (m_zoom / oldZoom);
            m_panY = m_panY * (m_zoom / oldZoom);
        }

        if (ctrlHeld && ImGui::IsKeyPressed(ImGuiKey_0, false)) {
            m_zoom = 1.0f;
            m_panX = 0.f;
            m_panY = 0.f;
        }
    }

    // ── Zoom with scroll wheel ────────────────────────────────────────────────
    if (inWindow && !popupOpen && io.MouseWheel != 0.f) {
        float step = m_zoom < 2.f ? 0.25f : (m_zoom < 8.f ? 0.5f : 1.f);
        float oldZoom = m_zoom;

        m_zoom = fnClamp(
            io.MouseWheel > 0.f ? m_zoom + step : m_zoom - step,
            0.25f,
            64.f
        );

        float ratio = m_zoom / oldZoom;

        m_panX =
            (m_panX - (mp.x - (panelPos.x + panelSize.x * 0.5f))) * ratio +
            (mp.x - (panelPos.x + panelSize.x * 0.5f));

        m_panY =
            (m_panY - (mp.y - (panelPos.y + panelSize.y * 0.5f))) * ratio +
            (mp.y - (panelPos.y + panelSize.y * 0.5f));

        io.MouseWheel = 0.f;
    }

    // ── Mouse state ───────────────────────────────────────────────────────────
    bool drawingMouseDown =
        io.MouseDown[0] ||
        io.MouseDown[1];

    bool drawingMouseReleased =
        io.MouseReleased[0] ||
        io.MouseReleased[1];

    (void)drawingMouseReleased;

    // ── Pan ───────────────────────────────────────────────────────────────────
    bool isPanning =
        io.MouseDown[2] ||
        (spaceHeld && io.MouseDown[0]);

    bool isDrawing =
        inCanvas &&
        !spaceHeld &&
        !io.MouseDown[2] &&
        drawingMouseDown;

    if (inWindow && isPanning && !isDrawing && !popupOpen && !imguiCapturing) {
        m_panX += io.MouseDelta.x;
        m_panY += io.MouseDelta.y;
    }

    // ── Drawing / tools ───────────────────────────────────────────────────────
    {
        Tool* tool = m_toolManager->activeTool();

        auto makeToolEvent = [&]() {
            float relX = (mp.x - originX) / canvasW;
            float relY = (mp.y - originY) / canvasH;

            int px = static_cast<int>(relX * cw);
            int py = static_cast<int>(relY * ch);

            if (px < 0) px = 0;
            if (py < 0) py = 0;
            if (px >= cw) px = cw - 1;
            if (py >= ch) py = ch - 1;

            ToolEvent e;
            e.canvasX = static_cast<float>(px);
            e.canvasY = static_cast<float>(py);
            e.leftDown = io.MouseDown[0];
            e.rightDown = io.MouseDown[1];
            e.brushSize = m_toolManager->brushSize();
            e.filled = m_toolManager->rectFilled();
            e.addToSelection = io.KeyShift;
            e.selection = m_selection;
            e.tab = m_tab;

            return e;
        };

        bool canDraw =
            !spaceHeld &&
            !io.MouseDown[2] &&
            !popupOpen &&
            !imguiCapturing;

        if (tool && m_strokeActive && (!drawingMouseDown || !canDraw || !rawInCanvas)) {
            ToolEvent e = makeToolEvent();

            int releaseFrame =
                m_strokeFrameIndex >= 0
                    ? m_strokeFrameIndex
                    : m_timeline->currentFrame();

            tool->onRelease(*m_document, releaseFrame, e);

            m_strokeActive = false;
            m_strokeFrameIndex = -1;
        }

        if (tool && rawInCanvas && canDraw && drawingMouseDown) {
            int fi = m_timeline->currentFrame();
            ToolEvent e = makeToolEvent();

            if (!m_strokeActive) {
                auto& f = m_document->frame(fi);

                Snapshot snap;
                snap.frameIndex = fi;
                snap.bufferWidth = f.bufferWidth();
                snap.bufferHeight = f.bufferHeight();
                snap.pixels = f.pixels();

                m_history.push(std::move(snap));

                m_strokeActive = true;
                m_strokeFrameIndex = fi;

                tool->onPress(*m_document, fi, e);
            } else {
                tool->onDrag(*m_document, fi, e);
            }
        }
    }

    // Zoom level display.
    dl->AddText(
        {panelPos.x + 4, panelPos.y + panelSize.y - 20},
        IM_COL32(150, 150, 150, 200),
        (std::to_string(static_cast<int>(m_zoom * 100)) + "%").c_str()
    );

    ImGui::End();
}

void CanvasPanel::handleInput(float, float, float, float) {}

} // namespace Framenote