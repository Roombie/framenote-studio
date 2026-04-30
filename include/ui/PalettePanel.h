#pragma once

#include "core/Document.h"

#include <vector>

namespace Framenote {

class PalettePanel {
public:
    PalettePanel(
        Document* document,
        bool& editingSecondary,
        std::vector<int>& selection,
        bool& gestureActive,
        bool& gestureSelecting,
        bool& gestureStartedOnSelected,
        int& gestureStartIndex
    );

    void render();

private:
    void renderColorSlots(Palette& palette);
    void renderSwatchGrid(Palette& palette);
    void renderColorChooser(Palette& palette);
    void renderToolbar(Palette& palette);

    Document* m_document;
    bool&     m_editingSecondary;

    std::vector<int>& m_selection;
    bool&             m_gestureActive;
    bool&             m_gestureSelecting;
    bool&             m_gestureStartedOnSelected;
    int&              m_gestureStartIndex;

    static constexpr float SWATCH_SIZE = 20.0f;
};

} // namespace Framenote