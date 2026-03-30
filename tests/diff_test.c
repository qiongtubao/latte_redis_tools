// tests/diff_test.c — 字符级差异检测测试
#include "fcmp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 创建临时测试文件 */
static void create_test_file(const char *path, const char *content, size_t len) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fwrite(content, 1, len, f);
    fclose(f);
}

/* 测试相同文件 */
static void test_diff_identical(void) {
    printf("  [TEST] diff_identical... ");

    const char *content = "Hello World\n";
    create_test_file("/tmp/fcmp_a.txt", content, strlen(content));
    create_test_file("/tmp/fcmp_b.txt", content, strlen(content));

    DiffResult dr = {0};
    int ret = diff_files("/tmp/fcmp_a.txt", "/tmp/fcmp_b.txt", &dr);
    assert(ret == 0);
    assert(dr.count == 0);

    diff_result_free(&dr);
    printf("PASS\n");
}

/* 测试字符插入 */
static void test_diff_insert(void) {
    printf("  [TEST] diff_insert... ");

    /* "abcccdefg" vs "abcdefg" → "cc" 插入在位置 3 */
    create_test_file("/tmp/fcmp_a.txt", "abcdefg", 7);
    create_test_file("/tmp/fcmp_b.txt", "abcccdefg", 9);

    DiffResult dr = {0};
    int ret = diff_files("/tmp/fcmp_a.txt", "/tmp/fcmp_b.txt", &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_INSERT);
    assert(dr.entries[0].line_b == 4);  /* 位置从1开始，"cc" 在位置4 */
    assert(strcmp(dr.entries[0].content, "cc") == 0);

    diff_result_free(&dr);
    printf("PASS\n");
}

/* 测试字符删除 */
static void test_diff_delete(void) {
    printf("  [TEST] diff_delete... ");

    /* "abcdefg" vs "abdefg" → "c" 被删除 */
    create_test_file("/tmp/fcmp_a.txt", "abcdefg", 7);
    create_test_file("/tmp/fcmp_b.txt", "abdefg", 6);

    DiffResult dr = {0};
    int ret = diff_files("/tmp/fcmp_a.txt", "/tmp/fcmp_b.txt", &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_DELETE);

    diff_result_free(&dr);
    printf("PASS\n");
}

/* 测试二进制内容 */
static void test_diff_binary(void) {
    printf("  [TEST] diff_binary... ");

    /* 二进制文件：A 包含 \x01\x02，B 包含 \x01\x02\x03\x04 */
    unsigned char bin_a[] = {'A', 'B', 'C', 0x01, 0x02, 'D', 'E', 'F'};
    unsigned char bin_b[] = {'A', 'B', 'C', 0x01, 0x02, 0x03, 0x04, 'D', 'E', 'F'};

    create_test_file("/tmp/fcmp_a.bin", (char*)bin_a, sizeof(bin_a));
    create_test_file("/tmp/fcmp_b.bin", (char*)bin_b, sizeof(bin_b));

    DiffResult dr = {0};
    int ret = diff_files("/tmp/fcmp_a.bin", "/tmp/fcmp_b.bin", &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_INSERT);
    /* 内容应该是 \x03\x04 */
    assert(dr.entries[0].content[0] == 0x03);
    assert(dr.entries[0].content[1] == 0x04);

    diff_result_free(&dr);
    printf("PASS\n");
}

/* 测试空文件 */
static void test_diff_empty(void) {
    printf("  [TEST] diff_empty... ");

    create_test_file("/tmp/fcmp_a.txt", "", 0);
    create_test_file("/tmp/fcmp_b.txt", "", 0);

    DiffResult dr = {0};
    int ret = diff_files("/tmp/fcmp_a.txt", "/tmp/fcmp_b.txt", &dr);
    assert(ret == 0);
    assert(dr.count == 0);

    diff_result_free(&dr);
    printf("PASS\n");
}

int main(void) {
    printf("=== Diff Tests (Character-Level) ===\n");
    test_diff_identical();
    test_diff_insert();
    test_diff_delete();
    test_diff_binary();
    test_diff_empty();
    printf("=== All Diff Tests Passed ===\n");
    return 0;
}
