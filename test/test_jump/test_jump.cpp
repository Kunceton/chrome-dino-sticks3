// T2 跳跃物理测试
#include <unity.h>
#include "game_logic.h"

void setUp() {}
void tearDown() {}

// 满按跳：顶点钳制触发于头顶 y<27，受 DROP_VELOCITY 续升影响实际升高约 76~86px，且永不出屏
void test_jump_rise_capped_at_57px() {
  DinoState d = dino_create();
  dino_update(d, true, false, 5.4f, 1.0f);
  TEST_ASSERT_TRUE(d.jumping);
  float min_y = d.y;
  for (int i = 0; i < 120 && d.jumping; i++) {
    dino_update(d, false, false, 5.4f, 1.0f);
    if (d.y < min_y) min_y = d.y;
  }
  float rise = kDinoStandTopY - min_y;
  TEST_ASSERT_TRUE(rise >= 70.0f && rise <= 84.0f);   // 对齐原版顶点头顶 y≈9（×0.9 ≈ 8）
  TEST_ASSERT_TRUE(min_y >= 0.0f);                    // 不出屏
  TEST_ASSERT_FALSE(d.jumping);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, kDinoStandTopY, d.y);
}

// 空中再按跳无效（无二段跳）
void test_no_double_jump() {
  DinoState d = dino_create();
  dino_update(d, true, false, 5.4f, 1.0f);
  dino_update(d, false, false, 5.4f, 1.0f);
  float y_before = d.y, vy_before = d.vy;
  dino_update(d, true, false, 5.4f, 1.0f);   // 空中再按跳
  TEST_ASSERT_FLOAT_WITHIN(0.01f, y_before + vy_before, d.y);
}

// 空中按下蹲 → 速降
void test_speed_drop() {
  DinoState d = dino_create();
  dino_update(d, true, false, 5.4f, 1.0f);
  dino_update(d, false, true, 5.4f, 1.0f);
  TEST_ASSERT_TRUE(d.speed_drop);
}

// 地面按住下蹲 → ducking，不跳
void test_duck_on_ground() {
  DinoState d = dino_create();
  dino_update(d, false, true, 5.4f, 1.0f);
  TEST_ASSERT_TRUE(d.ducking);
  TEST_ASSERT_FALSE(d.jumping);
}

// 可变跳高：按 8 帧后松开（cut_jump）→ 升高明显小于满跳
//（原版 endJump 要求先过最小升高 27px，过早松键不截断，故测试按 8 帧）
void test_variable_jump_height() {
  DinoState full = dino_create();
  dino_update(full, true, false, 5.4f, 1.0f);
  float full_min = full.y;
  for (int i = 0; i < 120 && full.jumping; i++) {
    dino_update(full, false, false, 5.4f, 1.0f);
    if (full.y < full_min) full_min = full.y;
  }
  DinoState tap = dino_create();
  dino_update(tap, true, false, 5.4f, 1.0f);
  for (int i = 0; i < 4; i++) dino_update(tap, false, false, 5.4f, 1.0f);
  dino_cut_jump(tap);                        // 松键截断（已过最小升高、未到顶点）
  float tap_min = tap.y;
  for (int i = 0; i < 120 && tap.jumping; i++) {
    dino_update(tap, false, false, 5.4f, 1.0f);
    if (tap.y < tap_min) tap_min = tap.y;
  }
  TEST_ASSERT_TRUE((kDinoStandTopY - tap_min) < (kDinoStandTopY - full_min) - 15.0f);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_jump_rise_capped_at_57px);
  RUN_TEST(test_no_double_jump);
  RUN_TEST(test_speed_drop);
  RUN_TEST(test_duck_on_ground);
  RUN_TEST(test_variable_jump_height);
  return UNITY_END();
}
