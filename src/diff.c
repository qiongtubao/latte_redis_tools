// src/diff.c — 差异检测引擎
#include "fcmp.h"
#include <string.h>

/* 初始化 DiffResult */
static int diff_result_init(DiffResult *dr, size_t capacity) {
    dr->entries = (DiffEntry *)malloc(sizeof(DiffEntry) * capacity);
    if (!dr->entries) return -1;
    dr->count = 0;
    dr->capacity = capacity;
    return 0;
}

/* 追加差异条目 */
static int diff_result_push(DiffResult *dr, DiffEntry entry) {
    if (dr->count >= dr->capacity) {
        size_t new_cap = dr->capacity * 2;
        DiffEntry *new_arr = (DiffEntry *)realloc(dr->entries, sizeof(DiffEntry) * new_cap);
        if (!new_arr) return -1;
        dr->entries = new_arr;
        dr->capacity = new_cap;
    }
    dr->entries[dr->count++] = entry;
    return 0;
}

/* 简化的差异检测：基于分块哈希逐块对比 */
int diff_files(const char *file_a, const char *file_b, size_t chunk_size, DiffResult *out) {
    HashList ha = {0}, hb = {0};

    int ret_a = hash_file(file_a, chunk_size, &ha);
    int ret_b = hash_file(file_b, chunk_size, &hb);
    if (ret_a != 0 || ret_b != 0) {
        hash_list_free(&ha);
        hash_list_free(&hb);
        return -1;
    }

    if (diff_result_init(out, 64) != 0) {
        hash_list_free(&ha);
        hash_list_free(&hb);
        return -2;
    }

    size_t max_len = ha.count > hb.count ? ha.count : hb.count;

    for (size_t i = 0; i < max_len; i++) {
        DiffEntry entry = {0};

        if (i >= ha.count) {
            /* A 文件已结束，B 还有内容 → INSERT */
            entry.type = DIFF_INSERT;
            entry.line_a = -1;
            entry.line_b = (long)i + 1;
            entry.content = strdup("[新增块]");
        } else if (i >= hb.count) {
            /* B 文件已结束，A 还有内容 → DELETE */
            entry.type = DIFF_DELETE;
            entry.line_a = (long)i + 1;
            entry.line_b = -1;
            entry.content = strdup("[删除块]");
        } else if (ha.hashes[i] != hb.hashes[i]) {
            /* 两边都有但哈希不同 → MODIFY */
            entry.type = DIFF_MODIFY;
            entry.line_a = (long)i + 1;
            entry.line_b = (long)i + 1;
            entry.content = strdup("[内容修改]");
        } else {
            /* 相同，跳过 */
            continue;
        }

        if (diff_result_push(out, entry) != 0) {
            hash_list_free(&ha);
            hash_list_free(&hb);
            diff_result_free(out);
            return -3;
        }
    }

    hash_list_free(&ha);
    hash_list_free(&hb);
    return 0;
}

/* 释放 DiffResult 资源 */
void diff_result_free(DiffResult *dr) {
    if (dr->entries) {
        for (size_t i = 0; i < dr->count; i++) {
            if (dr->entries[i].content) {
                free(dr->entries[i].content);
            }
        }
        free(dr->entries);
        dr->entries = NULL;
    }
    dr->count = 0;
    dr->capacity = 0;
}
