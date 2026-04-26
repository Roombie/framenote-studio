#pragma once

#include <functional>

namespace Framenote {

// Manages animation playback state.
// Completely decoupled from SDL — call tick() with delta time each frame.
class Timeline {
public:
    Timeline() = default;

    // ── Playback control ──────────────────────────────────────────────────────
    void play();
    void pause();
    void stop();   // pause + rewind to frame 0
    bool isPlaying() const { return m_playing; }

    // ── Frame navigation ──────────────────────────────────────────────────────
    int  currentFrame()  const { return m_currentFrame; }
    void setCurrentFrame(int index);
    void nextFrame();
    void prevFrame();

    // ── Settings ──────────────────────────────────────────────────────────────
    int  fps()         const { return m_fps; }
    void setFps(int fps)     { m_fps = fps > 0 ? fps : 1; }

    int  frameCount()  const { return m_frameCount; }
    void setFrameCount(int count);

    bool looping()     const { return m_looping; }
    void setLooping(bool loop) { m_looping = loop; }

    // ── Onion skinning ────────────────────────────────────────────────────────
    bool onionSkinEnabled()    const { return m_onionSkin; }
    void setOnionSkin(bool on)       { m_onionSkin = on; }
    float onionSkinOpacity()   const { return m_onionOpacity; }
    void  setOnionSkinOpacity(float v) { m_onionOpacity = v < 0.05f ? 0.05f : (v > 1.f ? 1.f : v); }

    // ── Tick: call every render loop with deltaTime in seconds ────────────────
    // Returns true if the current frame changed (so renderer knows to redraw)
    bool tick(float deltaTime);

    // ── Callback: fired when frame changes ────────────────────────────────────
    std::function<void(int frameIndex)> onFrameChanged;

private:
    int   m_currentFrame  = 0;
    int   m_frameCount    = 1;
    int   m_fps           = 8;
    bool  m_playing       = false;
    bool  m_looping       = true;
    bool  m_onionSkin     = true;
    float m_onionOpacity  = 0.4f;
    float m_accumulator   = 0.0f; // accumulated time since last frame advance
};

} // namespace Framenote