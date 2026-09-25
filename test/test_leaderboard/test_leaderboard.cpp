// T7 积分榜逻辑测试
#include <unity.h>
#include "leaderboard.h"
#include <cstring>

void setUp() {}
void tearDown() {}

void test_insert_into_empty() {
  Leaderboard b; board_init(b);
  TEST_ASSERT_EQUAL(0, board_insert(b, "SwiftFox", 340));
  TEST_ASSERT_EQUAL(1, b.count);
  TEST_ASSERT_EQUAL_STRING("SwiftFox", b.e[0].name);
  TEST_ASSERT_EQUAL_UINT32(340, b.e[0].score);
}

void test_sorted_descending() {
  Leaderboard b; board_init(b);
  board_insert(b, "A", 100);
  board_insert(b, "B", 500);
  board_insert(b, "C", 300);
  TEST_ASSERT_EQUAL_UINT32(500, b.e[0].score);
  TEST_ASSERT_EQUAL_UINT32(300, b.e[1].score);
  TEST_ASSERT_EQUAL_UINT32(100, b.e[2].score);
  TEST_ASSERT_EQUAL_STRING("B", b.e[0].name);
}

void test_rejected_when_full_and_low() {
  Leaderboard b; board_init(b);
  board_insert(b, "A", 500); board_insert(b, "B", 400); board_insert(b, "C", 300);
  board_insert(b, "D", 200); board_insert(b, "E", 100);
  TEST_ASSERT_EQUAL(-1, board_insert(b, "F", 50));
  TEST_ASSERT_EQUAL(5, b.count);
  TEST_ASSERT_EQUAL_STRING("E", b.e[4].name);   // 榜单未被破坏
}

void test_tie_goes_after() {
  Leaderboard b; board_init(b);
  board_insert(b, "First", 300);
  int pos = board_insert(b, "Second", 300);
  TEST_ASSERT_EQUAL(1, pos);
  TEST_ASSERT_EQUAL_STRING("First", b.e[0].name);
}

void test_long_name_truncated() {
  Leaderboard b; board_init(b);
  board_insert(b, "VeryLongPlayerName12345", 100);
  TEST_ASSERT_EQUAL(15, strlen(b.e[0].name));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_insert_into_empty);
  RUN_TEST(test_sorted_descending);
  RUN_TEST(test_rejected_when_full_and_low);
  RUN_TEST(test_tie_goes_after);
  RUN_TEST(test_long_name_truncated);
  return UNITY_END();
}
