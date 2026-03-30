// src/mem_guard.c — 内存保护机制
#include "fcmp.h"

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif

/* 获取当前进程内存占用（MB） */
size_t get_current_memory_mb(void) {
#ifdef __APPLE__
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) != KERN_SUCCESS) {
        return 0;
    }
    return (size_t)(info.resident_size / (1024 * 1024));
#elif defined(__linux__)
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return 0;
    }
    /* Linux 下使用 /proc/self/status 更精确，这里简化处理 */
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[256];
    size_t rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "VmRSS: %zu", &rss) == 1) {
            rss /= 1024; /* KB -> MB */
            break;
        }
    }
    fclose(f);
    return rss;
#else
    return 0;
#endif
}

/* 检查是否在内存限制范围内 */
bool check_memory_limit(size_t limit_mb) {
    size_t current = get_current_memory_mb();
    /* 如果无法获取内存信息，默认允许执行 */
    if (current == 0) return true;
    return current < limit_mb;
}
