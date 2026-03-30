#ifndef FCMP_H
#define FCMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* 行缓冲区大小 */
#ifndef LINE_BUF_SIZE
#define LINE_BUF_SIZE 65536
#endif

/* 默认内存上限 512MB */
#ifndef MEM_LIMIT_MB
#define MEM_LIMIT_MB 512
#endif

/* ─── 差异类型枚举 ─────────────────────────────────────── */

typedef enum {
    DIFF_NONE   = 0,   /* 无差异 */
    DIFF_INSERT = 1,   /* 新增内容 */
    DIFF_DELETE = 2,   /* 删除内容 */
    DIFF_MODIFY = 3    /* 覆盖/修改内容 */
} DiffType;

/* ─── 行数据结构 ────────────────────────────────────────── */

typedef struct {
    char   **lines;      /* 行内容数组 */
    size_t   count;      /* 行数 */
    size_t   capacity;   /* 容量 */
    size_t   data_len;   /* 实际数据长度（二进制安全，不依赖 strlen） */
} LineList;

int  linelist_load(const char *path, LineList *ll);
void linelist_free(LineList *ll);

/* ─── 差异检测 ────────────────────────────────────────── */

typedef struct {
    DiffType  type;
    long      line_a;   /* 文件 A 中的行号（-1 表示无对应行） */
    long      line_b;   /* 文件 B 中的行号（-1 表示无对应行） */
    char     *content;  /* 差异内容（B 文件中的行内容） */
} DiffEntry;

typedef struct {
    DiffEntry *entries;
    size_t     count;
    size_t     capacity;
} DiffResult;

int  diff_files(const char *file_a, const char *file_b, DiffResult *out);
void diff_result_free(DiffResult *dr);

/* ─── 输出模块 ────────────────────────────────────────── */

typedef enum {
    OUTPUT_TEXT = 0,
    OUTPUT_JSON = 1
} OutputFormat;

int diff_print(const DiffResult *dr, const char *file_a, const char *file_b, OutputFormat fmt);

/* ─── 内存保护 ────────────────────────────────────────── */

size_t get_current_memory_mb(void);
bool   check_memory_limit(size_t limit_mb);

#endif /* FCMP_H */
