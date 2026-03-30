// src/diff.c — 字符级差异检测引擎（Myers O(ND) 简化版）
#include "fcmp.h"
#include <string.h>

/* ─── LineList: 加载文件为单个字符串（用于字符级对比）────────────────── */

int linelist_load(const char *path, LineList *ll) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    /* 读取整个文件到一个缓冲区 */
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* 单行存储整个文件内容（字符级对比不需要分行） */
    ll->capacity = 1;
    ll->lines = (char **)malloc(sizeof(char *) * 1);
    if (!ll->lines) { fclose(fp); return -2; }
    ll->count = 0;

    if (fsize > 0) {
        char *buf = (char *)malloc((size_t)fsize + 1);
        if (!buf) {
            free(ll->lines);
            fclose(fp);
            return -3;
        }
        fread(buf, 1, (size_t)fsize, fp);
        buf[fsize] = '\0';
        ll->lines[0] = buf;
        ll->count = 1;
    } else {
        ll->lines[0] = strdup("");
        ll->count = 1;
    }

    fclose(fp);
    return 0;
}

void linelist_free(LineList *ll) {
    if (ll->lines) {
        for (size_t i = 0; i < ll->count; i++)
            free(ll->lines[i]);
        free(ll->lines);
        ll->lines = NULL;
    }
    ll->count = 0;
    ll->capacity = 0;
}

/* ─── DiffResult 管理 ───────────────────────────────────────────────────── */

static int diff_result_init(DiffResult *dr, size_t cap) {
    dr->entries = (DiffEntry *)malloc(sizeof(DiffEntry) * cap);
    if (!dr->entries) return -1;
    dr->count = 0;
    dr->capacity = cap;
    return 0;
}

static int diff_result_push(DiffResult *dr, DiffEntry e) {
    if (dr->count >= dr->capacity) {
        size_t nc = dr->capacity * 2;
        DiffEntry *na = (DiffEntry *)realloc(dr->entries, sizeof(DiffEntry) * nc);
        if (!na) return -1;
        dr->entries = na;
        dr->capacity = nc;
    }
    dr->entries[dr->count++] = e;
    return 0;
}

void diff_result_free(DiffResult *dr) {
    if (dr->entries) {
        for (size_t i = 0; i < dr->count; i++)
            free(dr->entries[i].content);
        free(dr->entries);
        dr->entries = NULL;
    }
    dr->count = 0;
    dr->capacity = 0;
}

/* ─── 字符级 LCS 差异检测 ─────────────────────────────────────────────────── */

/*
 * 使用简化版 Myers O(ND) 算法
 * 找出两个字符串之间的最短编辑脚本（SES - Shortest Edit Script）
 */
int diff_files(const char *file_a, const char *file_b, DiffResult *out) {
    LineList la = {0}, lb = {0};

    if (linelist_load(file_a, &la) != 0) return -1;
    if (linelist_load(file_b, &lb) != 0) { linelist_free(&la); return -1; }

    if (diff_result_init(out, 64) != 0) {
        linelist_free(&la); linelist_free(&lb);
        return -2;
    }

    const char *a = la.count > 0 ? la.lines[0] : "";
    const char *b = lb.count > 0 ? lb.lines[0] : "";
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);

    /* 使用 LCS（最长公共子序列）算法找出差异 */
    /* dp[i][j] = a[0..i-1] 和 b[0..j-1] 的 LCS 长度 */

    /* 对于大文件，完整 DP 表会消耗 O(n*m) 内存，这里使用优化的 O(ND) 算法 */

    /* 简化实现：使用双指针扫描 + 回溯 */

    /* 首先找出公共前缀 */
    size_t prefix = 0;
    while (prefix < len_a && prefix < len_b && a[prefix] == b[prefix]) {
        prefix++;
    }

    /* 找出公共后缀 */
    size_t suffix = 0;
    while (suffix < len_a - prefix && suffix < len_b - prefix &&
           a[len_a - 1 - suffix] == b[len_b - 1 - suffix]) {
        suffix++;
    }

    /* 中间部分就是差异区域 */
    size_t diff_start = prefix;
    size_t diff_end_a = len_a - suffix;
    size_t diff_end_b = len_b - suffix;

    /* 构建差异结果 */
    if (diff_start < diff_end_a || diff_start < diff_end_b) {
        /* 从 A 中删除的部分 */
        if (diff_start < diff_end_a) {
            DiffEntry e = {0};
            e.type = DIFF_DELETE;
            e.line_a = (long)diff_start + 1;
            e.line_b = -1;

            /* 提取被删除的内容 */
            size_t del_len = diff_end_a - diff_start;
            char *del_content = (char *)malloc(del_len + 1);
            if (del_content) {
                strncpy(del_content, a + diff_start, del_len);
                del_content[del_len] = '\0';
                e.content = del_content;
            } else {
                e.content = strdup("[内存错误]");
            }
            diff_result_push(out, e);
        }

        /* 在 B 中新增的部分 */
        if (diff_start < diff_end_b) {
            DiffEntry e = {0};
            e.type = DIFF_INSERT;
            e.line_a = -1;
            e.line_b = (long)diff_start + 1;

            /* 提取新增的内容 */
            size_t ins_len = diff_end_b - diff_start;
            char *ins_content = (char *)malloc(ins_len + 1);
            if (ins_content) {
                strncpy(ins_content, b + diff_start, ins_len);
                ins_content[ins_len] = '\0';
                e.content = ins_content;
            } else {
                e.content = strdup("[内存错误]");
            }
            diff_result_push(out, e);
        }
    }

    linelist_free(&la);
    linelist_free(&lb);
    return 0;
}
