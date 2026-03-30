// src/reader.c — 分块文件读取器
#include "fcmp.h"
#include <errno.h>
#include <string.h>

/* 打开文件并初始化读取器 */
int reader_open(ChunkReader *r, const char *path, size_t chunk_size) {
    memset(r, 0, sizeof(*r));
    r->fp = fopen(path, "rb");
    if (!r->fp) {
        return -1;
    }
    r->chunk_size = chunk_size;
    r->buffer = (uint8_t *)malloc(chunk_size);
    if (!r->buffer) {
        fclose(r->fp);
        return -2;
    }
    r->eof = false;
    r->bytes_read = 0;
    return 0;
}

/* 读取下一个分块，返回读取的字节数，0 表示 EOF */
int reader_read_chunk(ChunkReader *r) {
    if (r->eof) return 0;

    size_t n = fread(r->buffer, 1, r->chunk_size, r->fp);
    r->bytes_read = n;
    if (n < r->chunk_size) {
        r->eof = true;
    }
    return (int)n;
}

/* 关闭读取器并释放资源 */
void reader_close(ChunkReader *r) {
    if (r->fp) {
        fclose(r->fp);
        r->fp = NULL;
    }
    if (r->buffer) {
        free(r->buffer);
        r->buffer = NULL;
    }
    r->bytes_read = 0;
    r->eof = false;
}
