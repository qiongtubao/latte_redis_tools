// src/main.c — CLI 入口
#include "fcmp.h"
#include <string.h>
#include <getopt.h>

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options] <file_a> <file_b>\n"
        "\n"
        "Options:\n"
        "  -o, --output <format>      输出格式: text | json，默认 text\n"
        "  -h, --help                 显示帮助\n"
        "\n"
        "Example:\n"
        "  %s file1.txt file2.txt\n"
        "  %s -o json file1.txt file2.txt\n",
        prog, prog, prog);
}

int main(int argc, char *argv[]) {
    OutputFormat fmt = OUTPUT_TEXT;

    static struct option long_opts[] = {
        { "output",     required_argument, 0, 'o' },
        { "help",       no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:h", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'o':
                if (strcmp(optarg, "json") == 0) fmt = OUTPUT_JSON;
                break;
            case 'h': usage(argv[0]); return 0;
            default:  usage(argv[0]); return 1;
        }
    }

    if (argc - optind < 2) {
        fprintf(stderr, "Error: 需要两个文件参数\n\n");
        usage(argv[0]);
        return 1;
    }

    const char *file_a = argv[optind];
    const char *file_b = argv[optind + 1];

    /* 内存保护检查 */
    if (!check_memory_limit(MEM_LIMIT_MB)) {
        fprintf(stderr, "Error: 可用内存不足 %dMB\n", MEM_LIMIT_MB);
        return 1;
    }

    /* 执行差异对比（逐行模式） */
    DiffResult dr = {0};
    int ret = diff_files(file_a, file_b, &dr);
    if (ret != 0) {
        fprintf(stderr, "Error: 文件对比失败 (code=%d)\n", ret);
        return 1;
    }

    /* 输出结果 */
    diff_print(&dr, file_a, file_b, fmt);

    diff_result_free(&dr);
    return 0;
}
