// src/output.c — 结果格式化输出（字符级）
#include "fcmp.h"
#include <stdio.h>
#include <string.h>

static const char *diff_type_str(DiffType t) {
    switch (t) {
        case DIFF_INSERT: return "INSERT";
        case DIFF_DELETE: return "DELETE";
        case DIFF_MODIFY: return "MODIFY";
        default:          return "NONE";
    }
}

/* 检查字符是否可打印 */
static int is_printable(unsigned char c) {
    return c >= 32 && c < 127;
}

/* 打印内容（自动检测二进制，用 \xNN 格式显示不可打印字符） */
static void print_content_escaped(const char *content) {
    if (!content) return;
    int has_binary = 0;
    /* 检测是否包含二进制 */
    for (const char *p = content; *p; p++) {
        if (!is_printable((unsigned char)*p) && *p != '\n' && *p != '\r' && *p != '\t') {
            has_binary = 1;
            break;
        }
    }

    if (has_binary) {
        /* 二进制模式：显示为 \xNN */
        for (const char *p = content; *p; p++) {
            unsigned char c = (unsigned char)*p;
            if (is_printable(c) || c == '\n' || c == '\r' || c == '\t') {
                putchar(c);
            } else {
                printf("\\x%02x", c);
            }
        }
    } else {
        /* 文本模式：直接输出 */
        fputs(content, stdout);
    }
}

/* 文本格式输出（字符级友好，支持二进制） */
static int print_text(const DiffResult *dr, const char *file_a, const char *file_b) {
    printf("=== 文件对比结果（字符级） ===\n");
    printf("文件 A: %s\n", file_a);
    printf("文件 B: %s\n", file_b);
    printf("差异总数: %zu\n\n", dr->count);

    if (dr->count == 0) {
        printf("✅ 两个文件内容完全相同\n");
        return 0;
    }

    size_t inserts = 0, deletes = 0;
    for (size_t i = 0; i < dr->count; i++) {
        if (dr->entries[i].type == DIFF_INSERT) inserts++;
        else if (dr->entries[i].type == DIFF_DELETE) deletes++;
    }

    printf("统计:\n");
    printf("  新增字符: %zu\n", inserts);
    printf("  删除字符: %zu\n\n", deletes);

    printf("详细差异:\n");
    for (size_t i = 0; i < dr->count; i++) {
        const DiffEntry *e = &dr->entries[i];
        if (e->type == DIFF_DELETE) {
            printf("  [-] 位置 %ld: \"", e->line_a);
            print_content_escaped(e->content);
            printf("\"\n");
        } else if (e->type == DIFF_INSERT) {
            printf("  [+] 位置 %ld: \"", e->line_b);
            print_content_escaped(e->content);
            printf("\"\n");
        } else {
            printf("  [~] A:%ld B:%ld: \"", e->line_a, e->line_b);
            print_content_escaped(e->content);
            printf("\"\n");
        }
    }

    return 0;
}

/* JSON 格式输出 */
static int print_json(const DiffResult *dr, const char *file_a, const char *file_b) {
    printf("{\n");
    printf("  \"file_a\": \"%s\",\n", file_a);
    printf("  \"file_b\": \"%s\",\n", file_b);
    printf("  \"total_diffs\": %zu,\n", dr->count);
    printf("  \"diffs\": [\n");

    for (size_t i = 0; i < dr->count; i++) {
        const DiffEntry *e = &dr->entries[i];
        printf("    {\"type\": \"%s\", \"pos\": %ld, \"content\": \"",
               diff_type_str(e->type),
               e->type == DIFF_DELETE ? e->line_a : e->line_b);

        /* 转义 JSON 特殊字符（含二进制） */
        if (e->content) {
            for (const char *p = e->content; *p; p++) {
                unsigned char c = (unsigned char)*p;
                switch (c) {
                    case '"':  printf("\\\""); break;
                    case '\\': printf("\\\\"); break;
                    case '\n': printf("\\n"); break;
                    case '\r': printf("\\r"); break;
                    case '\t': printf("\\t"); break;
                    default:
                    if (c >= 32 && c < 127) {
                        putchar(c);
                    } else {
                        printf("\\x%02x", c);
                    }
                }
            }
        }
        printf("\"}%s\n", (i < dr->count - 1) ? "," : "");
    }

    printf("  ]\n");
    printf("}\n");
    return 0;
}

/* 统一输出入口 */
int diff_print(const DiffResult *dr, const char *file_a, const char *file_b, OutputFormat fmt) {
    if (fmt == OUTPUT_JSON) {
        return print_json(dr, file_a, file_b);
    }
    return print_text(dr, file_a, file_b);
}
