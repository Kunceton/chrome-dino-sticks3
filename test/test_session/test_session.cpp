// T5 会话测试
#include <unity.h>
#include "game_session.h"

void setUp() {}
void tearDown() {}

void test_speed_caps_at_max() {
  GameSession s; session_init(s);
  for (int i = 0; i < 200000; i++) session_update(s, 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, kSpeedMax, s.speed);
}

void test_score_from_distance() {
  GameSession s; session_init(s);
  s.distance_px = 40000.0f;
  session_update(s, 0.0f);   // 仅刷新分数
  TEST_ASSERT_EQUAL_UINT32((uint32_t)(40000.0f * kScoreCoef), s.score);
}

void test_milestone_events() {
  GameSession s; session_init(s);
  s.distance_px = 100.0f / kScoreCoef - 12.0f;   // ≈99.5 分
  uint32_t e = session_update(s, 5.0f);          // 推过 100 分
  TEST_ASSERT_TRUE(e & kEvtMilestone);
  e = session_update(s, 1.0f);                   // 下一帧不重复触发
  TEST_ASSERT_FALSE(e & kEvtMilestone);
  s.distance_px = 200.0f / kScoreCoef - 12.0f;   // ≈199.5 分
  e = session_update(s, 5.0f);                   // 过 200
  TEST_ASSERT_TRUE(e & kEvtMilestone);
}

void test_night_flips_every_700() {
  GameSession s; session_init(s);
  TEST_ASSERT_FALSE(s.night);
  s.distance_px = 700.0f / kScoreCoef;           // 700 分
  uint32_t e = session_update(s, 0.0f);
  TEST_ASSERT_TRUE(e & kEvtNightFlip);
  TEST_ASSERT_TRUE(s.night);
  s.distance_px = 1400.0f / kScoreCoef;          // 1400 分
  e = session_update(s, 0.0f);
  TEST_ASSERT_TRUE(e & kEvtNightFlip);
  TEST_ASSERT_FALSE(s.night);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_speed_caps_at_max);
  RUN_TEST(test_score_from_distance);
  RUN_TEST(test_milestone_events);
  RUN_TEST(test_night_flips_every_700);
  return UNITY_END();
}
