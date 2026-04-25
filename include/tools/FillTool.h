#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

// Flood-fill (bucket) tool using iterative BFS to avoid stack overflow
// on large canvases.
class FillTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Fill; }
    const char* name() const override { return "Fill"; }

    void onPress(Document& doc, int frameIndex, const ToolEvent& e) override;
};

} // namespace Framenote
