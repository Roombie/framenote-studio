#include "core/History.h"

namespace Framenote {

void History::push(Snapshot snapshot) {
    HistoryEntry entry;
    entry.frames.push_back(std::move(snapshot));
    // canvasWidth/Height stay 0 — marks this as a drawing action
    m_undoStack.push_back(std::move(entry));
    trimUndo();
    m_redoStack.clear();
}

void History::pushResize(std::vector<Snapshot> allFrames, int oldCanvasW, int oldCanvasH) {
    HistoryEntry entry;
    entry.canvasWidth  = oldCanvasW;
    entry.canvasHeight = oldCanvasH;
    entry.frames       = std::move(allFrames);
    m_undoStack.push_back(std::move(entry));
    trimUndo();
    m_redoStack.clear();
}

HistoryEntry History::undo(HistoryEntry current) {
    m_redoStack.push_back(std::move(current));
    HistoryEntry prev = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    return prev;
}

HistoryEntry History::redo(HistoryEntry current) {
    m_undoStack.push_back(std::move(current));
    HistoryEntry next = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    return next;
}

void History::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

void History::trimUndo() {
    while ((int)m_undoStack.size() > MAX_STEPS)
        m_undoStack.erase(m_undoStack.begin());
}

} // namespace Framenote