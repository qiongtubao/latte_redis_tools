#ifndef FCMP_H
#define FCMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* 默认分块大小 4KB */
#ifndef FILE_CHUNK_SIZE
#define FILE_CHUNK_SIZE 4096
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

/* ─── 分块读取器 ──────────────────────────────────────── */

typedef struct {
    FILE    *fp;
    size_t   chunk_size;
    uint8_t *buffer;
    size_t   bytes_read;
    bool     eof;
} ChunkReader;

int  reader_open(ChunkReader *r, const char *path, size_t chunk_size);
int  reader_read_chunk(ChunkReader *r);
void reader_close(ChunkReader *r);

/* ─── 哈希模块 ────────────────────────────────────────── */

uint32_t hash_block(const uint8_t *data, size_t len);

typedef struct {
    uint32_t *hashes;
    size_t    count;
    size_t    capacity;
} HashList;

int hash_file(const char *path, size_t chunk_size, HashList *out);
void hash_list_free(HashList *hl);

/* ─── 差异检测 ────────────────────────────────────────── */

typedef struct {
    DiffType  type;
    long      line_a;   /* 文件 A 中的行号（-1 表示无对应行） */
    long      line_b;   /* 文件 B 中的行号（-1 表示无对应行） */
    char     *content;  /* 差异内容 */
} DiffEntry;

typedef struct {
    DiffEntry *entries;
    size_t     count;
    size_t     capacity;
} DiffResult;

int  diff_files(const char *file_a, const char *file_b, size_t chunk_size, DiffResult *out);
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
