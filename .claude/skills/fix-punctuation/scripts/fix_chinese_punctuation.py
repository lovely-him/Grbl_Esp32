#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复代码中的中文标点符号，替换为英文标点符号
"""

import re
import sys
import os
import glob

def fix_chinese_punctuation(content):
    """替换中文标点符号为英文标点符号"""
    replacements = {
        '，': ', ',
        '。': '. ',
        '；': '; ',
        '：': ': ',
        '！': '! ',
        '？': '? ',
        '、': ', ',
        '（': ' (',
        '）': ') ',
        '【': ' [',
        '】': '] ',
        '《': ' <',
        '》': '> ',
        '"': '"',
        '"': '"',
        ''': "'",
        ''': "'"
    }

    result = content
    changes = 0
    for cn_punct, en_punct in replacements.items():
        count = result.count(cn_punct)
        if count > 0:
            result = result.replace(cn_punct, en_punct)
            changes += count

    return result, changes

def is_code_file(file_path):
    """检查是否为代码文件"""
    code_extensions = {
        '.c', '.cpp', '.h', '.hpp', '.cc', '.cxx',
        '.py', '.js', '.ts', '.jsx', '.tsx',
        '.java', '.cs', '.go', '.rs',
        '.json', '.yaml', '.yml'
    }
    _, ext = os.path.splitext(file_path)
    return ext.lower() in code_extensions

def should_skip_file(file_path):
    """检查是否应该跳过该文件"""
    # 获取脚本自身的绝对路径
    script_path = os.path.abspath(__file__)
    target_path = os.path.abspath(file_path)

    # 跳过脚本自身
    if target_path == script_path:
        return True

    return False

def process_file(file_path):
    """处理单个文件"""
    # 跳过脚本自身
    if should_skip_file(file_path):
        print(f"[SKIP] Skipping script itself: {file_path}")
        return False

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        fixed_content, changes = fix_chinese_punctuation(content)

        if changes > 0:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(fixed_content)
            print(f"[SUCCESS] Fixed {changes} punctuation(s) in: {file_path}")
            return True
        else:
            print(f"[SKIP] No Chinese punctuation found in: {file_path}")
            return False

    except Exception as e:
        print(f"[ERROR] Error processing {file_path}: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_chinese_punctuation.py <file_path_or_pattern>")
        print("Examples:")
        print("  python fix_chinese_punctuation.py file.c")
        print("  python fix_chinese_punctuation.py 'src/**/*.cpp'")
        sys.exit(1)

    pattern = sys.argv[1]

    # 如果是具体文件路径
    if os.path.isfile(pattern):
        files = [pattern]
    # 如果包含通配符
    elif '*' in pattern or '?' in pattern:
        files = glob.glob(pattern, recursive=True)
        files = [f for f in files if os.path.isfile(f) and is_code_file(f)]
    # 如果是目录
    elif os.path.isdir(pattern):
        files = []
        for root, _, filenames in os.walk(pattern):
            for filename in filenames:
                file_path = os.path.join(root, filename)
                if is_code_file(file_path):
                    files.append(file_path)
    else:
        print(f"[ERROR] Invalid path or pattern: {pattern}")
        sys.exit(1)

    if not files:
        print(f"[WARNING] No files found matching: {pattern}")
        sys.exit(0)

    print(f"[INFO] Found {len(files)} file(s) to process")

    processed = 0
    for file_path in files:
        if process_file(file_path):
            processed += 1

    print(f"\n[SUMMARY] Processed {processed}/{len(files)} file(s)")

if __name__ == '__main__':
    main()
