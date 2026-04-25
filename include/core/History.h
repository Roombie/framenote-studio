#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace Framenote {

// A single undoable snapshot — stores pixel data for one frame at one moment
struct Snapshot {
    int frameIndex;
    int bufferWidth;
    int bufferHeight;
    std::vector<uint8_t> pixels;
};

// History manages undo/redo stacks
// Max 50 steps to keep memory reasonable
class History {
public:
    static constexpr int MAX_STEPS = 50;

    // Push a snapshot BEFORE a drawing action
    void push(Snapshot snapshot);

    // Returns true if undo was possible
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    // Pop from undo stack — caller must apply the snapshot to the document
    Snapshot undo(Snapshot current);

    // Pop from redo stack
    Snapshot redo(Snapshot current);

    void clear();

private:
    std::vector<Snapshot> m_undoStack;
    std::vector<Snapshot> m_redoStack;
};

} // namespace Framenote
