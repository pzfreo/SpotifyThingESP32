#include <unity.h>
#include "logic/combo_detector.h"

using namespace ComboDetector;

void setUp(void) {}
void tearDown(void) {}

// ============================================================
// determineComboActionOnRelease() tests
// ============================================================

void test_comboAction_quick_tap(void) {
    // Less than 2 seconds = no action
    ComboAction action = determineComboActionOnRelease(500);
    TEST_ASSERT_EQUAL(ComboAction::NONE, action);
}

void test_comboAction_short_hold(void) {
    // 1.5 seconds = no action
    ComboAction action = determineComboActionOnRelease(1500);
    TEST_ASSERT_EQUAL(ComboAction::NONE, action);
}

void test_comboAction_at_threshold(void) {
    // Exactly 2 seconds = no action (need to exceed)
    ComboAction action = determineComboActionOnRelease(2000);
    TEST_ASSERT_EQUAL(ComboAction::NONE, action);
}

void test_comboAction_logout_range_start(void) {
    // Just over 2 seconds = logout
    ComboAction action = determineComboActionOnRelease(2001);
    TEST_ASSERT_EQUAL(ComboAction::LOGOUT, action);
}

void test_comboAction_logout_range_middle(void) {
    // 5 seconds = logout
    ComboAction action = determineComboActionOnRelease(5000);
    TEST_ASSERT_EQUAL(ComboAction::LOGOUT, action);
}

void test_comboAction_logout_range_end(void) {
    // 9.9 seconds = still logout
    ComboAction action = determineComboActionOnRelease(9999);
    TEST_ASSERT_EQUAL(ComboAction::LOGOUT, action);
}

void test_comboAction_reset_range_start(void) {
    // Exactly 10 seconds = reset (crosses into reset range)
    ComboAction action = determineComboActionOnRelease(10000);
    TEST_ASSERT_EQUAL(ComboAction::RESET, action);
}

void test_comboAction_reset_range_middle(void) {
    // 15 seconds = reset
    ComboAction action = determineComboActionOnRelease(15000);
    TEST_ASSERT_EQUAL(ComboAction::RESET, action);
}

void test_comboAction_reset_range_end(void) {
    // 19.9 seconds = still reset
    ComboAction action = determineComboActionOnRelease(19999);
    TEST_ASSERT_EQUAL(ComboAction::RESET, action);
}

void test_comboAction_factory_reset(void) {
    // 20+ seconds = factory reset
    ComboAction action = determineComboActionOnRelease(20000);
    TEST_ASSERT_EQUAL(ComboAction::FACTORY_RESET, action);
}

void test_comboAction_long_factory_reset(void) {
    // 30 seconds = still factory reset
    ComboAction action = determineComboActionOnRelease(30000);
    TEST_ASSERT_EQUAL(ComboAction::FACTORY_RESET, action);
}

// ============================================================
// getComboPendingState() tests
// ============================================================

void test_pendingState_none(void) {
    ComboPendingState state = getComboPendingState(1000);
    TEST_ASSERT_EQUAL(ComboPendingState::NONE, state);
}

void test_pendingState_logout_pending(void) {
    ComboPendingState state = getComboPendingState(5000);
    TEST_ASSERT_EQUAL(ComboPendingState::LOGOUT_PENDING, state);
}

void test_pendingState_reset_pending(void) {
    ComboPendingState state = getComboPendingState(15000);
    TEST_ASSERT_EQUAL(ComboPendingState::RESET_PENDING, state);
}

void test_pendingState_past_reset(void) {
    // Even at 20+ seconds, pending state is still RESET_PENDING
    // (factory reset triggers immediately, not on release)
    ComboPendingState state = getComboPendingState(25000);
    TEST_ASSERT_EQUAL(ComboPendingState::RESET_PENDING, state);
}

// ============================================================
// getCountdownSeconds() tests
// ============================================================

void test_countdown_before_logout(void) {
    // 1 second in, 1 second until logout range
    int countdown = getCountdownSeconds(1000);
    TEST_ASSERT_EQUAL_INT(1, countdown);
}

void test_countdown_in_logout_range(void) {
    // 5 seconds in, 5 seconds until reset range
    int countdown = getCountdownSeconds(5000);
    TEST_ASSERT_EQUAL_INT(5, countdown);
}

void test_countdown_in_reset_range(void) {
    // 15 seconds in, 5 seconds until factory reset
    int countdown = getCountdownSeconds(15000);
    TEST_ASSERT_EQUAL_INT(5, countdown);
}

void test_countdown_at_factory_reset(void) {
    // Past factory reset threshold
    int countdown = getCountdownSeconds(21000);
    TEST_ASSERT_EQUAL_INT(0, countdown);
}

// ============================================================
// shouldTriggerFactoryReset() tests
// ============================================================

void test_factoryReset_not_yet(void) {
    bool should = shouldTriggerFactoryReset(15000);
    TEST_ASSERT_FALSE(should);
}

void test_factoryReset_at_threshold(void) {
    bool should = shouldTriggerFactoryReset(20000);
    TEST_ASSERT_TRUE(should);
}

void test_factoryReset_past_threshold(void) {
    bool should = shouldTriggerFactoryReset(25000);
    TEST_ASSERT_TRUE(should);
}

// ============================================================
// Test runner
// ============================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // determineComboActionOnRelease tests
    RUN_TEST(test_comboAction_quick_tap);
    RUN_TEST(test_comboAction_short_hold);
    RUN_TEST(test_comboAction_at_threshold);
    RUN_TEST(test_comboAction_logout_range_start);
    RUN_TEST(test_comboAction_logout_range_middle);
    RUN_TEST(test_comboAction_logout_range_end);
    RUN_TEST(test_comboAction_reset_range_start);
    RUN_TEST(test_comboAction_reset_range_middle);
    RUN_TEST(test_comboAction_reset_range_end);
    RUN_TEST(test_comboAction_factory_reset);
    RUN_TEST(test_comboAction_long_factory_reset);

    // getComboPendingState tests
    RUN_TEST(test_pendingState_none);
    RUN_TEST(test_pendingState_logout_pending);
    RUN_TEST(test_pendingState_reset_pending);
    RUN_TEST(test_pendingState_past_reset);

    // getCountdownSeconds tests
    RUN_TEST(test_countdown_before_logout);
    RUN_TEST(test_countdown_in_logout_range);
    RUN_TEST(test_countdown_in_reset_range);
    RUN_TEST(test_countdown_at_factory_reset);

    // shouldTriggerFactoryReset tests
    RUN_TEST(test_factoryReset_not_yet);
    RUN_TEST(test_factoryReset_at_threshold);
    RUN_TEST(test_factoryReset_past_threshold);

    return UNITY_END();
}
