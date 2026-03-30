// src/hash.c — 分块哈希计算
#include "fcmp.h"
#include <string.h>

/* 简单的 FNV-1a 32-bit 哈希 */
uint32_t hash_block(const uint8_t *data, size_t len) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

/* 初始化 HashList */
static int hash_list_init(HashList *hl, size_t capacity) {
    hl->hashes = (uint32_t *)malloc(sizeof(uint32_t) * capacity);
    if (!hl->hashes) return -1;
    hl->count = 0;
    hl->capacity = capacity;
    return 0;
}

/* 追加哈希值 */
static int hash_list_push(HashList *hl, uint32_t hash) {
    if (hl->count >= hl->capacity) {
        size_t new_cap = hl->capacity * 2;
        uint32_t *new_arr = (uint32_t *)realloc(hl->hashes, sizeof(uint32_t) * new_cap);
        if (!new_arr) return -1;
        hl->hashes = new_arr;
        hl->capacity = new_cap;
    }
    hl->hashes[hl->count++] = hash;
    return 0;
}

/* 对整个文件分块计算哈希列表 */
int hash_file(const char *path, size_t chunk_size, HashList *out) {
    ChunkReader r;
    if (reader_open(&r, path, chunk_size) != 0) {
        return -1;
    }

    if (hash_list_init(out, 64) != 0) {
        reader_close(&r);
        return -2;
    }

    int n;
    while ((n = reader_read_chunk(&r)) > 0) {
        uint32_t h = hash_block(r.buffer, (size_t)n);
        if (hash_list_push(out, h) != 0) {
            reader_close(&r);
            hash_list_free(out);
            return -3;
        }
    }

    reader_close(&r);
    return 0;
}

/* 释放 HashList 资源 */
void hash_list_free(HashList *hl) {
    if (hl->hashes) {
        free(hl->hashes);
        hl->hashes = NULL;
    }
    hl->count = 0;
    hl->capacity = 0;
}
