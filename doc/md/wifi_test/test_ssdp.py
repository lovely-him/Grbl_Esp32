#!/usr/bin/env python3
"""
SSDP/UPnP 服务测试脚本
测试 Grbl_ESP32 的 SSDP 设备发现功能
"""

import sys
import socket
import argparse
from urllib.request import urlopen

def test_ssdp_discovery(timeout: int = 5) -> bool:
    """测试 SSDP 设备发现"""
    try:
        # SSDP M-SEARCH 请求
        msg = \
            'M-SEARCH * HTTP/1.1\r\n' \
            'HOST:239.255.255.250:1900\r\n' \
            'ST:upnp:rootdevice\r\n' \
            'MX:2\r\n' \
            'MAN:"ssdp:discover"\r\n' \
            '\r\n'

        # 创建 UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.settimeout(timeout)

        print(f"发送 SSDP M-SEARCH 请求...")
        sock.sendto(msg.encode(), ('239.255.255.250', 1900))

        devices = []
        try:
            while True:
                data, addr = sock.recvfrom(1024)
                response = data.decode('utf-8', errors='ignore')

                # 检查是否是 Grbl_ESP32 设备
                if 'grblesp' in response.lower() or 'esp32' in response.lower():
                    print(f"\n✓ 发现 Grbl_ESP32 设备: {addr[0]}")
                    devices.append((addr[0], response))

                    # 解析响应
                    lines = response.split('\r\n')
                    for line in lines:
                        if line.startswith('LOCATION:'):
                            location = line.split(':', 1)[1].strip()
                            print(f"  位置: {location}")
                        elif line.startswith('USN:'):
                            usn = line.split(':', 1)[1].strip()
                            print(f"  USN: {usn}")

        except socket.timeout:
            pass

        sock.close()

        if devices:
            print(f"\n✓ 共发现 {len(devices)} 个设备")
            return True
        else:
            print("\n✗ 未发现任何设备")
            print("可能原因:")
            print("  1. 设备未启动或未连接到网络")
            print("  2. 设备在 AP 模式 (SSDP 仅 STA 模式)")
            print("  3. 防火墙阻止 UDP 1900 端口")
            print("  4. 网络不支持多播")
            return False

    except Exception as e:
        print(f"✗ SSDP 发现失败: {e}")
        return False

def test_ssdp_description(ip: str, port: int = 80) -> bool:
    """测试 SSDP 设备描述 XML"""
    try:
        url = f"http://{ip}:{port}/description.xml"
        print(f"\n获取设备描述: {url}")

        with urlopen(url, timeout=5) as response:
            data = response.read().decode('utf-8')

            if '<root' in data and '<device>' in data:
                print("✓ 设备描述 XML 正常")

                # 提取关键信息
                if '<friendlyName>' in data:
                    start = data.find('<friendlyName>') + 14
                    end = data.find('</friendlyName>')
                    name = data[start:end]
                    print(f"  设备名称: {name}")

                if '<modelName>' in data:
                    start = data.find('<modelName>') + 11
                    end = data.find('</modelName>')
                    model = data[start:end]
                    print(f"  型号: {model}")

                return True
            else:
                print("✗ 设备描述格式错误")
                return False

    except Exception as e:
        print(f"✗ 获取设备描述失败: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='测试 Grbl_ESP32 SSDP/UPnP 服务')
    parser.add_argument('--ip', help='设备 IP 地址 (用于测试描述 XML)')
    parser.add_argument('--timeout', type=int, default=5, help='发现超时时间 (秒)')

    args = parser.parse_args()

    print("=" * 60)
    print("Grbl_ESP32 SSDP/UPnP 服务测试")
    print("=" * 60)
    print()

    # SSDP 发现测试
    success = test_ssdp_discovery(args.timeout)

    # 如果提供了 IP, 测试描述 XML
    if args.ip:
        success = test_ssdp_description(args.ip) and success

    print()
    print("=" * 60)
    if success:
        print("✓ SSDP 测试通过")
    else:
        print("✗ SSDP 测试失败")
    print("=" * 60)

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
