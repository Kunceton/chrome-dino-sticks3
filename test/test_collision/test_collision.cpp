// T3 碰撞判定测试（官方碰撞盒 ×0.9）
#include <unity.h>
#include "game_logic.h"

void setUp() {}
void tearDown() {}

static bool dino_hits(const DinoState& d, float dx, ObstacleType t, float ox, float oy, float ow) {
  Rect a[6], b[6];
  int na = dino_collision_boxes(d, dx, a, 6);
  int nb = obstacle_collision_boxes(t, ox, oy, ow, b, 6);
  return boxes_collide(a, na, b, nb);
}

// 站立恐龙脸贴小仙人掌 → 撞
void test_head_on_cactus_collides() {
  DinoState d = dino_create();
  float cactus_x = kDinoX + 20;   // 仙人掌进入恐龙身位
  float cactus_y = kGroundLineY - 32;
  TEST_ASSERT_TRUE(dino_hits(d, kDinoX, ObstacleType::CactusSmall, cactus_x, cactus_y, 15.0f));
}

// 三株仙人掌簇（宽 46）：恐龙碰到簇的右缘也必须死（回归：此前右半簇无碰撞盒）
void test_cluster_right_edge_collides() {
  DinoState d = dino_create();
  float cluster_x = kDinoX - 30;  // 簇右缘 x=16+46=... 使簇右缘压在恐龙身上
  float cactus_y = kGroundLineY - 32;
  // 簇右缘 = cluster_x + 46 = kDinoX + 16，落在恐龙身位内（24~64）
  TEST_ASSERT_TRUE(dino_hits(d, kDinoX, ObstacleType::CactusSmall, cluster_x, cactus_y, 46.0f));
}

// 跳高到顶点（头顶 y≈8，脚 y≈50）越过 32px 高小仙人掌（顶 y=94）→ 不撞
void test_jump_over_small_cactus() {
  DinoState d = dino_create();
  d.y = 8.0f;
  float cactus_y = kGroundLineY - 32;   // 94
  TEST_ASSERT_FALSE(dino_hits(d, kDinoX, ObstacleType::CactusSmall, kDinoX + 10, cactus_y, 15.0f));
}

// 中空翼龙（y=68，高 36 → 底 104）：站立恐龙（头顶 84）撞上；下蹲（头顶 104）穿过
void test_duck_under_mid_ptero() {
  DinoState d = dino_create();
  TEST_ASSERT_TRUE(dino_hits(d, kDinoX, ObstacleType::Ptero, kDinoX + 10, 68.0f, 41.0f));
  d.ducking = true;
  TEST_ASSERT_FALSE(dino_hits(d, kDinoX, ObstacleType::Ptero, kDinoX + 10, 68.0f, 41.0f));
}

// 高空翼龙（y=45，底 81）：站立恐龙头顶 84 > 81 → 直接跑过
void test_run_under_high_ptero() {
  DinoState d = dino_create();
  TEST_ASSERT_FALSE(dino_hits(d, kDinoX, ObstacleType::Ptero, kDinoX + 10, 45.0f, 41.0f));
}

// 水平相离 → 不撞
void test_far_apart_no_collision() {
  DinoState d = dino_create();
  TEST_ASSERT_FALSE(dino_hits(d, kDinoX, ObstacleType::CactusSmall, 200.0f, kGroundLineY - 32, 15.0f));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_head_on_cactus_collides);
  RUN_TEST(test_cluster_right_edge_collides);
  RUN_TEST(test_jump_over_small_cactus);
  RUN_TEST(test_duck_under_mid_ptero);
  RUN_TEST(test_run_under_high_ptero);
  RUN_TEST(test_far_apart_no_collision);
  return UNITY_END();
}
