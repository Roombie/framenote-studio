#pragma once

#include <vector>
#include <memory>
#include <cstdint>

namespace Framenote {

// A single undoable snapshot — stores pixel data for one frame at one moment.
struct Snapshot {
    int frameIndex;
    int bufferWidth;
    int bufferHeight;
    std::vector<uint8_t> pixels;
};

// A history entry is either a single drawing snapshot or a batch of snapshots
// (e.g. canvas resize which affects all frames at once). Batch entries undo
// and redo as a single logical step.
struct HistoryEntry {
    int canvasWidth  = 0;  // 0 = drawing action, >0 = canvas resize action
    int canvasHeight = 0;
    std::vector<Snapshot> frames;  // one per frame for resize, one for drawing
};

// History manages undo/redo stacks — max 50 steps
class History {
public:
    static constexpr int MAX_STEPS = 50;

    // Push a single-frame drawing snapshot
    void push(Snapshot snapshot);

    // Push a full-canvas resize entry (all frames + old canvas dimensions)
    void pushResize(std::vector<Snapshot> allFrames, int oldCanvasW, int oldCanvasH);

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    // Undo: caller provides current state as entry, gets back previous entry
    HistoryEntry undo(HistoryEntry current);

    // Redo: caller provides current state as entry, gets back next entry
    HistoryEntry redo(HistoryEntry current);

    void clear();

private:
    void trimUndo();

    std::vector<HistoryEntry> m_undoStack;
    std::vector<HistoryEntry> m_redoStack;
};

} // namespace Framenote