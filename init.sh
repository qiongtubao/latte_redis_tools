#!/bin/bash
# latte_redis_tools 项目初始化脚本
set -euo pipefail

echo "=== latte_redis_tools 初始化 ==="

# 创建目录结构
mkdir -p src include bin tests

# 创建 .gitignore
cat > .gitignore << 'EOF'
# 编译产物
bin/
*.o
*.d
*.out

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# 系统文件
.DS_Store
Thumbs.db
EOF

# 创建空源文件占位
touch src/main.c src/reader.c src/hash.c src/diff.c src/output.c src/mem_guard.c
touch include/fcmp.h

echo "=== 目录结构已创建 ==="
ls -la
