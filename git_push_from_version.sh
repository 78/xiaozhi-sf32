#!/bin/bash

# 检查是否在Git仓库中
if [ ! -d .git ]; then
    echo "错误：当前目录不是Git仓库，请先初始化仓库或切换到仓库目录"
    exit 1
fi

# 从version.txt最后一行读取提交信息
version_file="version.txt"
default_msg="更新内容"

if [ -f "$version_file" ]; then
    # 读取文件最后一行内容并去除首尾空白
    commit_msg=$(tail -n 1 "$version_file" | xargs)
    # 如果最后一行内容为空，使用默认信息
    if [ -z "$commit_msg" ]; then
        commit_msg="$default_msg"
        echo "警告：$version_file 最后一行内容为空，将使用默认提交信息"
    else
        echo "已从$version_file最后一行读取提交信息：$commit_msg"
    fi
else
    # 文件不存在时使用默认信息
    commit_msg="$default_msg"
    echo "警告：未找到$version_file，将使用默认提交信息"
fi

# 添加所有变更文件
echo "正在添加文件..."
git add .
if [ $? -ne 0 ]; then
    echo "错误：添加文件失败"
    exit 1
fi

# 提交变更
echo "正在提交变更..."
git commit -m "$commit_msg"
if [ $? -ne 0 ]; then
    echo "错误：提交变更失败，可能没有需要提交的内容"
    exit 1
fi

# 推送到远程仓库的master分支（明确指定origin和master）
echo "正在推送到远程仓库 origin/master..."
git push origin master
if [ $? -ne 0 ]; then
    echo "错误：推送失败，请检查网络连接、远程仓库配置或分支名称是否正确"
    exit 1
fi

echo "操作完成：代码已成功上传到远程仓库 origin/master"
    