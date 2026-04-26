#include "core/Timeline.h"
#include <algorithm>

namespace Framenote {

void Timeline::play() {
    m_playing = true;
    m_accumulator = 0.0f;
}

void Timeline::pause() {
    m_playing    = false;
    m_accumulator = 0.0f;
}

void Timeline::stop() {
    pause();
    setCurrentFrame(0);
}

void Timeline::setCurrentFrame(int index) {
    int clamped = std::clamp(index, 0, m_frameCount - 1);
    if (clamped != m_currentFrame) {
        m_currentFrame = clamped;
        if (onFrameChanged) onFrameChanged(m_currentFrame);
    }
}

void Timeline::nextFrame() {
    int next = m_currentFrame + 1;
    if (next >= m_frameCount) next = m_looping ? 0 : m_frameCount - 1;
    setCurrentFrame(next);
}

void Timeline::prevFrame() {
    int prev = m_currentFrame - 1;
    if (prev < 0) prev = m_looping ? m_frameCount - 1 : 0;
    setCurrentFrame(prev);
}

void Timeline::setFrameCount(int count) {
    m_frameCount = std::max(1, count);
    if (m_currentFrame >= m_frameCount)
        setCurrentFrame(m_frameCount - 1);
}

bool Timeline::tick(float deltaTime) {
    if (!m_playing || m_frameCount <= 1) return false;

    float frameDuration = 1.0f / static_cast<float>(m_fps);
    m_accumulator += deltaTime;

    if (m_accumulator >= frameDuration) {
        m_accumulator -= frameDuration;
        int prev = m_currentFrame;
        nextFrame();
        return m_currentFrame != prev;
    }
    return false;
}

} // namespace Framenote