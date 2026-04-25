#pragma once
#include "tools/ToolManager.h"

namespace Framenote {

class EyedropperTool : public Tool {
public:
    ToolType    type() const override { return ToolType::Eyedropper; }
    const char* name() const override { return "Eyedropper"; }
    void onPress(Document& doc, int frameIndex, const ToolEvent& e) override;
};

} // namespace Framenote
