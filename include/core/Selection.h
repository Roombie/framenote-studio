#pragma once

#include <vector>
#include <cstdint>

namespace Framenote {

struct SelectionRect {
    int x = 0, y = 0, w = 0, h = 0;
    bool empty() const { return w <= 0 || h <= 0; }
};

class Selection {
public:
    Selection() = default;
    Selection(int canvasW, int canvasH);

    void resize(int canvasW, int canvasH);

    // Select / deselect a rectangular region
    void setRect(int x, int y, int w, int h, bool value = true);

    // Add a rect to the current selection
    void addRect(int x, int y, int w, int h);

    // Remove a rect from the current selection
    void removeRect(int x, int y, int w, int h);

    // Select all / deselect all
    void selectAll();
    void clear();

    bool isSelected(int x, int y) const;
    bool isEmpty() const;

    // Tight bounding rect around all selected pixels
    SelectionRect bounds() const;

    int width()  const { return m_width; }
    int height() const { return m_height; }

    const std::vector<bool>& mask() const { return m_mask; }

private:
    int              m_width  = 0;
    int              m_height = 0;
    std::vector<bool> m_mask;

    bool inBounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < m_width && y < m_height;
    }
};

} // namespace Framenote
