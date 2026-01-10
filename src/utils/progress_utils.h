#pragma once

#include <cstdint>

/**
 * Progress bar calculation utilities
 *
 * Pure functions for calculating progress bar dimensions
 * based on playback state.
 */

namespace ProgressUtils {

/**
 * Calculate progress bar width in pixels
 *
 * @param progressMs Current playback position in milliseconds
 * @param durationMs Total track duration in milliseconds
 * @param maxWidth Maximum width in pixels (screen width)
 * @return Width of the progress bar in pixels (0 to maxWidth)
 */
inline int calculateProgressBarWidth(int progressMs, int durationMs, int maxWidth) {
    // Guard against division by zero
    if (durationMs <= 0) {
        return 0;
    }

    // Guard against negative progress
    if (progressMs < 0) {
        return 0;
    }

    // Calculate proportional width
    // Use int64_t to avoid overflow with large values
    int64_t width = (static_cast<int64_t>(progressMs) * maxWidth) / durationMs;

    // Clamp to valid range
    if (width < 0) {
        return 0;
    }
    if (width > maxWidth) {
        return maxWidth;
    }

    return static_cast<int>(width);
}

/**
 * Calculate progress percentage
 *
 * @param progressMs Current playback position in milliseconds
 * @param durationMs Total track duration in milliseconds
 * @return Percentage (0-100)
 */
inline int calculateProgressPercent(int progressMs, int durationMs) {
    if (durationMs <= 0) {
        return 0;
    }

    if (progressMs < 0) {
        return 0;
    }

    int percent = (progressMs * 100) / durationMs;

    if (percent > 100) {
        return 100;
    }

    return percent;
}

/**
 * Check if progress bar width has changed enough to redraw
 *
 * @param oldWidth Previous bar width
 * @param newWidth New calculated width
 * @param threshold Minimum change to trigger redraw (default 1 pixel)
 * @return true if redraw is needed
 */
inline bool shouldRedrawProgressBar(int oldWidth, int newWidth, int threshold = 1) {
    int diff = newWidth - oldWidth;
    if (diff < 0) diff = -diff;  // abs
    return diff >= threshold;
}

} // namespace ProgressUtils
