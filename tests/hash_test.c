// tests/hash_test.c — 哈希模块测试
#include "fcmp.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试相同数据哈希一致 */
static void test_hash_consistency(void) {
    printf("  [TEST] hash_consistency... ");

    uint8_t data[] = {1, 2, 3, 4, 5};
    uint32_t h1 = hash_block(data, 5);
    uint32_t h2 = hash_block(data, 5);

    assert(h1 == h2);
    assert(h1 != 0);

    printf("PASS (hash=0x%08x)\n", h1);
}

/* 测试不同数据哈希不同 */
static void test_hash_difference(void) {
    printf("  [TEST] hash_difference... ");

    uint8_t data1[] = {1, 2, 3, 4, 5};
    uint8_t data2[] = {1, 2, 3, 4, 6};  /* 最后一个字节不同 */

    uint32_t h1 = hash_block(data1, 5);
    uint32_t h2 = hash_block(data2, 5);

    assert(h1 != h2);

    printf("PASS\n");
}

/* 测试空数据 */
static void test_hash_empty(void) {
    printf("  [TEST] hash_empty... ");

    uint32_t h = hash_block(NULL, 0);
    /* FNV-1a 初始值 */
    assert(h == 2166136261u);

    printf("PASS\n");
}

/* 测试文件哈希 */
static void test_hash_file(void) {
    printf("  [TEST] hash_file... ");

    /* 创建测试文件 */
    const char *test_file = "/tmp/fcmp_test_hash.txt";
    FILE *f = fopen(test_file, "wb");
    assert(f != NULL);
    const char *data = "AAAA";  /* 4 字节 */
    fwrite(data, 1, strlen(data), f);
    fclose(f);

    /* 用 2 字节块大小，应得到 2 个哈希 */
    HashList hl;
    int ret = hash_file(test_file, 2, &hl);
    assert(ret == 0);
    assert(hl.count == 2);  /* "AAAA" 分成 "AA" + "AA" */
    assert(hl.hashes[0] == hl.hashes[1]);  /* 两块相同，哈希相同 */

    hash_list_free(&hl);
    remove(test_file);

    printf("PASS\n");
}

int main(void) {
    printf("=== Hash Tests ===\n");
    test_hash_consistency();
    test_hash_difference();
    test_hash_empty();
    test_hash_file();
    printf("=== All Hash Tests Passed ===\n");
    return 0;
}
