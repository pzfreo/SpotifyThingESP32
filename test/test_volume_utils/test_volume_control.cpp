#include <unity.h>
#include "utils/volume_utils.h"

using namespace VolumeUtils;

void setUp(void) {}
void tearDown(void) {}

// ============================================================
// clampVolume() tests
// ============================================================

void test_clampVolume_within_range(void) {
    TEST_ASSERT_EQUAL_INT(50, clampVolume(50));
    TEST_ASSERT_EQUAL_INT(0, clampVolume(0));
    TEST_ASSERT_EQUAL_INT(100, clampVolume(100));
}

void test_clampVolume_above_max(void) {
    TEST_ASSERT_EQUAL_INT(100, clampVolume(101));
    TEST_ASSERT_EQUAL_INT(100, clampVolume(150));
    TEST_ASSERT_EQUAL_INT(100, clampVolume(1000));
}

void test_clampVolume_below_min(void) {
    TEST_ASSERT_EQUAL_INT(0, clampVolume(-1));
    TEST_ASSERT_EQUAL_INT(0, clampVolume(-50));
    TEST_ASSERT_EQUAL_INT(0, clampVolume(-1000));
}

// ============================================================
// calculateNewVolume() tests
// ============================================================

void test_calculateNewVolume_increase(void) {
    TEST_ASSERT_EQUAL_INT(60, calculateNewVolume(50, 10));
    TEST_ASSERT_EQUAL_INT(75, calculateNewVolume(50, 25));
}

void test_calculateNewVolume_decrease(void) {
    TEST_ASSERT_EQUAL_INT(40, calculateNewVolume(50, -10));
    TEST_ASSERT_EQUAL_INT(25, calculateNewVolume(50, -25));
}

void test_calculateNewVolume_clamp_at_max(void) {
    TEST_ASSERT_EQUAL_INT(100, calculateNewVolume(95, 10));
    TEST_ASSERT_EQUAL_INT(100, calculateNewVolume(100, 10));
    TEST_ASSERT_EQUAL_INT(100, calculateNewVolume(90, 50));
}

void test_calculateNewVolume_clamp_at_min(void) {
    TEST_ASSERT_EQUAL_INT(0, calculateNewVolume(5, -10));
    TEST_ASSERT_EQUAL_INT(0, calculateNewVolume(0, -10));
    TEST_ASSERT_EQUAL_INT(0, calculateNewVolume(20, -50));
}

void test_calculateNewVolume_no_change(void) {
    TEST_ASSERT_EQUAL_INT(50, calculateNewVolume(50, 0));
}

// ============================================================
// shouldRepeatVolumeChange() tests
// ============================================================

void test_shouldRepeatVolumeChange_before_initial_delay(void) {
    // Button pressed at time 0, current time 500ms
    // Should NOT repeat (initial delay is 800ms)
    bool should = shouldRepeatVolumeChange(0, 0, 500);
    TEST_ASSERT_FALSE(should);
}

void test_shouldRepeatVolumeChange_after_initial_delay_first_repeat(void) {
    // Button pressed at 0, current time 1000ms, last repeat at 0
    // Should repeat (past 800ms initial delay, past 500ms since last)
    bool should = shouldRepeatVolumeChange(0, 0, 1000);
    TEST_ASSERT_TRUE(should);
}

void test_shouldRepeatVolumeChange_too_soon_after_last_repeat(void) {
    // Button pressed at 0, current time 1200ms, last repeat at 1000ms
    // Should NOT repeat (only 200ms since last repeat, need 500ms)
    bool should = shouldRepeatVolumeChange(0, 1000, 1200);
    TEST_ASSERT_FALSE(should);
}

void test_shouldRepeatVolumeChange_ready_for_second_repeat(void) {
    // Button pressed at 0, current time 1600ms, last repeat at 1000ms
    // Should repeat (600ms since last repeat)
    bool should = shouldRepeatVolumeChange(0, 1000, 1600);
    TEST_ASSERT_TRUE(should);
}

void test_shouldRepeatVolumeChange_exact_threshold(void) {
    // At exactly the threshold
    bool should = shouldRepeatVolumeChange(0, 0, 800);
    TEST_ASSERT_FALSE(should);  // Not strictly greater

    should = shouldRepeatVolumeChange(0, 0, 801);
    TEST_ASSERT_TRUE(should);  // Just past threshold
}

// ============================================================
// determineVolumeDelta() tests
// ============================================================

void test_determineVolumeDelta_next_pressed(void) {
    int delta = determineVolumeDelta(true, false);
    TEST_ASSERT_EQUAL_INT(DEFAULT_VOLUME_STEP, delta);  // +10
}

void test_determineVolumeDelta_prev_pressed(void) {
    int delta = determineVolumeDelta(false, true);
    TEST_ASSERT_EQUAL_INT(-DEFAULT_VOLUME_STEP, delta);  // -10
}

void test_determineVolumeDelta_both_pressed(void) {
    // Both pressed = conflict, no change
    int delta = determineVolumeDelta(true, true);
    TEST_ASSERT_EQUAL_INT(0, delta);
}

void test_determineVolumeDelta_neither_pressed(void) {
    int delta = determineVolumeDelta(false, false);
    TEST_ASSERT_EQUAL_INT(0, delta);
}

// ============================================================
// Test runner
// ============================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // clampVolume tests
    RUN_TEST(test_clampVolume_within_range);
    RUN_TEST(test_clampVolume_above_max);
    RUN_TEST(test_clampVolume_below_min);

    // calculateNewVolume tests
    RUN_TEST(test_calculateNewVolume_increase);
    RUN_TEST(test_calculateNewVolume_decrease);
    RUN_TEST(test_calculateNewVolume_clamp_at_max);
    RUN_TEST(test_calculateNewVolume_clamp_at_min);
    RUN_TEST(test_calculateNewVolume_no_change);

    // shouldRepeatVolumeChange tests
    RUN_TEST(test_shouldRepeatVolumeChange_before_initial_delay);
    RUN_TEST(test_shouldRepeatVolumeChange_after_initial_delay_first_repeat);
    RUN_TEST(test_shouldRepeatVolumeChange_too_soon_after_last_repeat);
    RUN_TEST(test_shouldRepeatVolumeChange_ready_for_second_repeat);
    RUN_TEST(test_shouldRepeatVolumeChange_exact_threshold);

    // determineVolumeDelta tests
    RUN_TEST(test_determineVolumeDelta_next_pressed);
    RUN_TEST(test_determineVolumeDelta_prev_pressed);
    RUN_TEST(test_determineVolumeDelta_both_pressed);
    RUN_TEST(test_determineVolumeDelta_neither_pressed);

    return UNITY_END();
}
