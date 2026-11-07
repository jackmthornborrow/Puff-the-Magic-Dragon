#include <unity.h>

#include "Button.h"

using namespace puff;

void setUp() {}
void tearDown() {}

// ---- debounce --------------------------------------------------------------

void test_debounce_ignores_bounce() {
  Debouncer d(20);
  TEST_ASSERT_FALSE(d.update(true, 0));
  TEST_ASSERT_FALSE(d.update(false, 5));
  TEST_ASSERT_FALSE(d.update(true, 10));
  TEST_ASSERT_FALSE(d.update(true, 25));  // only 15 ms stable
  TEST_ASSERT_FALSE(d.pressed());
}

void test_debounce_accepts_stable_press_and_release() {
  Debouncer d(20);
  d.update(true, 100);
  TEST_ASSERT_TRUE(d.update(true, 120));
  TEST_ASSERT_TRUE(d.pressed());
  TEST_ASSERT_FALSE(d.update(true, 130));  // no repeated edge
  d.update(false, 200);
  TEST_ASSERT_TRUE(d.update(false, 220));
  TEST_ASSERT_FALSE(d.pressed());
}

// ---- clicks ------------------------------------------------------------------

static void click(ClickCounter& c, uint32_t at, uint32_t len = 100) {
  c.onPress(at);
  c.onRelease(at + len);
}

void test_three_clicks_reported_after_gap() {
  ClickCounter c(400, 500);
  click(c, 0);
  click(c, 300);
  click(c, 600);  // last release at 700
  TEST_ASSERT_EQUAL_UINT8(0, c.poll(1199));
  TEST_ASSERT_EQUAL_UINT8(3, c.poll(1200));
  TEST_ASSERT_EQUAL_UINT8(0, c.poll(1300));  // reported once
}

void test_five_clicks() {
  ClickCounter c(400, 500);
  for (int i = 0; i < 5; i++) click(c, i * 250);
  TEST_ASSERT_EQUAL_UINT8(5, c.poll(10000));
}

void test_long_hold_cancels_group() {
  ClickCounter c(400, 500);
  click(c, 0);
  click(c, 300);
  c.onPress(600);
  TEST_ASSERT_EQUAL_UINT8(0, c.poll(1100));  // holding: a puff, not a click
  c.onRelease(2000);
  TEST_ASSERT_EQUAL_UINT8(0, c.poll(5000));
}

void test_held_ms() {
  ClickCounter c;
  TEST_ASSERT_EQUAL_UINT32(0, c.heldMs(100));
  c.onPress(100);
  TEST_ASSERT_EQUAL_UINT32(3000, c.heldMs(3100));
  c.onRelease(3200);
  TEST_ASSERT_EQUAL_UINT32(0, c.heldMs(3300));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_debounce_ignores_bounce);
  RUN_TEST(test_debounce_accepts_stable_press_and_release);
  RUN_TEST(test_three_clicks_reported_after_gap);
  RUN_TEST(test_five_clicks);
  RUN_TEST(test_long_hold_cancels_group);
  RUN_TEST(test_held_ms);
  return UNITY_END();
}
