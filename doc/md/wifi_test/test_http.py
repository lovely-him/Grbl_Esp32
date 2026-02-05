#!/usr/bin/env python3
"""
HTTP Web Server 测试脚本
测试 Grbl_ESP32 的 HTTP 服务功能
"""

import sys
import argparse
from urllib.request import urlopen, Request
from urllib.error import URLError, HTTPError
from urllib.parse import urlencode

def test_http_service(ip: str, port: int = 80) -> bool:
    """测试 HTTP 基本连接"""
    try:
        url = f"http://{ip}:{port}/"
        req = Request(url, headers={'User-Agent': 'Grbl_ESP32_Tester/1.0'})

        with urlopen(req, timeout=5) as response:
            status = response.getcode()
            data = response.read()

            print(f"✓ HTTP 服务正常")
            print(f"  状态码: {status}")
            print(f"  响应大小: {len(data)} 字节")
            return True

    except HTTPError as e:
        print(f"✗ HTTP 错误: {e.code} {e.reason}")
        return False
    except URLError as e:
        print(f"✗ URL 错误: {e.reason}")
        return False
    except Exception as e:
        print(f"✗ 未知错误: {e}")
        return False

def test_http_command(ip: str, port: int, cmd: str) -> bool:
    """测试 HTTP 命令执行"""
    try:
        params = urlencode({'cmd': cmd})
        url = f"http://{ip}:{port}/command?{params}"
        req = Request(url, headers={'User-Agent': 'Grbl_ESP32_Tester/1.0'})

        with urlopen(req, timeout=5) as response:
            data = response.read().decode('utf-8')

            print(f"✓ 命令执行成功: {cmd}")
            print(f"  响应: {data.strip()}")
            return True

    except Exception as e:
        print(f"✗ 命令执行失败: {e}")
        return False

def test_http_files(ip: str, port: int) -> bool:
    """测试文件列表接口"""
    try:
        url = f"http://{ip}:{port}/files"
        req = Request(url, headers={'User-Agent': 'Grbl_ESP32_Tester/1.0'})

        with urlopen(req, timeout=5) as response:
            data = response.read().decode('utf-8')

            print(f"✓ 文件接口正常")
            print(f"  响应大小: {len(data)} 字节")
            return True

    except Exception as e:
        print(f"✗ 文件接口测试失败: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='HTTP Web Server 测试工具')
    parser.add_argument('ip', help='ESP32 设备的 IP 地址')
    parser.add_argument('-p', '--port', type=int, default=80, help='HTTP 端口 (默认: 80)')
    parser.add_argument('-c', '--command', help='执行 G-code 命令')
    parser.add_argument('--test-files', action='store_true', help='测试文件接口')

    args = parser.parse_args()

    print(f"测试 HTTP 服务: {args.ip}:{args.port}\n")

    # 基本连接测试
    if not test_http_service(args.ip, args.port):
        sys.exit(1)

    print()

    # 命令执行测试
    if args.command:
        test_http_command(args.ip, args.port, args.command)
        print()

    # 文件接口测试
    if args.test_files:
        test_http_files(args.ip, args.port)
        print()

    print("✓ HTTP 测试完成")

if __name__ == '__main__':
    main()
