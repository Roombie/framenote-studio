#pragma once

#include "core/Document.h"

namespace Framenote {

// Right sidebar: 32-color swatch grid + selected color preview.
class PalettePanel {
public:
    explicit PalettePanel(Document* document);

    void render();

private:
    Document* m_document;

    static constexpr float SWATCH_SIZE = 20.0f;
    static constexpr int   SWATCHES_PER_ROW = 4;
};

} // namespace Framenote
