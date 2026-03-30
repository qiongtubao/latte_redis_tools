// src/diff.c — 字符级差异检测引擎（流式处理，内存安全）
#include "fcmp.h"
#include <string.h>

/* ─── 配置 ────────────────────────────────────────────── */

/* 贪心搜索的最大前瞻距离 */
#define MAX_LOOKAHEAD 256

/* 流式处理块大小（1MB） */
#define STREAM_BLOCK_SIZE (1024 * 1024)

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
        size_t nc = dr->capacity ? dr->capacity * 2 : 64;
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

/* ─── LineList（保持兼容，用于小文件）──────────────────────────────────── */

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
    ll->data_len = 0;

    if (fsize > 0) {
        char *buf = (char *)malloc((size_t)fsize + 1);
        if (!buf) { free(ll->lines); fclose(fp); return -3; }
        size_t nread = fread(buf, 1, (size_t)fsize, fp);
        buf[nread] = '\0';
        ll->lines[0] = buf;
        ll->lines[1] = NULL;
        ll->count = 2;
        ll->data_len = nread;
    } else {
        ll->lines[0] = strdup("");
        ll->lines[1] = NULL;
        ll->count = 2;
        ll->data_len = 0;
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
    ll->data_len = 0;
}

/* ─── 流式辅助函数 ──────────────────────────────────────────────────────── */

/* 读取指定范围的数据 */
static char *read_range(FILE *fp, size_t start, size_t len) {
    if (len == 0) return strdup("");
    char *buf = (char *)malloc(len + 1);
    if (!buf) return NULL;
    fseek(fp, (long)start, SEEK_SET);
    size_t n = fread(buf, 1, len, fp);
    buf[n] = '\0';
    return buf;
}

/* 读取指定范围的数据 */
static size_t stream_common_prefix(FILE *fa, FILE *fb, size_t len_a, size_t len_b) {
    size_t prefix = 0;
    size_t min_len = len_a < len_b ? len_a : len_b;
    char bufa[STREAM_BLOCK_SIZE], bufb[STREAM_BLOCK_SIZE];

    fseek(fa, 0, SEEK_SET);
    fseek(fb, 0, SEEK_SET);

    while (prefix < min_len) {
        size_t to_read = min_len - prefix;
        if (to_read > STREAM_BLOCK_SIZE) to_read = STREAM_BLOCK_SIZE;

        size_t na = fread(bufa, 1, to_read, fa);
        size_t nb = fread(bufb, 1, to_read, fb);
        if (na != nb || na == 0) break;

        for (size_t i = 0; i < na; i++) {
            if (bufa[i] != bufb[i]) {
                return prefix + i;
            }
        }
        prefix += na;
    }

    return prefix;
}

/* 流式计算公共后缀长度 */
static size_t stream_common_suffix(FILE *fa, FILE *fb, size_t len_a, size_t len_b, size_t prefix) {
    size_t suffix = 0;
    size_t avail_a = len_a > prefix ? len_a - prefix : 0;
    size_t avail_b = len_b > prefix ? len_b - prefix : 0;
    size_t min_avail = avail_a < avail_b ? avail_a : avail_b;

    char bufa[STREAM_BLOCK_SIZE], bufb[STREAM_BLOCK_SIZE];

    while (suffix < min_avail) {
        size_t to_check = min_avail - suffix;
        if (to_check > STREAM_BLOCK_SIZE) to_check = STREAM_BLOCK_SIZE;

        /* 从文件末尾向前读取 */
        size_t read_pos_a = len_a - suffix - to_check;
        size_t read_pos_b = len_b - suffix - to_check;

        fseek(fa, (long)read_pos_a, SEEK_SET);
        fseek(fb, (long)read_pos_b, SEEK_SET);

        size_t na = fread(bufa, 1, to_check, fa);
        size_t nb = fread(bufb, 1, to_check, fb);
        if (na != nb || na == 0) break;

        /* 从后向前比较 */
        for (size_t i = na; i > 0; i--) {
            if (bufa[i-1] != bufb[i-1]) {
                return suffix + (na - i);
            }
        }
        suffix += na;
    }

    return suffix;
}

/* ─── 贪心字符级 Diff（内存安全）────────────────────────────────────────── */

static int greedy_diff_region_streaming(
    FILE *fa, size_t start_a, size_t len_a,
    FILE *fb, size_t start_b, size_t len_b,
    long pos_offset,
    DiffResult *out
) {
    if (len_a == 0 && len_b == 0) return 0;

    /* 一方为空 */
    if (len_a == 0) {
        char *content = read_range(fb, start_b, len_b);
        DiffEntry e = {0};
        e.type = DIFF_INSERT;
        e.line_a = -1;
        e.line_b = pos_offset + 1;
        e.content = content ? content : strdup("");
        diff_result_push(out, e);
        return 0;
    }
    if (len_b == 0) {
        char *content = read_range(fa, start_a, len_a);
        DiffEntry e = {0};
        e.type = DIFF_DELETE;
        e.line_a = pos_offset + 1;
        e.line_b = -1;
        e.content = content ? content : strdup("");
        diff_result_push(out, e);
        return 0;
    }

    /* 对于小区域，直接加载到内存 */
    if (len_a + len_b <= STREAM_BLOCK_SIZE) {
        char *data_a = read_range(fa, start_a, len_a);
        char *data_b = read_range(fb, start_b, len_b);

        /* 贪心对比 */
        size_t i = 0, j = 0;
        typedef struct { DiffType type; long pos; char ch; } EditOp;
        size_t max_ops = len_a + len_b;
        EditOp *ops = (EditOp *)malloc(max_ops * sizeof(EditOp));
        size_t opcount = 0;

        while (i < len_a || j < len_b) {
            if (i < len_a && j < len_b && data_a[i] == data_b[j]) {
                i++; j++;
                continue;
            }

            size_t insert_len = 0;
            if (i < len_a) {
                for (size_t k = j; k < len_b && k - j < MAX_LOOKAHEAD; k++) {
                    if (data_b[k] == data_a[i]) { insert_len = k - j; break; }
                }
            }

            size_t delete_len = 0;
            if (j < len_b) {
                for (size_t k = i; k < len_a && k - i < MAX_LOOKAHEAD; k++) {
                    if (data_a[k] == data_b[j]) { delete_len = k - i; break; }
                }
            }

            if (insert_len > 0 && (delete_len == 0 || insert_len <= delete_len)) {
                for (size_t k = 0; k < insert_len; k++) {
                    ops[opcount].type = DIFF_INSERT;
                    ops[opcount].pos = (long)(j + k) + pos_offset + 1;
                    ops[opcount].ch = data_b[j + k];
                    opcount++;
                }
                j += insert_len;
            } else if (delete_len > 0) {
                for (size_t k = 0; k < delete_len; k++) {
                    ops[opcount].type = DIFF_DELETE;
                    ops[opcount].pos = (long)(i + k) + pos_offset + 1;
                    ops[opcount].ch = data_a[i + k];
                    opcount++;
                }
                i += delete_len;
            } else if (j < len_b && i < len_a) {
                ops[opcount].type = DIFF_DELETE;
                ops[opcount].pos = (long)i + pos_offset + 1;
                ops[opcount].ch = data_a[i];
                opcount++;
                i++;
                ops[opcount].type = DIFF_INSERT;
                ops[opcount].pos = (long)j + pos_offset + 1;
                ops[opcount].ch = data_b[j];
                opcount++;
                j++;
            } else if (j < len_b) {
                ops[opcount].type = DIFF_INSERT;
                ops[opcount].pos = (long)j + pos_offset + 1;
                ops[opcount].ch = data_b[j];
                opcount++;
                j++;
            } else {
                ops[opcount].type = DIFF_DELETE;
                ops[opcount].pos = (long)i + pos_offset + 1;
                ops[opcount].ch = data_a[i];
                opcount++;
                i++;
            }
        }

        /* 合并连续操作 */
        size_t k = 0;
        while (k < opcount) {
            DiffType cur_type = ops[k].type;
            long first_pos = ops[k].pos;
            size_t buf_cap = 128;
            char *buf = (char *)malloc(buf_cap);
            size_t blen = 0;

            while (k < opcount && ops[k].type == cur_type) {
                if (blen + 2 >= buf_cap) { buf_cap *= 2; buf = (char *)realloc(buf, buf_cap); }
                buf[blen++] = ops[k].ch;
                k++;
            }
            buf[blen] = '\0';

            DiffEntry e = {0};
            e.type = cur_type;
            e.content = buf;
            if (cur_type == DIFF_DELETE) { e.line_a = first_pos; e.line_b = -1; }
            else { e.line_a = -1; e.line_b = first_pos; }
            diff_result_push(out, e);
        }

        free(ops);
        free(data_a);
        free(data_b);
        return 0;
    }

    /* 大区域：分块处理 */
    size_t block = STREAM_BLOCK_SIZE / 2;
    size_t i = 0, j = 0;

    while (i < len_a || j < len_b) {
        size_t chunk_a = len_a - i;
        size_t chunk_b = len_b - j;
        if (chunk_a > block) chunk_a = block;
        if (chunk_b > block) chunk_b = block;

        if (chunk_a == chunk_b) {
            char *da = read_range(fa, start_a + i, chunk_a);
            char *db = read_range(fb, start_b + j, chunk_b);
            if (da && db && memcmp(da, db, chunk_a) == 0) {
                i += chunk_a; j += chunk_b;
                free(da); free(db);
                continue;
            }
            free(da); free(db);
        }

        /* 递归处理这块 */
        greedy_diff_region_streaming(fa, start_a + i, chunk_a,
                                      fb, start_b + j, chunk_b,
                                      pos_offset + (long)(i < j ? i : j), out);
        i += chunk_a;
        j += chunk_b;
    }

    return 0;
}

/* ─── 主函数（流式处理）──────────────────────────────────────────────────── */

int diff_files(const char *file_a, const char *file_b, DiffResult *out) {
    FILE *fa = fopen(file_a, "rb");
    FILE *fb = fopen(file_b, "rb");
    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return -1;
    }

    if (diff_result_init(out, 64) != 0) {
        fclose(fa); fclose(fb);
        return -2;
    }

    /* 获取文件大小 */
    fseek(fa, 0, SEEK_END);
    size_t len_a = (size_t)ftell(fa);
    fseek(fb, 0, SEEK_END);
    size_t len_b = (size_t)ftell(fb);

    /* 1. 流式计算公共前缀 */
    size_t prefix = stream_common_prefix(fa, fb, len_a, len_b);

    /* 2. 流式计算公共后缀 */
    size_t suffix = stream_common_suffix(fa, fb, len_a, len_b, prefix);

    /* 3. 中间差异区域 */
    size_t na = len_a - prefix - suffix;
    size_t nb = len_b - prefix - suffix;

    if (na > 0 || nb > 0) {
        greedy_diff_region_streaming(fa, prefix, na, fb, prefix, nb, (long)prefix, out);
    }

    fclose(fa);
    fclose(fb);
    return 0;
}
