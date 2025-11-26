#!/bin/bash

# 遇到错误立即停止
set -e

# 定义变量
MAIN_BRANCH="main"
SCHOOL_BRANCH="school"
SCHOOL_REMOTE="school"
SUBMODULE_PATH="vendor/fluf"

echo "=== 开始准备向学校服务器推送 ==="

# 1. 检查当前是否在主分支且工作区干净
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
echo "-> 更新 Submodules..."
git submodule update --init --recursive

# 3. 准备 School 分支
# 如果本地已经有 school 分支，先删除它，确保每次都是基于最新的 main 生成
if git show-ref --verify --quiet refs/heads/$SCHOOL_BRANCH; then
    echo "-> 重置本地 $SCHOOL_BRANCH 分支..."
    git branch -D $SCHOOL_BRANCH
fi

# 基于 main 创建新的 school 分支
git checkout -b $SCHOOL_BRANCH

echo "-> 正在将 Submodule 转换为普通文件 (Flattening)..."

# ================= 核心魔法操作 =================

# A. 从 Git 索引中移除 submodule 记录（但保留文件在磁盘上）
git rm --cached $SUBMODULE_PATH

# B. 删除 .gitmodules 文件（不再需要它了）
if [ -f ".gitmodules" ]; then
    git rm .gitmodules
fi

# C. 清理 submodule 内部的 .git 文件/文件夹
# 这一步至关重要，否则 Git 会再次把它识别为 submodule
rm -rf $SUBMODULE_PATH/.git

# D. 重新添加文件（这次作为普通文件添加）
git add $SUBMODULE_PATH

# ===============================================

# 4. 提交变更
echo "-> 提交变更..."
git commit -m "release: flatten submodules for offline school environment submission"

# 5. 强制推送到学校服务器
echo "-> 推送到学校远程仓库 ($SCHOOL_REMOTE)..."
# 注意：这里必须用 force，因为我们每次都是重写 school 分支的历史
git push -f $SCHOOL_REMOTE $SCHOOL_BRANCH:main

# 6. 清理现场，回到主分支
echo "-> 切换回 $MAIN_BRANCH 分支..."
git checkout $MAIN_BRANCH

# 7. 恢复 Submodule 状态（因为切分支可能会导致 submodule 文件夹空了）
git submodule update --init --recursive

echo "=== 推送完成！ ==="
echo "现在的状态："
echo "1. GitHub (origin/main): 保持 Submodule 指针，干净整洁。"
echo "2. School (school/main): 包含 fluf 的完整源码，无需联网即可编译。"
