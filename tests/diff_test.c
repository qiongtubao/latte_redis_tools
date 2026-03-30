// tests/diff_test.c — 差异检测测试
#include "fcmp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 创建临时测试文件 */
static void create_test_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fwrite(content, 1, strlen(content), f);
    fclose(f);
}

/* 测试相同文件 */
static void test_diff_identical(void) {
    printf("  [TEST] diff_identical... ");

    const char *file_a = "/tmp/fcmp_diff_a.txt";
    const char *file_b = "/tmp/fcmp_diff_b.txt";
    create_test_file(file_a, "Same content");
    create_test_file(file_b, "Same content");

    DiffResult dr;
    int ret = diff_files(file_a, file_b, 4, &dr);
    assert(ret == 0);
    assert(dr.count == 0);  /* 相同文件无差异 */

    diff_result_free(&dr);
    remove(file_a);
    remove(file_b);

    printf("PASS\n");
}

/* 测试新增内容 */
static void test_diff_insert(void) {
    printf("  [TEST] diff_insert... ");

    const char *file_a = "/tmp/fcmp_diff_a.txt";
    const char *file_b = "/tmp/fcmp_diff_b.txt";
    create_test_file(file_a, "AB");    /* 2 字节 */
    create_test_file(file_b, "ABCD");  /* 4 字节，多了 CD */

    DiffResult dr;
    int ret = diff_files(file_a, file_b, 2, &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_INSERT);

    diff_result_free(&dr);
    remove(file_a);
    remove(file_b);

    printf("PASS\n");
}

/* 测试删除内容 */
static void test_diff_delete(void) {
    printf("  [TEST] diff_delete... ");

    const char *file_a = "/tmp/fcmp_diff_a.txt";
    const char *file_b = "/tmp/fcmp_diff_b.txt";
    create_test_file(file_a, "ABCD");  /* 4 字节 */
    create_test_file(file_b, "AB");    /* 2 字节，少了 CD */

    DiffResult dr;
    int ret = diff_files(file_a, file_b, 2, &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_DELETE);

    diff_result_free(&dr);
    remove(file_a);
    remove(file_b);

    printf("PASS\n");
}

/* 测试修改内容 */
static void test_diff_modify(void) {
    printf("  [TEST] diff_modify... ");

    const char *file_a = "/tmp/fcmp_diff_a.txt";
    const char *file_b = "/tmp/fcmp_diff_b.txt";
    create_test_file(file_a, "AB");
    create_test_file(file_b, "AX");  /* 第二个字节不同 */

    DiffResult dr;
    int ret = diff_files(file_a, file_b, 2, &dr);
    assert(ret == 0);
    assert(dr.count == 1);
    assert(dr.entries[0].type == DIFF_MODIFY);

    diff_result_free(&dr);
    remove(file_a);
    remove(file_b);

    printf("PASS\n");
}

int main(void) {
    printf("=== Diff Tests ===\n");
    test_diff_identical();
    test_diff_insert();
    test_diff_delete();
    test_diff_modify();
    printf("=== All Diff Tests Passed ===\n");
    return 0;
}
