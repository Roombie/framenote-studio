#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Frame.h"
#include "Palette.h"

namespace Framenote {

struct CanvasSize {
    int width  = 128;
    int height = 128;
};

// Document is the root in-memory representation of a .framenote file.
// It owns all frames and the palette. Zero SDL/ImGui dependencies.
class Document {
public:
    Document() = default;
    explicit Document(int width, int height);

    // ── Frames ────────────────────────────────────────────────────────────────
    int             frameCount()  const { return static_cast<int>(m_frames.size()); }
    Frame&          frame(int index);
    const Frame&    frame(int index) const;
    int             addFrame();                   // Appends a blank frame, returns index
    void            duplicateFrame(int index);    // Inserts copy after index
    void            deleteFrame(int index);
    void            moveFrame(int from, int to);

    // ── Playback metadata ─────────────────────────────────────────────────────
    int  fps()         const { return m_fps; }
    void setFps(int fps)     { m_fps = fps; }

    // ── Canvas size ───────────────────────────────────────────────────────────
    CanvasSize canvasSize() const { return m_canvasSize; }

    void setCanvasSize(int w, int h) { m_canvasSize = {w, h}; }

    // ── Palette ───────────────────────────────────────────────────────────────
    Palette&       palette()       { return m_palette; }
    const Palette& palette() const { return m_palette; }

    // ── Dirty flag (unsaved changes) ──────────────────────────────────────────
    bool isDirty()      const { return m_dirty; }
    void markDirty()          { m_dirty = true; }
    void clearDirty()         { m_dirty = false; }

    // ── File path ─────────────────────────────────────────────────────────────
    const std::string& filePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }

private:
    CanvasSize                    m_canvasSize;
    std::vector<std::unique_ptr<Frame>> m_frames;
    Palette                       m_palette;
    int                           m_fps     = 8;
    bool                          m_dirty   = false;
    std::string                   m_filePath;
};

} // namespace Framenote
