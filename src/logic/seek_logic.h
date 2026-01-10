#pragma once

#include <cstdint>

/**
 * Track seek/navigation logic
 *
 * Smart previous track behavior:
 * - If less than 10 seconds into track: go to previous track
 * - If more than 10 seconds into track: seek to beginning
 */

namespace SeekLogic {

// Threshold for "restart track" vs "previous track" (10 seconds)
constexpr long SEEK_TO_START_THRESHOLD_MS = 10000;

/**
 * Determine if "previous" should seek to start or go to previous track
 *
 * @param currentProgressMs Current playback position in milliseconds
 * @param isPlaying Whether playback is currently active
 * @param elapsedSinceLastUpdate Milliseconds since last Spotify API update
 * @return true if should seek to start, false if should go to previous track
 */
inline bool shouldSeekToStart(long currentProgressMs, bool isPlaying,
                              unsigned long elapsedSinceLastUpdate) {
    // Estimate actual progress accounting for time since last API poll
    long estimatedProgress = currentProgressMs;

    if (isPlaying) {
        estimatedProgress += static_cast<long>(elapsedSinceLastUpdate);
    }

    return estimatedProgress > SEEK_TO_START_THRESHOLD_MS;
}

/**
 * Simplified version when elapsed time is unknown
 *
 * @param currentProgressMs Current playback position in milliseconds
 * @return true if should seek to start, false if should go to previous track
 */
inline bool shouldSeekToStart(long currentProgressMs) {
    return currentProgressMs > SEEK_TO_START_THRESHOLD_MS;
}

/**
 * Action to take when "previous" button is pressed
 */
enum class PreviousAction {
    GO_TO_PREVIOUS_TRACK,  // Skip to previous track
    SEEK_TO_START          // Restart current track from beginning
};

/**
 * Determine previous button action
 *
 * @param currentProgressMs Current playback position in milliseconds
 * @param isPlaying Whether playback is currently active
 * @param elapsedSinceLastUpdate Milliseconds since last Spotify API update
 * @return Action to take
 */
inline PreviousAction determinePreviousAction(long currentProgressMs, bool isPlaying,
                                               unsigned long elapsedSinceLastUpdate) {
    if (shouldSeekToStart(currentProgressMs, isPlaying, elapsedSinceLastUpdate)) {
        return PreviousAction::SEEK_TO_START;
    }
    return PreviousAction::GO_TO_PREVIOUS_TRACK;
}

} // namespace SeekLogic
