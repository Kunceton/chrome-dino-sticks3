// T6 名字生成测试
#include <unity.h>
#include "name_gen.h"
#include <cstring>
#include <cctype>

void setUp() {}
void tearDown() {}

struct SeqRng { uint32_t state; };
static uint32_t seq_rng(void* ctx) {
  SeqRng* s = (SeqRng*)ctx;
  s->state = s->state * 1664525u + 1013904223u;
  return s->state >> 8;
}

void test_format_adj_noun() {
  char buf[17];
  SeqRng r{1};
  name_generate(buf, sizeof(buf), seq_rng, &r);
  // 格式：大写开头 + 小写* + 大写 + 小写*
  TEST_ASSERT_TRUE(isupper(buf[0]));
  int i = 1;
  while (islower(buf[i])) i++;
  TEST_ASSERT_TRUE(isupper(buf[i]));   // 中段大写（名词开头）
  TEST_ASSERT_TRUE(i > 1);             // 形容词非空
  int j = i + 1;
  while (islower(buf[j])) j++;
  TEST_ASSERT_EQUAL_CHAR(0, buf[j]);   // 其后即结尾
}

void test_length_limit() {
  char buf[17];
  SeqRng r{42};
  for (int k = 0; k < 100; k++) {
    name_generate(buf, sizeof(buf), seq_rng, &r);
    TEST_ASSERT_LESS_OR_EQUAL(16, strlen(buf));
  }
}

void test_deterministic_with_seed() {
  char a[17], b[17];
  SeqRng r1{777}, r2{777};
  name_generate(a, sizeof(a), seq_rng, &r1);
  name_generate(b, sizeof(b), seq_rng, &r2);
  TEST_ASSERT_EQUAL_STRING(a, b);
}

void test_variety() {
  char seen[60][17];
  int uniq = 0;
  SeqRng r{2024};
  for (int k = 0; k < 200; k++) {
    char buf[17];
    name_generate(buf, sizeof(buf), seq_rng, &r);
    bool dup = false;
    for (int m = 0; m < uniq; m++) if (strcmp(seen[m], buf) == 0) { dup = true; break; }
    if (!dup && uniq < 60) { strcpy(seen[uniq], buf); uniq++; }
  }
  TEST_ASSERT_GREATER_OR_EQUAL(50, uniq);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_format_adj_noun);
  RUN_TEST(test_length_limit);
  RUN_TEST(test_deterministic_with_seed);
  RUN_TEST(test_variety);
  return UNITY_END();
}
