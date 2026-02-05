#!/usr/bin/env python3
"""
Telnet Server 测试脚本
测试 Grbl_ESP32 的 Telnet 服务功能
"""

import sys
import socket
import time
import argparse

def test_telnet_service(ip: str, port: int = 8080) -> bool:
    """测试 Telnet 基本连接"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)

        print(f"连接到 {ip}:{port}...")
        sock.connect((ip, port))
        print("✓ Telnet 连接成功")

        sock.close()
        return True

    except socket.timeout:
        print("✗ 连接超时")
        return False
    except ConnectionRefusedError:
        print("✗ 连接被拒绝 (Telnet 服务可能未启用)")
        return False
    except Exception as e:
        print(f"✗ 连接失败: {e}")
        return False

def test_telnet_command(ip: str, port: int, cmd: str) -> bool:
    """测试 Telnet 命令执行"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)

        print(f"连接到 {ip}:{port}...")
        sock.connect((ip, port))

        # 发送命令
        print(f"发送命令: {cmd}")
        sock.sendall((cmd + '\n').encode('utf-8'))

        # 接收响应
        time.sleep(0.5)  # 等待响应
        response = sock.recv(4096).decode('utf-8', errors='ignore')

        print(f"✓ 收到响应:")
        print(response.strip())

        sock.close()
        return True

    except socket.timeout:
        print("✗ 响应超时")
        return False
    except Exception as e:
        print(f"✗ 命令执行失败: {e}")
        return False

def test_telnet_interactive(ip: str, port: int):
    """交互式 Telnet 模式"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1)  # 1 秒超时用于非阻塞接收

        print(f"连接到 {ip}:{port}...")
        sock.connect((ip, port))
        print("✓ 已连接 (输入 'exit' 退出)\n")

        while True:
            # 接收数据
            try:
                data = sock.recv(1024).decode('utf-8', errors='ignore')
                if data:
                    print(data, end='')
            except socket.timeout:
                pass

            # 发送命令
            try:
                cmd = input("> ")
                if cmd.lower() == 'exit':
                    break

                sock.sendall((cmd + '\n').encode('utf-8'))
                time.sleep(0.2)  # 等待响应

            except KeyboardInterrupt:
                print("\n中断")
                break

        sock.close()
        print("\n连接已关闭")

    except Exception as e:
        print(f"✗ 错误: {e}")

def main():
    parser = argparse.ArgumentParser(description='Telnet Server 测试工具')
    parser.add_argument('ip', help='ESP32 设备的 IP 地址')
    parser.add_argument('-p', '--port', type=int, default=8080, help='Telnet 端口 (默认: 8080)')
    parser.add_argument('-c', '--command', help='执行单条命令')
    parser.add_argument('-i', '--interactive', action='store_true', help='交互模式')

    args = parser.parse_args()

    print(f"测试 Telnet 服务: {args.ip}:{args.port}\n")

    if args.interactive:
        # 交互模式
        test_telnet_interactive(args.ip, args.port)
    elif args.command:
        # 单命令模式
        if not test_telnet_command(args.ip, args.port, args.command):
            sys.exit(1)
    else:
        # 基本连接测试
        if not test_telnet_service(args.ip, args.port):
            sys.exit(1)

        print("\n执行测试命令...")
        test_telnet_command(args.ip, args.port, "?")

    print("\n✓ Telnet 测试完成")

if __name__ == '__main__':
    main()
