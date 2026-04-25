#include "core/History.h"

namespace Framenote {

void History::push(Snapshot snapshot) {
    m_undoStack.push_back(std::move(snapshot));

    // Trim oldest if over limit
    if ((int)m_undoStack.size() > MAX_STEPS)
        m_undoStack.erase(m_undoStack.begin());

    // Any new action clears redo
    m_redoStack.clear();
}

Snapshot History::undo(Snapshot current) {
    // Push current state to redo stack
    m_redoStack.push_back(std::move(current));

    // Pop and return previous state
    Snapshot prev = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    return prev;
}

Snapshot History::redo(Snapshot current) {
    // Push current state to undo stack
    m_undoStack.push_back(std::move(current));

    // Pop and return next state
    Snapshot next = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    return next;
}

void History::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

} // namespace Framenote
