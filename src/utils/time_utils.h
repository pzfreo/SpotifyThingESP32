#pragma once

#include <cstdint>
#include <cstdio>

/**
 * Time formatting utilities for Spotify playback display
 *
 * These are pure functions with no hardware dependencies,
 * making them easy to unit test.
 */

namespace TimeUtils {

/**
 * Represents formatted playback time
 */
struct PlaybackTime {
    int currentMinutes;
    int currentSeconds;
    int totalMinutes;
    int totalSeconds;
};

/**
 * Calculate playback time components from milliseconds
 *
 * @param progressMs Current playback position in milliseconds
 * @param durationMs Total track duration in milliseconds
 * @return PlaybackTime struct with formatted components
 */
inline PlaybackTime calculatePlaybackTime(int progressMs, int durationMs) {
    PlaybackTime result = {0, 0, 0, 0};

    if (progressMs >= 0) {
        result.currentMinutes = progressMs / 60000;
        result.currentSeconds = (progressMs / 1000) % 60;
    }

    if (durationMs > 0) {
        result.totalMinutes = durationMs / 60000;
        result.totalSeconds = (durationMs / 1000) % 60;
    }

    return result;
}

/**
 * Format playback time as string "MM:SS / MM:SS"
 *
 * @param progressMs Current playback position in milliseconds
 * @param durationMs Total track duration in milliseconds
 * @param buffer Output buffer (must be at least 16 bytes)
 * @param bufferSize Size of the output buffer
 */
inline void formatPlaybackTimeString(int progressMs, int durationMs,
                                     char* buffer, size_t bufferSize) {
    PlaybackTime time = calculatePlaybackTime(progressMs, durationMs);
    snprintf(buffer, bufferSize, "%02d:%02d / %02d:%02d",
             time.currentMinutes, time.currentSeconds,
             time.totalMinutes, time.totalSeconds);
}

/**
 * Format current time only as string "MM:SS"
 *
 * @param progressMs Current playback position in milliseconds
 * @param buffer Output buffer (must be at least 8 bytes)
 * @param bufferSize Size of the output buffer
 */
inline void formatCurrentTimeString(int progressMs, char* buffer, size_t bufferSize) {
    int minutes = progressMs >= 0 ? progressMs / 60000 : 0;
    int seconds = progressMs >= 0 ? (progressMs / 1000) % 60 : 0;
    snprintf(buffer, bufferSize, "%02d:%02d", minutes, seconds);
}

} // namespace TimeUtils
