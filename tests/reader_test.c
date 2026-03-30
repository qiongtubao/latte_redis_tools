// tests/reader_test.c — 分块读取器测试
#include "fcmp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试正常读取 */
static void test_reader_normal(void) {
    printf("  [TEST] reader_normal... ");

    /* 创建测试文件 */
    const char *test_file = "/tmp/fcmp_test_normal.txt";
    FILE *f = fopen(test_file, "wb");
    assert(f != NULL);
    const char *data = "Hello World! This is a test file for chunk reader.";
    fwrite(data, 1, strlen(data), f);
    fclose(f);

    /* 测试分块读取 */
    ChunkReader r;
    assert(reader_open(&r, test_file, 16) == 0);

    int total = 0;
    int chunks = 0;
    int n;
    while ((n = reader_read_chunk(&r)) > 0) {
        total += n;
        chunks++;
    }

    assert(total == (int)strlen(data));
    assert(chunks > 1);  /* 数据超过 16 字节，应该分多块 */

    reader_close(&r);
    remove(test_file);

    printf("PASS\n");
}

/* 测试文件不存在 */
static void test_reader_not_found(void) {
    printf("  [TEST] reader_not_found... ");

    ChunkReader r;
    int ret = reader_open(&r, "/nonexistent/file/path.txt", 4096);
    assert(ret != 0);

    printf("PASS\n");
}

/* 测试空文件 */
static void test_reader_empty(void) {
    printf("  [TEST] reader_empty... ");

    const char *test_file = "/tmp/fcmp_test_empty.txt";
    FILE *f = fopen(test_file, "wb");
    assert(f != NULL);
    fclose(f);

    ChunkReader r;
    assert(reader_open(&r, test_file, 4096) == 0);

    int n = reader_read_chunk(&r);
    assert(n == 0);  /* 空文件应返回 0 */
    assert(r.eof == true);

    reader_close(&r);
    remove(test_file);

    printf("PASS\n");
}

int main(void) {
    printf("=== Reader Tests ===\n");
    test_reader_normal();
    test_reader_not_found();
    test_reader_empty();
    printf("=== All Reader Tests Passed ===\n");
    return 0;
}
