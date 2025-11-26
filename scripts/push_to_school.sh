#!/bin/bash

# 遇到错误立即停止
set -e

# 定义变量
MAIN_BRANCH="main"
SCHOOL_BRANCH="school"
SCHOOL_REMOTE="school"

# =================配置区域=================
# 在这里列出所有的 Submodule 路径，用空格隔开
SUBMODULE_PATHS=("vendor/fluf" "vendor/calico")
# =========================================

echo "=== 开始准备向学校服务器推送 ==="

# 1. 检查当前环境
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$CURRENT_BRANCH" != "$MAIN_BRANCH" ]; then
    echo "错误: 请先切换到 $MAIN_BRANCH 分支再运行此脚本。"
    exit 1
fi

if [ -n "$(git status --porcelain)" ]; then
    echo "错误: 工作区有未提交的更改，请先 commit 或 stash。"
    exit 1
fi

# 2. 确保本地拥有最新的代码和 submodule
echo "-> 更新所有 Submodules..."
git submodule update --init --recursive

# 3. 准备 School 分支
if git show-ref --verify --quiet refs/heads/$SCHOOL_BRANCH; then
    echo "-> 重置本地 $SCHOOL_BRANCH 分支..."
    git branch -D $SCHOOL_BRANCH
fi

git checkout -b $SCHOOL_BRANCH

echo "-> 正在将 Submodule 转换为普通文件 (Flattening)..."

# ================= 核心循环魔法 =================

# 遍历数组中的每一个 submodule
for path in "${SUBMODULE_PATHS[@]}"; do
    echo "   正在处理: $path ..."
    
    # A. 检查路径是否存在 (防止配错)
    if [ ! -d "$path" ]; then
        echo "警告: 路径 $path 不存在，跳过。"
        continue
    fi

    # B. 从 Git 索引中移除 submodule 记录（保留文件）
    # 注意：如果原本就没有 commit 这个 submodule，这一步会报错，所以前面加了 set -e 保护
    git rm --cached "$path"

    # C. 清理 submodule 内部的 .git 文件夹
    rm -rf "$path/.git"

    # D. 重新作为普通文件添加
    git add "$path"
done

# E. 删除 .gitmodules 文件（一次性删除）
if [ -f ".gitmodules" ]; then
    echo "   删除 .gitmodules 配置文件..."
    git rm .gitmodules
fi

# ===============================================

# 4. 提交变更
echo "-> 提交变更..."
git commit -m "release: flatten submodules (fluf, calico) for offline submission"

# 5. 强制推送到学校服务器
echo "-> 推送到学校远程仓库 ($SCHOOL_REMOTE)..."
git push -f $SCHOOL_REMOTE $SCHOOL_BRANCH:main

# 6. 清理现场
echo "-> 切换回 $MAIN_BRANCH 分支..."
git checkout $MAIN_BRANCH

# 7. 恢复 Submodule 状态
echo "-> 恢复 Submodule 链接..."
git submodule update --init --recursive

echo "=== 推送完成！ ==="
echo "School 分支现在包含了 fluf 和 calico 的完整源码。"
