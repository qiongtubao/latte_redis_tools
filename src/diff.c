// src/diff.c — 字符级差异检测引擎（LCS 算法）
#include "fcmp.h"
#include <string.h>

/* ─── 配置 ────────────────────────────────────────────── */

/* 字符级 LCS 最大区域大小（超过则降级为块对比） */
#define LCS_CHAR_LIMIT 10000

/* 大文件降级块大小 */
#define FALLBACK_BLOCK 64

/* ─── 文件加载（支持二进制，使用 fsize 而非 strlen）────────────────────── */

int linelist_load(const char *path, LineList *ll) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    ll->capacity = 2;
    ll->lines = (char **)malloc(sizeof(char *) * 2);
    if (!ll->lines) { fclose(fp); return -2; }
    ll->count = 0;

    if (fsize > 0) {
        char *buf = (char *)malloc((size_t)fsize + 1);
        if (!buf) { free(ll->lines); fclose(fp); return -3; }
        size_t nread = fread(buf, 1, (size_t)fsize, fp);
        buf[nread] = '\0';
        ll->lines[0] = buf;
        ll->lines[1] = NULL;
        ll->count = 2; /* lines[0]=data, lines[1]=NULL 终止标记 */
    } else {
        ll->lines[0] = strdup("");
        ll->lines[1] = NULL;
        ll->count = 2;
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

/* ─── LCS 字符级 Diff ───────────────────────────────────────────────────── */

/*
 * 对两个差异区域做字符级 LCS，生成编辑脚本
 * a, na: A 文件的差异区域
 * b, nb: B 文件的差异区域
 * pos_offset: 该区域在原始文件中的起始偏移
 */
static int lcs_diff_region(
    const char *a, size_t na,
    const char *b, size_t nb,
    long pos_offset,
    DiffResult *out
) {
    /* 一方为空 */
    if (na == 0) {
        DiffEntry e = {0};
        e.type = DIFF_INSERT;
        e.line_a = -1;
        e.line_b = pos_offset + 1;
        e.content = (char *)malloc(nb + 1);
        if (e.content) { memcpy(e.content, b, nb); e.content[nb] = '\0'; }
        diff_result_push(out, e);
        return 0;
    }
    if (nb == 0) {
        DiffEntry e = {0};
        e.type = DIFF_DELETE;
        e.line_a = pos_offset + 1;
        e.line_b = -1;
        e.content = (char *)malloc(na + 1);
        if (e.content) { memcpy(e.content, a, na); e.content[na] = '\0'; }
        diff_result_push(out, e);
        return 0;
    }

    /* 大区域降级：按块逐一对比 */
    if (na > LCS_CHAR_LIMIT || nb > LCS_CHAR_LIMIT) {
        size_t i = 0, j = 0;
        while (i < na || j < nb) {
            size_t block_a = na - i;
            size_t block_b = nb - j;
            if (block_a > FALLBACK_BLOCK) block_a = FALLBACK_BLOCK;
            if (block_b > FALLBACK_BLOCK) block_b = FALLBACK_BLOCK;

            if (block_a == block_b && memcmp(a + i, b + j, block_a) == 0) {
                /* 块相同，跳过 */
                i += block_a;
                j += block_b;
            } else if (block_b > 0 && block_a == 0) {
                DiffEntry e = {0};
                e.type = DIFF_INSERT;
                e.line_a = -1;
                e.line_b = pos_offset + (long)j + 1;
                e.content = (char *)malloc(block_b + 1);
                if (e.content) { memcpy(e.content, b + j, block_b); e.content[block_b] = '\0'; }
                diff_result_push(out, e);
                j += block_b;
            } else if (block_a > 0 && block_b == 0) {
                DiffEntry e = {0};
                e.type = DIFF_DELETE;
                e.line_a = pos_offset + (long)i + 1;
                e.line_b = -1;
                e.content = (char *)malloc(block_a + 1);
                if (e.content) { memcpy(e.content, a + i, block_a); e.content[block_a] = '\0'; }
                diff_result_push(out, e);
                i += block_a;
            } else {
                /* 两边都有但不同 → 先输出 DELETE 再 INSERT */
                DiffEntry de = {0};
                de.type = DIFF_DELETE;
                de.line_a = pos_offset + (long)i + 1;
                de.line_b = -1;
                de.content = (char *)malloc(block_a + 1);
                if (de.content) { memcpy(de.content, a + i, block_a); de.content[block_a] = '\0'; }
                diff_result_push(out, de);

                DiffEntry ie = {0};
                ie.type = DIFF_INSERT;
                ie.line_a = -1;
                ie.line_b = pos_offset + (long)j + 1;
                ie.content = (char *)malloc(block_b + 1);
                if (ie.content) { memcpy(ie.content, b + j, block_b); ie.content[block_b] = '\0'; }
                diff_result_push(out, ie);

                i += block_a;
                j += block_b;
            }
        }
        return 0;
    }

    /* 标准 LCS DP + 回溯 */
    size_t rows = na + 1, cols = nb + 1;
    int *dp = (int *)malloc(rows * cols * sizeof(int));
    if (!dp) return -1;

    #define DP(r, c) dp[(r) * cols + (c)]

    for (size_t i = 0; i <= na; i++) DP(i, 0) = 0;
    for (size_t j = 0; j <= nb; j++) DP(0, j) = 0;

    for (size_t i = 1; i <= na; i++) {
        for (size_t j = 1; j <= nb; j++) {
            if (a[i - 1] == b[j - 1]) {
                DP(i, j) = DP(i - 1, j - 1) + 1;
            } else {
                DP(i, j) = DP(i - 1, j) > DP(i, j - 1) ? DP(i - 1, j) : DP(i, j - 1);
            }
        }
    }

    /* 回溯，收集编辑操作（从后向前） */
    typedef struct { DiffType type; long pos; char ch; } EditOp;
    EditOp *ops = (EditOp *)malloc((na + nb) * sizeof(EditOp));
    int opcount = 0;

    size_t i = na, j = nb;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
            i--; j--;
        } else if (j > 0 && (i == 0 || DP(i, j - 1) >= DP(i - 1, j))) {
            ops[opcount].type = DIFF_INSERT;
            ops[opcount].pos = (long)(j - 1) + pos_offset + 1;
            ops[opcount].ch = b[j - 1];
            opcount++;
            j--;
        } else {
            ops[opcount].type = DIFF_DELETE;
            ops[opcount].pos = (long)(i - 1) + pos_offset + 1;
            ops[opcount].ch = a[i - 1];
            opcount++;
            i--;
        }
    }

    /* 反转（改为从前向后） */
    for (int k = 0; k < opcount / 2; k++) {
        EditOp tmp = ops[k];
        ops[k] = ops[opcount - 1 - k];
        ops[opcount - 1 - k] = tmp;
    }

    /* 合并连续相同类型操作 */
    int k = 0;
    while (k < opcount) {
        DiffType cur_type = ops[k].type;
        long first_pos = ops[k].pos;

        /* 收集连续内容 */
        size_t buf_cap = 64;
        char *buf = (char *)malloc(buf_cap);
        size_t blen = 0;

        while (k < opcount && ops[k].type == cur_type) {
            if (blen + 2 >= buf_cap) {
                buf_cap *= 2;
                buf = (char *)realloc(buf, buf_cap);
            }
            buf[blen++] = ops[k].ch;
            k++;
        }
        buf[blen] = '\0';

        DiffEntry e = {0};
        e.type = cur_type;
        e.content = buf;

        if (cur_type == DIFF_DELETE) {
            e.line_a = first_pos;
            e.line_b = -1;
        } else {
            e.line_a = -1;
            e.line_b = first_pos;
        }

        diff_result_push(out, e);
    }

    free(ops);
    free(dp);
    #undef DP
    return 0;
}

/* ─── 主函数 ──────────────────────────────────────────── */

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

    /* 1. 找公共前缀 */
    size_t prefix = 0;
    while (prefix < len_a && prefix < len_b && a[prefix] == b[prefix]) {
        prefix++;
    }

    /* 2. 找公共后缀 */
    size_t suffix = 0;
    while (suffix < len_a - prefix && suffix < len_b - prefix &&
           a[len_a - 1 - suffix] == b[len_b - 1 - suffix]) {
        suffix++;
    }

    /* 3. 中间差异区域 */
    size_t na = len_a - prefix - suffix;
    size_t nb = len_b - prefix - suffix;

    if (na > 0 || nb > 0) {
        lcs_diff_region(a + prefix, na, b + prefix, nb,
                        (long)prefix, out);
    }

    linelist_free(&la);
    linelist_free(&lb);
    return 0;
}
