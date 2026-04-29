#pragma once

#include "core/Document.h"

namespace Framenote {

class PalettePanel {
public:
    PalettePanel(Document* document, bool& editingSecondary);

    void render();

private:
    void renderColorSlots(Palette& palette);
    void renderSwatchGrid(Palette& palette);
    void renderColorChooser(Palette& palette);
    void renderToolbar(Palette& palette);

    Document* m_document;
    bool&     m_editingSecondary;

    static constexpr float SWATCH_SIZE = 20.0f;
};

} // namespace Framenote