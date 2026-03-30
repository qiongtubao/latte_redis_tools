// src/output.c — 结果格式化输出
#include "fcmp.h"
#include <stdio.h>

static const char *diff_type_str(DiffType t) {
    switch (t) {
        case DIFF_INSERT: return "INSERT";
        case DIFF_DELETE: return "DELETE";
        case DIFF_MODIFY: return "MODIFY";
        default:          return "NONE";
    }
}

/* 文本格式输出 */
static int print_text(const DiffResult *dr, const char *file_a, const char *file_b) {
    printf("=== 文件对比结果 ===\n");
    printf("文件 A: %s\n", file_a);
    printf("文件 B: %s\n", file_b);
    printf("差异总数: %zu\n\n", dr->count);

    if (dr->count == 0) {
        printf("✅ 两个文件内容完全相同\n");
        return 0;
    }

    size_t inserts = 0, deletes = 0, modifies = 0;
    for (size_t i = 0; i < dr->count; i++) {
        switch (dr->entries[i].type) {
            case DIFF_INSERT: inserts++; break;
            case DIFF_DELETE: deletes++; break;
            case DIFF_MODIFY: modifies++; break;
            default: break;
        }
    }

    printf("统计:\n");
    printf("  新增 (INSERT): %zu\n", inserts);
    printf("  删除 (DELETE): %zu\n", deletes);
    printf("  修改 (MODIFY): %zu\n\n", modifies);

    printf("详细差异:\n");
    for (size_t i = 0; i < dr->count; i++) {
        const DiffEntry *e = &dr->entries[i];
        printf("  [%s] A:%ld B:%ld %s\n",
               diff_type_str(e->type), e->line_a, e->line_b,
               e->content ? e->content : "");
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
        printf("    {\"type\": \"%s\", \"line_a\": %ld, \"line_b\": %ld, \"content\": \"%s\"}%s\n",
               diff_type_str(e->type), e->line_a, e->line_b,
               e->content ? e->content : "",
               (i < dr->count - 1) ? "," : "");
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
