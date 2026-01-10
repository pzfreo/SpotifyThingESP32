#pragma once

#include <cstdint>

/**
 * Volume control utilities
 *
 * Pure functions for volume calculation and boundary checking.
 */

namespace VolumeUtils {

// Volume boundaries
constexpr int MIN_VOLUME = 0;
constexpr int MAX_VOLUME = 100;

// Default volume step for increment/decrement
constexpr int DEFAULT_VOLUME_STEP = 10;

// Timing constants for volume repeat
constexpr unsigned long VOLUME_HOLD_DELAY_MS = 800;   // Initial delay before repeat
constexpr unsigned long VOLUME_REPEAT_INTERVAL_MS = 500;  // Repeat interval

/**
 * Clamp volume to valid range [0, 100]
 *
 * @param volume Volume value to clamp
 * @return Clamped volume value
 */
inline int clampVolume(int volume) {
    if (volume < MIN_VOLUME) return MIN_VOLUME;
    if (volume > MAX_VOLUME) return MAX_VOLUME;
    return volume;
}

/**
 * Calculate new volume with delta, clamped to valid range
 *
 * @param currentVolume Current volume (0-100)
 * @param delta Change amount (positive or negative)
 * @return New volume clamped to [0, 100]
 */
inline int calculateNewVolume(int currentVolume, int delta) {
    return clampVolume(currentVolume + delta);
}

/**
 * Check if volume repeat should trigger
 *
 * @param buttonPressedAt Timestamp when button was first pressed
 * @param lastRepeatAt Timestamp of last volume repeat
 * @param now Current timestamp
 * @return true if volume change should be triggered
 */
inline bool shouldRepeatVolumeChange(unsigned long buttonPressedAt,
                                     unsigned long lastRepeatAt,
                                     unsigned long now) {
    // Check if initial hold delay has passed
    if (now - buttonPressedAt < VOLUME_HOLD_DELAY_MS) {
        return false;
    }

    // Check if repeat interval has passed since last change
    if (now - lastRepeatAt < VOLUME_REPEAT_INTERVAL_MS) {
        return false;
    }

    return true;
}

/**
 * Determine volume delta based on which button is pressed
 *
 * @param nextPressed Is the NEXT button pressed (volume up)
 * @param prevPressed Is the PREV button pressed (volume down)
 * @return Volume delta (+step, -step, or 0)
 */
inline int determineVolumeDelta(bool nextPressed, bool prevPressed) {
    if (nextPressed && !prevPressed) {
        return DEFAULT_VOLUME_STEP;
    }
    if (prevPressed && !nextPressed) {
        return -DEFAULT_VOLUME_STEP;
    }
    return 0;  // Neither or both pressed
}

} // namespace VolumeUtils
