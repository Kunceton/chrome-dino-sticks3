// native 环境的占位 main：仅在非测试编译时生效（pio test 时 PIO_UNIT_TESTING 已定义，由各测试提供 main）
#ifndef PIO_UNIT_TESTING
int main() { return 0; }
#endif
