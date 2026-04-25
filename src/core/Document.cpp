#include "core/Document.h"
#include <stdexcept>

namespace Framenote {

Document::Document(int width, int height)
    : m_canvasSize{width, height}
{
    m_palette.loadDefault();
}

Frame& Document::frame(int index) {
    if (index < 0 || index >= static_cast<int>(m_frames.size()))
        throw std::out_of_range("Frame index out of range");
    return *m_frames[index];
}

const Frame& Document::frame(int index) const {
    if (index < 0 || index >= static_cast<int>(m_frames.size()))
        throw std::out_of_range("Frame index out of range");
    return *m_frames[index];
}

int Document::addFrame() {
    m_frames.push_back(std::make_unique<Frame>(m_canvasSize.width, m_canvasSize.height));
    m_dirty = true;
    return static_cast<int>(m_frames.size()) - 1;
}

void Document::duplicateFrame(int index) {
    if (index < 0 || index >= static_cast<int>(m_frames.size())) return;
    auto copy = std::make_unique<Frame>(m_frames[index]->clone());
    m_frames.insert(m_frames.begin() + index + 1, std::move(copy));
    m_dirty = true;
}

void Document::deleteFrame(int index) {
    if (index < 0 || index >= static_cast<int>(m_frames.size())) return;
    if (m_frames.size() == 1) return; // always keep at least one frame
    m_frames.erase(m_frames.begin() + index);
    m_dirty = true;
}

void Document::moveFrame(int from, int to) {
    int count = static_cast<int>(m_frames.size());
    if (from < 0 || from >= count || to < 0 || to >= count || from == to) return;
    auto frame = std::move(m_frames[from]);
    m_frames.erase(m_frames.begin() + from);
    m_frames.insert(m_frames.begin() + to, std::move(frame));
    m_dirty = true;
}

} // namespace Framenote
