#!/usr/bin/env python3
"""
mDNS 服务测试脚本
测试 Grbl_ESP32 的 mDNS (.local 域名) 服务功能
"""

import sys
import socket
import argparse

def test_mdns_service(hostname: str = "grblesp.local") -> bool:
    """测试 mDNS 域名解析"""
    try:
        print(f"解析 mDNS 主机名: {hostname}...")

        # 尝试解析 .local 域名
        ip = socket.gethostbyname(hostname)
        print(f"✓ mDNS 解析成功: {hostname} -> {ip}")

        # 验证 IP 地址格式
        parts = ip.split('.')
        if len(parts) == 4 and all(0 <= int(p) <= 255 for p in parts):
            print(f"✓ IP 地址格式正确: {ip}")

            # 尝试 ping 验证
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                sock.connect((ip, 80))  # 尝试连接 HTTP 端口
                sock.close()
                print(f"✓ 主机可访问 (HTTP 端口开放)")
                return True
            except Exception as e:
                print(f"⚠ 主机解析成功但无法连接: {e}")
                return True  # 解析成功就算通过
        else:
            print(f"✗ IP 地址格式错误: {ip}")
            return False

    except socket.gaierror as e:
        print(f"✗ mDNS 解析失败: {e}")
        print("可能原因:")
        print("  1. 设备未启动或未连接到网络")
        print("  2. 设备在 AP 模式 (mDNS 仅 STA 模式)")
        print("  3. 本地网络不支持 mDNS")
        print("  4. 主机名不正确")
        return False
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='测试 Grbl_ESP32 mDNS 服务')
    parser.add_argument('--hostname', '-n', default='grblesp.local',
                        help='mDNS 主机名 (默认: grblesp.local)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='详细输出')

    args = parser.parse_args()

    print("=" * 60)
    print("Grbl_ESP32 mDNS 服务测试")
    print("=" * 60)
    print()

    success = test_mdns_service(args.hostname)

    print()
    print("=" * 60)
    if success:
        print("✓ mDNS 测试通过")
        print("=" * 60)
        return 0
    else:
        print("✗ mDNS 测试失败")
        print("=" * 60)
        return 1

if __name__ == '__main__':
    sys.exit(main())
