#include <unity.h>
#include "utils/time_utils.h"

using namespace TimeUtils;

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

// ============================================================
// calculatePlaybackTime() tests
// ============================================================

void test_calculatePlaybackTime_zero_progress(void) {
    PlaybackTime result = calculatePlaybackTime(0, 180000);

    TEST_ASSERT_EQUAL_INT(0, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.currentSeconds);
    TEST_ASSERT_EQUAL_INT(3, result.totalMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.totalSeconds);
}

void test_calculatePlaybackTime_halfway(void) {
    PlaybackTime result = calculatePlaybackTime(90000, 180000);  // 1:30 of 3:00

    TEST_ASSERT_EQUAL_INT(1, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(30, result.currentSeconds);
    TEST_ASSERT_EQUAL_INT(3, result.totalMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.totalSeconds);
}

void test_calculatePlaybackTime_near_end(void) {
    PlaybackTime result = calculatePlaybackTime(179000, 180000);  // 2:59 of 3:00

    TEST_ASSERT_EQUAL_INT(2, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(59, result.currentSeconds);
}

void test_calculatePlaybackTime_long_track(void) {
    // 1 hour progress, 2 hour duration
    PlaybackTime result = calculatePlaybackTime(3600000, 7200000);

    TEST_ASSERT_EQUAL_INT(60, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.currentSeconds);
    TEST_ASSERT_EQUAL_INT(120, result.totalMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.totalSeconds);
}

void test_calculatePlaybackTime_zero_duration(void) {
    PlaybackTime result = calculatePlaybackTime(5000, 0);

    // Current time should still be calculated
    TEST_ASSERT_EQUAL_INT(0, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(5, result.currentSeconds);
    // Total should be zero
    TEST_ASSERT_EQUAL_INT(0, result.totalMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.totalSeconds);
}

void test_calculatePlaybackTime_negative_progress(void) {
    PlaybackTime result = calculatePlaybackTime(-1000, 180000);

    // Should handle gracefully (treated as 0)
    TEST_ASSERT_EQUAL_INT(0, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.currentSeconds);
}

void test_calculatePlaybackTime_exact_minute(void) {
    PlaybackTime result = calculatePlaybackTime(120000, 300000);  // 2:00 of 5:00

    TEST_ASSERT_EQUAL_INT(2, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.currentSeconds);
    TEST_ASSERT_EQUAL_INT(5, result.totalMinutes);
    TEST_ASSERT_EQUAL_INT(0, result.totalSeconds);
}

void test_calculatePlaybackTime_seconds_rollover(void) {
    // 1:01 should not be displayed as 0:61
    PlaybackTime result = calculatePlaybackTime(61000, 180000);

    TEST_ASSERT_EQUAL_INT(1, result.currentMinutes);
    TEST_ASSERT_EQUAL_INT(1, result.currentSeconds);
}

// ============================================================
// formatPlaybackTimeString() tests
// ============================================================

void test_formatPlaybackTimeString_basic(void) {
    char buffer[32];
    formatPlaybackTimeString(90000, 180000, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING("01:30 / 03:00", buffer);
}

void test_formatPlaybackTimeString_zero(void) {
    char buffer[32];
    formatPlaybackTimeString(0, 0, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING("00:00 / 00:00", buffer);
}

void test_formatPlaybackTimeString_leading_zeros(void) {
    char buffer[32];
    formatPlaybackTimeString(5000, 65000, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING("00:05 / 01:05", buffer);
}

// ============================================================
// formatCurrentTimeString() tests
// ============================================================

void test_formatCurrentTimeString_basic(void) {
    char buffer[16];
    formatCurrentTimeString(125000, buffer, sizeof(buffer));  // 2:05

    TEST_ASSERT_EQUAL_STRING("02:05", buffer);
}

void test_formatCurrentTimeString_zero(void) {
    char buffer[16];
    formatCurrentTimeString(0, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_STRING("00:00", buffer);
}

// ============================================================
// Test runner
// ============================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // calculatePlaybackTime tests
    RUN_TEST(test_calculatePlaybackTime_zero_progress);
    RUN_TEST(test_calculatePlaybackTime_halfway);
    RUN_TEST(test_calculatePlaybackTime_near_end);
    RUN_TEST(test_calculatePlaybackTime_long_track);
    RUN_TEST(test_calculatePlaybackTime_zero_duration);
    RUN_TEST(test_calculatePlaybackTime_negative_progress);
    RUN_TEST(test_calculatePlaybackTime_exact_minute);
    RUN_TEST(test_calculatePlaybackTime_seconds_rollover);

    // formatPlaybackTimeString tests
    RUN_TEST(test_formatPlaybackTimeString_basic);
    RUN_TEST(test_formatPlaybackTimeString_zero);
    RUN_TEST(test_formatPlaybackTimeString_leading_zeros);

    // formatCurrentTimeString tests
    RUN_TEST(test_formatCurrentTimeString_basic);
    RUN_TEST(test_formatCurrentTimeString_zero);

    return UNITY_END();
}
