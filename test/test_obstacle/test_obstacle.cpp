// T4 障碍生成器测试
#include <unity.h>
#include "obstacle.h"
#include "game_session.h"

void setUp() {}
void tearDown() {}

// 固定序列伪随机（可复现）
struct SeqRng { uint32_t state; };
static uint32_t seq_rng(void* ctx) {
  SeqRng* s = (SeqRng*)ctx;
  s->state = s->state * 1664525u + 1013904223u;
  return s->state >> 8;
}

// 满速连续生成：相邻间距 ≥ 滞空位移（保证可跳过，ADR-001）
void test_gap_always_jumpable_at_max_speed() {
  ObstaclePool p; pool_init(p);
  SeqRng rng{12345};
  for (int i = 0; i < 20000; i++) pool_update(p, kSpeedMax, kSpeedMax, seq_rng, &rng);
  for (int i = 0; i < kMaxObstacles; i++) {
    if (!p.items[i].active) continue;
    for (int j = 0; j < kMaxObstacles; j++) {
      if (!p.items[j].active || p.items[j].x <= p.items[i].x) continue;
      float gap = p.items[j].x - (p.items[i].x + p.items[i].w);
      TEST_ASSERT_TRUE(gap >= jump_air_distance(kSpeedMax));
    }
  }
}

// 低速不出现翼龙
void test_no_ptero_at_low_speed() {
  ObstaclePool p; pool_init(p);
  SeqRng rng{999};
  for (int i = 0; i < 20000; i++) pool_update(p, kSpeedStart, kSpeedStart, seq_rng, &rng);
  for (int i = 0; i < kMaxObstacles; i++)
    if (p.items[i].active) TEST_ASSERT_NOT_EQUAL((int)ObstacleType::Ptero, (int)p.items[i].type);
}

// 超过阈值速度后出现翼龙（统计意义）
void test_ptero_appears_above_threshold() {
  ObstaclePool p; pool_init(p);
  SeqRng rng{4242};
  int ptero_seen = 0;
  float spd = kPteroMinSpeed + 0.2f;
  for (int i = 0; i < 40000; i++) {
    pool_update(p, spd, spd, seq_rng, &rng);
    for (int k = 0; k < kMaxObstacles; k++)
      if (p.items[k].active && p.items[k].type == ObstacleType::Ptero && p.items[k].x > 200.0f)
        ptero_seen++;
  }
  TEST_ASSERT_TRUE(ptero_seen > 0);
}

// 池上限：激活数永不超 kMaxObstacles
void test_pool_never_overflows() {
  ObstaclePool p; pool_init(p);
  SeqRng rng{7};
  for (int i = 0; i < 5000; i++) {
    pool_update(p, 0.2f, 3.0f, seq_rng, &rng);  // 慢速累积
    int n = 0;
    for (int k = 0; k < kMaxObstacles; k++) if (p.items[k].active) n++;
    TEST_ASSERT_LESS_OR_EQUAL(kMaxObstacles, n);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_gap_always_jumpable_at_max_speed);
  RUN_TEST(test_no_ptero_at_low_speed);
  RUN_TEST(test_ptero_appears_above_threshold);
  RUN_TEST(test_pool_never_overflows);
  return UNITY_END();
}
