#include <unity.h>

#include "Safety.h"

using namespace puff;

static SafetyInputs ok(uint32_t now) {
  // healthy cell, not charging, 1.5 ohm coil
  return SafetyInputs{now, 4000, false, 1500};
}

void setUp() {}
void tearDown() {}

// ---- defaults are the agreed non-negotiables -------------------------------

void test_default_cutoff_is_10s_and_lock_is_5_clicks() {
  SafetyConfig c;
  TEST_ASSERT_EQUAL_UINT32(10000, c.maxFireMs);
  TEST_ASSERT_EQUAL_UINT8(5, c.lockClicks);
  TEST_ASSERT_FALSE(c.allowFireWhileCharging);
}

// ---- basic fire ------------------------------------------------------------

void test_fires_when_all_ok() {
  Safety s;
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(ok(0)));
  TEST_ASSERT_TRUE(s.firing());
  TEST_ASSERT_EQUAL(FireBlock::None, s.checkWhileFiring(ok(500)));
  s.onButtonReleased();
  TEST_ASSERT_FALSE(s.firing());
}

void test_check_while_not_firing_reports_not_firing() {
  Safety s;
  TEST_ASSERT_EQUAL(FireBlock::NotFiring, s.checkWhileFiring(ok(0)));
}

// ---- 10 s cutoff -------------------------------------------------------------

void test_cutoff_at_10s() {
  Safety s;
  s.requestFire(ok(1000));
  TEST_ASSERT_EQUAL(FireBlock::None, s.checkWhileFiring(ok(1000 + 9999)));
  TEST_ASSERT_EQUAL(FireBlock::Timeout, s.checkWhileFiring(ok(1000 + 10000)));
  TEST_ASSERT_FALSE(s.firing());
}

void test_cutoff_requires_release_before_refire() {
  Safety s;
  s.requestFire(ok(0));
  s.checkWhileFiring(ok(10000));
  TEST_ASSERT_EQUAL(FireBlock::AwaitingRelease, s.requestFire(ok(10001)));
  s.onButtonReleased();
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(ok(10100)));
}

void test_cutoff_survives_millis_wraparound() {
  Safety s;
  const uint32_t start = 0xFFFFF000u;  // ~4 s before wrap
  s.requestFire(ok(start));
  TEST_ASSERT_EQUAL(FireBlock::None, s.checkWhileFiring(ok(start + 9000)));
  TEST_ASSERT_EQUAL(FireBlock::Timeout, s.checkWhileFiring(ok(start + 10000)));
}

void test_repeat_request_while_firing_does_not_reset_timer() {
  Safety s;
  s.requestFire(ok(0));
  s.requestFire(ok(9000));  // spurious second press edge
  TEST_ASSERT_EQUAL(FireBlock::Timeout, s.checkWhileFiring(ok(10000)));
}

// ---- lock ----------------------------------------------------------------------

void test_five_clicks_toggle_lock() {
  Safety s;
  TEST_ASSERT_TRUE(s.onClicks(5));
  TEST_ASSERT_TRUE(s.locked());
  TEST_ASSERT_EQUAL(FireBlock::Locked, s.requestFire(ok(0)));
  TEST_ASSERT_TRUE(s.onClicks(5));
  TEST_ASSERT_FALSE(s.locked());
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(ok(0)));
}

void test_other_click_counts_do_not_toggle_lock() {
  Safety s;
  TEST_ASSERT_FALSE(s.onClicks(3));
  TEST_ASSERT_FALSE(s.onClicks(4));
  TEST_ASSERT_FALSE(s.onClicks(6));
  TEST_ASSERT_FALSE(s.locked());
}

void test_lock_beats_other_blocks() {
  Safety s;
  s.setLocked(true);
  SafetyInputs in = ok(0);
  in.batteryMv = 3000;
  in.coilMilliOhm = 100;
  TEST_ASSERT_EQUAL(FireBlock::Locked, s.requestFire(in));
}

// ---- low voltage ---------------------------------------------------------------

void test_low_voltage_lockout_with_hysteresis() {
  Safety s;
  SafetyInputs in = ok(0);
  in.batteryMv = 3299;
  TEST_ASSERT_EQUAL(FireBlock::LowVoltage, s.requestFire(in));
  in.batteryMv = 3400;  // above cutoff but below recover
  TEST_ASSERT_EQUAL(FireBlock::LowVoltage, s.requestFire(in));
  in.batteryMv = 3450;
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(in));
}

void test_idle_update_tracks_lockout() {
  Safety s;
  s.updateIdle(3200);
  TEST_ASSERT_TRUE(s.lowVoltageLockout());
  s.updateIdle(3500);
  TEST_ASSERT_FALSE(s.lowVoltageLockout());
}

void test_sag_under_load_tolerated_above_load_threshold() {
  Safety s;
  s.requestFire(ok(0));
  SafetyInputs in = ok(100);
  in.batteryMv = 3250;  // below resting cutoff, above under-load cutoff
  TEST_ASSERT_EQUAL(FireBlock::None, s.checkWhileFiring(in));
}

void test_stops_when_sagging_below_load_threshold() {
  Safety s;
  s.requestFire(ok(0));
  SafetyInputs in = ok(100);
  in.batteryMv = 3199;
  TEST_ASSERT_EQUAL(FireBlock::LowVoltage, s.checkWhileFiring(in));
  TEST_ASSERT_FALSE(s.firing());
  TEST_ASSERT_TRUE(s.lowVoltageLockout());
}

// ---- charging ------------------------------------------------------------------

void test_refuses_while_charging() {
  Safety s;
  SafetyInputs in = ok(0);
  in.charging = true;
  TEST_ASSERT_EQUAL(FireBlock::Charging, s.requestFire(in));
}

void test_stops_if_charger_plugged_mid_fire() {
  Safety s;
  s.requestFire(ok(0));
  SafetyInputs in = ok(100);
  in.charging = true;
  TEST_ASSERT_EQUAL(FireBlock::Charging, s.checkWhileFiring(in));
}

// ---- resistance check ----------------------------------------------------------

void test_refuses_short() {
  Safety s;
  SafetyInputs in = ok(0);
  in.coilMilliOhm = 799;
  TEST_ASSERT_EQUAL(FireBlock::Short, s.requestFire(in));
  in.coilMilliOhm = 800;
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(in));
}

void test_refuses_no_cartridge() {
  Safety s;
  SafetyInputs in = ok(0);
  in.coilMilliOhm = 4001;
  TEST_ASSERT_EQUAL(FireBlock::NoCartridge, s.requestFire(in));
}

void test_refuses_unknown_resistance() {
  Safety s;
  SafetyInputs in = ok(0);
  in.coilMilliOhm = -1;
  TEST_ASSERT_EQUAL(FireBlock::SensorFault, s.requestFire(in));
}

void test_stops_if_cart_removed_or_shorted_mid_fire() {
  Safety s;
  s.requestFire(ok(0));
  SafetyInputs in = ok(100);
  in.coilMilliOhm = 50000;
  TEST_ASSERT_EQUAL(FireBlock::NoCartridge, s.checkWhileFiring(in));

  s.onButtonReleased();
  s.requestFire(ok(200));
  in = ok(300);
  in.coilMilliOhm = 100;
  TEST_ASSERT_EQUAL(FireBlock::Short, s.checkWhileFiring(in));
}

// ---- boot guard ----------------------------------------------------------------

void test_require_release_blocks_until_released() {
  Safety s;
  s.requireRelease();  // button was held at boot
  TEST_ASSERT_EQUAL(FireBlock::AwaitingRelease, s.requestFire(ok(0)));
  s.onButtonReleased();
  TEST_ASSERT_EQUAL(FireBlock::None, s.requestFire(ok(10)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_cutoff_is_10s_and_lock_is_5_clicks);
  RUN_TEST(test_fires_when_all_ok);
  RUN_TEST(test_check_while_not_firing_reports_not_firing);
  RUN_TEST(test_cutoff_at_10s);
  RUN_TEST(test_cutoff_requires_release_before_refire);
  RUN_TEST(test_cutoff_survives_millis_wraparound);
  RUN_TEST(test_repeat_request_while_firing_does_not_reset_timer);
  RUN_TEST(test_five_clicks_toggle_lock);
  RUN_TEST(test_other_click_counts_do_not_toggle_lock);
  RUN_TEST(test_lock_beats_other_blocks);
  RUN_TEST(test_low_voltage_lockout_with_hysteresis);
  RUN_TEST(test_idle_update_tracks_lockout);
  RUN_TEST(test_sag_under_load_tolerated_above_load_threshold);
  RUN_TEST(test_stops_when_sagging_below_load_threshold);
  RUN_TEST(test_refuses_while_charging);
  RUN_TEST(test_stops_if_charger_plugged_mid_fire);
  RUN_TEST(test_refuses_short);
  RUN_TEST(test_refuses_no_cartridge);
  RUN_TEST(test_refuses_unknown_resistance);
  RUN_TEST(test_stops_if_cart_removed_or_shorted_mid_fire);
  RUN_TEST(test_require_release_blocks_until_released);
  return UNITY_END();
}
