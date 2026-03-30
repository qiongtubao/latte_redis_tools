# latte_redis_tools Makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
SRCDIR  = src
INCDIR  = include
BUILDDIR = bin
TESTDIR = tests

DEFINES = -DMEM_LIMIT_MB=512

# 只编译需要的源文件（移除 hash.c 和 reader.c）
SRCS    = $(filter-out $(SRCDIR)/hash.c $(SRCDIR)/reader.c, $(wildcard $(SRCDIR)/*.c))
OBJS    = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
TARGET  = $(BUILDDIR)/fcmp

.PHONY: all build clean test

all: build

# 编译主程序
build: $(TARGET)

$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 测试（依赖编译成功）
test: build
	@echo "=== Running tests ==="
	@if [ -d $(TESTDIR) ] && [ -n "$$(ls $(TESTDIR)/*_test.c 2>/dev/null)" ]; then \
		for t in $(TESTDIR)/*_test.c; do \
			echo "Compiling $$t..."; \
			$(CC) $(CFLAGS) $(DEFINES) -o $(BUILDDIR)/$$(basename $$t) $$t $(filter-out $(BUILDDIR)/main.o,$(OBJS)); \
			./$(BUILDDIR)/$$(basename $$t); \
		done; \
	else \
		echo "No test files found in $(TESTDIR)/"; \
	fi
	@echo "=== Tests done ==="

# 清理
clean:
	rm -rf $(BUILDDIR)
