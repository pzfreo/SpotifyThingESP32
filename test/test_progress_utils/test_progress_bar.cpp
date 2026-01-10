#include <unity.h>
#include "utils/progress_utils.h"

using namespace ProgressUtils;

void setUp(void) {}
void tearDown(void) {}

// ============================================================
// calculateProgressBarWidth() tests
// ============================================================

void test_calculateProgressBarWidth_empty(void) {
    int width = calculateProgressBarWidth(0, 100000, 480);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_calculateProgressBarWidth_half(void) {
    int width = calculateProgressBarWidth(50000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(240, width);
}

void test_calculateProgressBarWidth_full(void) {
    int width = calculateProgressBarWidth(100000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(480, width);
}

void test_calculateProgressBarWidth_over_100_percent(void) {
    // Progress beyond duration should clamp to max
    int width = calculateProgressBarWidth(120000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(480, width);  // Clamped to max
}

void test_calculateProgressBarWidth_zero_duration(void) {
    // Division by zero protection
    int width = calculateProgressBarWidth(50000, 0, 480);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_calculateProgressBarWidth_negative_duration(void) {
    int width = calculateProgressBarWidth(50000, -100, 480);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_calculateProgressBarWidth_negative_progress(void) {
    int width = calculateProgressBarWidth(-5000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(0, width);
}

void test_calculateProgressBarWidth_quarter(void) {
    int width = calculateProgressBarWidth(25000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(120, width);
}

void test_calculateProgressBarWidth_three_quarters(void) {
    int width = calculateProgressBarWidth(75000, 100000, 480);
    TEST_ASSERT_EQUAL_INT(360, width);
}

void test_calculateProgressBarWidth_large_values(void) {
    // Test with large millisecond values (3 hour track at 2 hours)
    int width = calculateProgressBarWidth(7200000, 10800000, 480);
    TEST_ASSERT_EQUAL_INT(320, width);  // 2/3 of 480
}

void test_calculateProgressBarWidth_small_screen(void) {
    // Test with different screen width
    int width = calculateProgressBarWidth(50000, 100000, 100);
    TEST_ASSERT_EQUAL_INT(50, width);
}

// ============================================================
// calculateProgressPercent() tests
// ============================================================

void test_calculateProgressPercent_empty(void) {
    int percent = calculateProgressPercent(0, 100000);
    TEST_ASSERT_EQUAL_INT(0, percent);
}

void test_calculateProgressPercent_half(void) {
    int percent = calculateProgressPercent(50000, 100000);
    TEST_ASSERT_EQUAL_INT(50, percent);
}

void test_calculateProgressPercent_full(void) {
    int percent = calculateProgressPercent(100000, 100000);
    TEST_ASSERT_EQUAL_INT(100, percent);
}

void test_calculateProgressPercent_over_100(void) {
    int percent = calculateProgressPercent(150000, 100000);
    TEST_ASSERT_EQUAL_INT(100, percent);  // Clamped
}

void test_calculateProgressPercent_zero_duration(void) {
    int percent = calculateProgressPercent(50000, 0);
    TEST_ASSERT_EQUAL_INT(0, percent);
}

// ============================================================
// shouldRedrawProgressBar() tests
// ============================================================

void test_shouldRedrawProgressBar_no_change(void) {
    bool shouldRedraw = shouldRedrawProgressBar(100, 100);
    TEST_ASSERT_FALSE(shouldRedraw);
}

void test_shouldRedrawProgressBar_small_change(void) {
    bool shouldRedraw = shouldRedrawProgressBar(100, 101);
    TEST_ASSERT_TRUE(shouldRedraw);  // 1 pixel change triggers redraw
}

void test_shouldRedrawProgressBar_large_change(void) {
    bool shouldRedraw = shouldRedrawProgressBar(100, 150);
    TEST_ASSERT_TRUE(shouldRedraw);
}

void test_shouldRedrawProgressBar_decrease(void) {
    // Progress can go backwards (seek)
    bool shouldRedraw = shouldRedrawProgressBar(150, 100);
    TEST_ASSERT_TRUE(shouldRedraw);
}

void test_shouldRedrawProgressBar_custom_threshold(void) {
    // With threshold of 5, changes < 5 should not trigger redraw
    bool shouldRedraw = shouldRedrawProgressBar(100, 103, 5);
    TEST_ASSERT_FALSE(shouldRedraw);

    shouldRedraw = shouldRedrawProgressBar(100, 105, 5);
    TEST_ASSERT_TRUE(shouldRedraw);
}

// ============================================================
// Test runner
// ============================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // calculateProgressBarWidth tests
    RUN_TEST(test_calculateProgressBarWidth_empty);
    RUN_TEST(test_calculateProgressBarWidth_half);
    RUN_TEST(test_calculateProgressBarWidth_full);
    RUN_TEST(test_calculateProgressBarWidth_over_100_percent);
    RUN_TEST(test_calculateProgressBarWidth_zero_duration);
    RUN_TEST(test_calculateProgressBarWidth_negative_duration);
    RUN_TEST(test_calculateProgressBarWidth_negative_progress);
    RUN_TEST(test_calculateProgressBarWidth_quarter);
    RUN_TEST(test_calculateProgressBarWidth_three_quarters);
    RUN_TEST(test_calculateProgressBarWidth_large_values);
    RUN_TEST(test_calculateProgressBarWidth_small_screen);

    // calculateProgressPercent tests
    RUN_TEST(test_calculateProgressPercent_empty);
    RUN_TEST(test_calculateProgressPercent_half);
    RUN_TEST(test_calculateProgressPercent_full);
    RUN_TEST(test_calculateProgressPercent_over_100);
    RUN_TEST(test_calculateProgressPercent_zero_duration);

    // shouldRedrawProgressBar tests
    RUN_TEST(test_shouldRedrawProgressBar_no_change);
    RUN_TEST(test_shouldRedrawProgressBar_small_change);
    RUN_TEST(test_shouldRedrawProgressBar_large_change);
    RUN_TEST(test_shouldRedrawProgressBar_decrease);
    RUN_TEST(test_shouldRedrawProgressBar_custom_threshold);

    return UNITY_END();
}
