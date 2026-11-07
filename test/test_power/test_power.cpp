#include <unity.h>

#include "OutputMath.h"

using namespace puff;

void setUp() {}
void tearDown() {}

void test_full_duty_when_target_at_or_above_battery() {
  TEST_ASSERT_EQUAL_FLOAT(1.0f, rmsDuty(4000, 3700));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, rmsDuty(3700, 3700));
}

void test_duty_is_voltage_ratio_squared() {
  // (3.0 / 4.2)^2 = 0.510204
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.510204f, rmsDuty(3000, 4200));
  // (2.4 / 3.6)^2 = 0.444444
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.444444f, rmsDuty(2400, 3600));
}

void test_duty_zero_for_bad_inputs() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, rmsDuty(3000, 0));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, rmsDuty(0, 4000));
}

void test_effective_output_capped_by_battery() {
  TEST_ASSERT_EQUAL_UINT16(3500, effectiveOutputMv(4000, 3500));
  TEST_ASSERT_EQUAL_UINT16(2800, effectiveOutputMv(2800, 4100));
}

void test_duty_to_counts() {
  TEST_ASSERT_EQUAL_UINT32(0, dutyToCounts(0.0f, 10));
  TEST_ASSERT_EQUAL_UINT32(1023, dutyToCounts(1.0f, 10));
  TEST_ASSERT_EQUAL_UINT32(1023, dutyToCounts(1.5f, 10));  // clamped
  TEST_ASSERT_EQUAL_UINT32(0, dutyToCounts(-0.5f, 10));
  TEST_ASSERT_EQUAL_UINT32(512, dutyToCounts(0.5f, 10));
  TEST_ASSERT_EQUAL_UINT32(0, dutyToCounts(0.5f, 0));
}

void test_presets_cycle_and_wrap() {
  VoltagePresets p;
  TEST_ASSERT_EQUAL_UINT16(2800, p.currentMv());
  TEST_ASSERT_EQUAL_UINT16(3200, p.next());
  TEST_ASSERT_EQUAL_UINT16(3600, p.next());
  TEST_ASSERT_EQUAL_UINT16(4000, p.next());
  TEST_ASSERT_EQUAL_UINT16(2400, p.next());
}

void test_preset_out_of_range_resets_to_first() {
  VoltagePresets p;
  p.setIndex(99);  // e.g. corrupt NVS value
  TEST_ASSERT_EQUAL_UINT8(0, p.index());
}

void test_all_presets_within_spec_range() {
  for (uint8_t i = 0; i < VoltagePresets::kCount; i++) {
    TEST_ASSERT_TRUE(VoltagePresets::kMv[i] >= 2400);
    TEST_ASSERT_TRUE(VoltagePresets::kMv[i] <= 4000);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_full_duty_when_target_at_or_above_battery);
  RUN_TEST(test_duty_is_voltage_ratio_squared);
  RUN_TEST(test_duty_zero_for_bad_inputs);
  RUN_TEST(test_effective_output_capped_by_battery);
  RUN_TEST(test_duty_to_counts);
  RUN_TEST(test_presets_cycle_and_wrap);
  RUN_TEST(test_preset_out_of_range_resets_to_first);
  RUN_TEST(test_all_presets_within_spec_range);
  return UNITY_END();
}
