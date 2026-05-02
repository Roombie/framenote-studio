#pragma once

#include <imgui.h>

#include <algorithm>

namespace Framenote {
namespace ModalUtils {

inline ImVec2 mainViewportCenter() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    return {
        viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f
    };
}

inline void centerNextWindowOnAppearing() {
    ImGui::SetNextWindowPos(
        mainViewportCenter(),
        ImGuiCond_Appearing,
        {0.5f, 0.5f}
    );
}

inline void keepCurrentWindowInsideMainViewport(float margin = 12.0f) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImVec2 workMin = viewport->WorkPos;
    ImVec2 workMax = {
        viewport->WorkPos.x + viewport->WorkSize.x,
        viewport->WorkPos.y + viewport->WorkSize.y
    };

    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    float minX = workMin.x + margin;
    float minY = workMin.y + margin;
    float maxX = workMax.x - size.x - margin;
    float maxY = workMax.y - size.y - margin;

    ImVec2 newPos = pos;

    if (maxX < minX) {
        newPos.x = workMin.x + (viewport->WorkSize.x - size.x) * 0.5f;
    }
    else {
        newPos.x = std::clamp(newPos.x, minX, maxX);
    }

    if (maxY < minY) {
        newPos.y = workMin.y + (viewport->WorkSize.y - size.y) * 0.5f;
    }
    else {
        newPos.y = std::clamp(newPos.y, minY, maxY);
    }

    if (newPos.x != pos.x || newPos.y != pos.y) {
        ImGui::SetWindowPos(newPos, ImGuiCond_Always);
    }
}

} // namespace ModalUtils
} // namespace Framenote