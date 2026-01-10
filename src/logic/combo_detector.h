#pragma once

#include <cstdint>

/**
 * Button combo detection for reset/logout functionality
 *
 * Detects held button combinations and determines action based on duration:
 * - 2-10 seconds: Logout pending
 * - 10-20 seconds: Reset pending
 * - 20+ seconds: Factory reset
 */

namespace ComboDetector {

/**
 * Actions that can result from button combos
 */
enum class ComboAction {
    NONE,           // No action (hold too short or released)
    LOGOUT,         // Logout from Spotify (2-10s hold, released)
    RESET,          // Device reset (10-20s hold, released)
    FACTORY_RESET   // Factory reset (20s+ hold)
};

/**
 * Pending state during hold (for UI feedback)
 */
enum class ComboPendingState {
    NONE,           // Not holding combo
    LOGOUT_PENDING, // Showing logout countdown
    RESET_PENDING   // Showing reset countdown
};

// Timing thresholds in milliseconds
constexpr unsigned long COMBO_NONE_THRESHOLD_MS = 2000;
constexpr unsigned long COMBO_LOGOUT_THRESHOLD_MS = 10000;
constexpr unsigned long COMBO_RESET_THRESHOLD_MS = 20000;

/**
 * Determine action when combo is released
 *
 * @param holdDurationMs How long the combo was held
 * @return Action to execute based on hold duration
 */
inline ComboAction determineComboActionOnRelease(unsigned long holdDurationMs) {
    if (holdDurationMs >= COMBO_RESET_THRESHOLD_MS) {
        return ComboAction::FACTORY_RESET;
    }
    if (holdDurationMs >= COMBO_LOGOUT_THRESHOLD_MS) {
        return ComboAction::RESET;
    }
    if (holdDurationMs >= COMBO_NONE_THRESHOLD_MS) {
        return ComboAction::LOGOUT;
    }
    return ComboAction::NONE;
}

/**
 * Determine pending state while combo is held (for UI feedback)
 *
 * @param holdDurationMs Current hold duration
 * @return Pending state for UI display
 */
inline ComboPendingState getComboPendingState(unsigned long holdDurationMs) {
    if (holdDurationMs >= COMBO_LOGOUT_THRESHOLD_MS) {
        return ComboPendingState::RESET_PENDING;
    }
    if (holdDurationMs >= COMBO_NONE_THRESHOLD_MS) {
        return ComboPendingState::LOGOUT_PENDING;
    }
    return ComboPendingState::NONE;
}

/**
 * Calculate countdown seconds for UI display
 *
 * @param holdDurationMs Current hold duration
 * @return Seconds remaining until next action threshold
 */
inline int getCountdownSeconds(unsigned long holdDurationMs) {
    if (holdDurationMs < COMBO_NONE_THRESHOLD_MS) {
        return static_cast<int>((COMBO_NONE_THRESHOLD_MS - holdDurationMs) / 1000);
    }
    if (holdDurationMs < COMBO_LOGOUT_THRESHOLD_MS) {
        return static_cast<int>((COMBO_LOGOUT_THRESHOLD_MS - holdDurationMs) / 1000);
    }
    if (holdDurationMs < COMBO_RESET_THRESHOLD_MS) {
        return static_cast<int>((COMBO_RESET_THRESHOLD_MS - holdDurationMs) / 1000);
    }
    return 0;
}

/**
 * Check if factory reset threshold has been reached (immediate action)
 *
 * @param holdDurationMs Current hold duration
 * @return true if factory reset should trigger immediately
 */
inline bool shouldTriggerFactoryReset(unsigned long holdDurationMs) {
    return holdDurationMs >= COMBO_RESET_THRESHOLD_MS;
}

} // namespace ComboDetector
