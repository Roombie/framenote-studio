#include "ui/CanvasPanel.h"

#include "core/Palette.h"
#include "core/Selection.h"
#include "core/DocumentTab.h"

#include "tools/LineTool.h"
#include "tools/RectangleTool.h"
#include "tools/EllipseTool.h"
#include "tools/SelectionTool.h"
#include "tools/ShapeRasterizer.h"
#include "tools/SelectionPixelClip.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>

namespace Framenote {

// ─────────────────────────────────────────────────────────────────────────────
// Floating pixels / overlays
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::drawFloatingPixels(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    (void)canvasW;
    (void)canvasH;

    if (!m_tab || !m_tab->hasFloating)
        return;

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

void CanvasPanel::drawCanvasBase(
    ImDrawList* dl,
    float originX,
    float originY,
    float canvasW,
    float canvasH
) {
    ImTextureID checker = (ImTextureID)(intptr_t)m_renderer->checkerboardTexture();

    dl->AddImage(
        checker,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

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

    ImTextureID tid = (ImTextureID)(intptr_t)m_renderer->canvasTexture();

    dl->AddImage(
        tid,
        {originX, originY},
        {originX + canvasW, originY + canvasH}
    );

    dl->AddRect(
        {originX, originY},
        {originX + canvasW, originY + canvasH},
        IM_COL32(80, 80, 80, 255),
        0.0f,
        0,
        2.0f
    );
}

void CanvasPanel::drawRubberBandSelectionPreview(
    ImDrawList* dl,
    float originX,
    float originY
) {
    if (m_toolManager->activeToolType() != ToolType::Select)
        return;

    auto* selTool = static_cast<SelectionTool*>(m_toolManager->activeTool());

    if (!selTool || !selTool->isSelecting())
        return;

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
        0.0f,
        0,
        1.5f
    );

    dl->AddRect(
        {rx0, ry0},
        {rx1, ry1},
        IM_COL32(44, 184, 213, 220),
        0.0f,
        0,
        1.0f
    );
}

void CanvasPanel::drawDash(
    ImDrawList* dl,
    float x0,
    float y0,
    float x1,
    float y1,
    float& edgeLen,
    float dashOffset,
    ImU32 colorA,
    ImU32 colorB
) {
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
                ? colorA
                : colorB;

        dl->AddLine({lx0, ly0}, {lx1, ly1}, col, 1.0f);

        pos += segLen;
        start += segLen;
        remaining -= segLen;
    }

    edgeLen += len;
}

void CanvasPanel::drawSelectionOverlays(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    (void)canvasW;
    (void)canvasH;

    bool hasAnts =
        (m_tab && m_tab->hasFloating) ||
        (m_selection && !m_selection->isEmpty());

    if (!hasAnts)
        return;

    float t = static_cast<float>(ImGui::GetTime());
    float dashOffset = std::fmod(t * 6.0f, 8.0f);

    ImU32 antWhite = IM_COL32(255, 255, 255, 230);
    ImU32 antBlack = IM_COL32(0, 0, 0, 230);

    float edgeLen = 0.0f;

    if (m_tab && m_tab->hasFloating) {
        if (m_selection && !m_selection->isEmpty()) {
            int sw = m_selection->width();
            int sh = m_selection->height();

            int deltaX = m_tab->floatOffsetX - m_tab->floatStartX;
            int deltaY = m_tab->floatOffsetY - m_tab->floatStartY;

            auto isTranslatedSelected = [&](int movedX, int movedY) -> bool {
                int originalX = movedX - deltaX;
                int originalY = movedY - deltaY;

                if (originalX < 0 || originalY < 0 ||
                    originalX >= sw || originalY >= sh) {
                    return false;
                }

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

                    if (!isTranslatedSelected(tx, ty - 1)) {
                        drawDash(
                            dl,
                            cx0,
                            cy0,
                            cx1,
                            cy0,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx, ty + 1)) {
                        drawDash(
                            dl,
                            cx0,
                            cy1,
                            cx1,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx - 1, ty)) {
                        drawDash(
                            dl,
                            cx0,
                            cy0,
                            cx0,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }

                    if (!isTranslatedSelected(tx + 1, ty)) {
                        drawDash(
                            dl,
                            cx1,
                            cy0,
                            cx1,
                            cy1,
                            edgeLen,
                            dashOffset,
                            antWhite,
                            antBlack
                        );
                    }
                }
            }
        }
        else {
            int ox = m_tab->floatOffsetX;
            int oy = m_tab->floatOffsetY;
            int fw = m_tab->floatW;
            int fh = m_tab->floatH;

            float rx0 = originX + ox * m_zoom;
            float ry0 = originY + oy * m_zoom;
            float rx1 = originX + (ox + fw) * m_zoom;
            float ry1 = originY + (oy + fh) * m_zoom;

            drawDash(dl, rx0, ry0, rx1, ry0, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx1, ry0, rx1, ry1, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx1, ry1, rx0, ry1, edgeLen, dashOffset, antWhite, antBlack);
            drawDash(dl, rx0, ry1, rx0, ry0, edgeLen, dashOffset, antWhite, antBlack);
        }

        return;
    }

    if (!m_selection || m_selection->isEmpty())
        return;

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
                float x0;
                float y0;
                float x1;
                float y1;
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

                const Edge& ed = edges[ei];

                drawDash(
                    dl,
                    ed.x0,
                    ed.y0,
                    ed.x1,
                    ed.y1,
                    edgeLen,
                    dashOffset,
                    antWhite,
                    antBlack
                );
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shape previews
// ─────────────────────────────────────────────────────────────────────────────

void CanvasPanel::drawShapePreviews(
    ImDrawList* dl,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    ImGuiIO& io = ImGui::GetIO();

    Color previewColor =
        (io.MouseDown[1] && !io.MouseDown[0])
            ? m_document->palette().secondaryColor()
            : m_document->palette().primaryColor();

    ImU32 previewFill = IM_COL32(
        previewColor.r,
        previewColor.g,
        previewColor.b,
        180
    );

    ToolType activeTool = m_toolManager->activeToolType();

    if (activeTool == ToolType::Line) {
        auto* lineTool = static_cast<LineTool*>(
            m_toolManager->getTool(ToolType::Line)
        );

        if (lineTool && lineTool->isDrawing()) {
            drawPreviewLinePixels(
                dl,
                lineTool->startX(),
                lineTool->startY(),
                lineTool->endX(),
                lineTool->endY(),
                m_toolManager->brushSize(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
    else if (activeTool == ToolType::Rectangle) {
        auto* rectTool = static_cast<RectangleTool*>(
            m_toolManager->getTool(ToolType::Rectangle)
        );

        if (rectTool && rectTool->isDrawing()) {
            drawPreviewRectPixels(
                dl,
                rectTool->startX(),
                rectTool->startY(),
                rectTool->endX(),
                rectTool->endY(),
                rectTool->filled(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
    else if (activeTool == ToolType::Ellipse) {
        auto* ellipseTool = static_cast<EllipseTool*>(
            m_toolManager->getTool(ToolType::Ellipse)
        );

        if (ellipseTool && ellipseTool->isDrawing()) {
            drawPreviewEllipsePixels(
                dl,
                ellipseTool->startX(),
                ellipseTool->startY(),
                ellipseTool->endX(),
                ellipseTool->endY(),
                m_toolManager->rectFilled(),
                previewFill,
                originX,
                originY,
                canvasW,
                canvasH
            );
        }
    }
}

void CanvasPanel::drawPreviewBrushPixel(
    ImDrawList* dl,
    int x,
    int y,
    int brushSize,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int half = brushSize / 2;

    for (int by = -half; by < brushSize - half; ++by) {
        for (int bx = -half; bx < brushSize - half; ++bx) {
            int px = x + bx;
            int py = y + by;

            if (px < 0 || py < 0 || px >= canvasW || py >= canvasH)
                continue;

            if (!SelectionPixelClip::canModifyPixel(m_selection, px, py))
                continue;

            float sx0 = originX + px * m_zoom;
            float sy0 = originY + py * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    }
}

void CanvasPanel::drawPreviewLinePixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    int brushSize,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);

    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true) {
        drawPreviewBrushPixel(
            dl,
            x0,
            y0,
            brushSize,
            color,
            originX,
            originY,
            canvasW,
            canvasH
        );

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void CanvasPanel::drawPreviewRectPixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    bool filled,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    int left   = std::min(x0, x1);
    int right  = std::max(x0, x1);
    int top    = std::min(y0, y1);
    int bottom = std::max(y0, y1);

    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            bool edge =
                x == left || x == right ||
                y == top  || y == bottom;

            if (!filled && !edge)
                continue;

            if (x < 0 || y < 0 || x >= canvasW || y >= canvasH)
                continue;

            if (!SelectionPixelClip::canModifyPixel(m_selection, x, y))
                continue;

            float sx0 = originX + x * m_zoom;
            float sy0 = originY + y * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    }
}

void CanvasPanel::drawPreviewEllipsePixels(
    ImDrawList* dl,
    int x0,
    int y0,
    int x1,
    int y1,
    bool filled,
    ImU32 color,
    float originX,
    float originY,
    int canvasW,
    int canvasH
) {
    ShapeRasterizer::rasterizeEllipse(
        x0,
        y0,
        x1,
        y1,
        filled,
        [&](int x, int y) {
            if (x < 0 || y < 0 || x >= canvasW || y >= canvasH)
                return;

            if (!SelectionPixelClip::canModifyPixel(m_selection, x, y))
                return;

            float sx0 = originX + x * m_zoom;
            float sy0 = originY + y * m_zoom;

            dl->AddRectFilled(
                {sx0, sy0},
                {sx0 + m_zoom, sy0 + m_zoom},
                color
            );
        }
    );
}
} // namespace Framenote