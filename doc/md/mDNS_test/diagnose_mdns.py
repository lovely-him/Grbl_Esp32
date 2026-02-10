#!/usr/bin/env python3
"""
mDNS 网络诊断工具
检查 mDNS 服务发现问题的可能原因
"""

import socket
import struct

def print_section(title):
    """打印分节标题"""
    print(f"\n{'=' * 60}")
    print(f"{title}")
    print(f"{'=' * 60}")

def get_all_interfaces():
    """获取所有网络接口信息"""
    interfaces = []
    try:
        hostname = socket.gethostname()
        for info in socket.getaddrinfo(hostname, None):
            ip = info[4][0]
            if ':' not in ip and ip != '127.0.0.1':
                if ip not in [iface['ip'] for iface in interfaces]:
                    interfaces.append({'ip': ip, 'family': info[0]})
    except Exception as e:
        print(f"[ERROR] 获取网络接口失败: {e}")
    return interfaces

def check_multicast_support(interface_ip):
    """检查接口是否支持组播"""
    print(f"\n[CHECK] 检查接口 {interface_ip} 的组播支持...")

    try:
        # 创建 UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # 尝试绑定到 mDNS 端口
        try:
            sock.bind(('', 5353))
            print(f"  ✓ 可以绑定到 UDP 5353 端口")
        except OSError as e:
            print(f"  ✗ 无法绑定到 UDP 5353 端口: {e}")
            sock.close()
            return False

        # 尝试加入 mDNS 组播组
        mcast_group = '224.0.0.251'
        try:
            mreq = struct.pack('4s4s', socket.inet_aton(mcast_group), socket.inet_aton(interface_ip))
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            print(f"  ✓ 可以加入组播组 {mcast_group}")
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_DROP_MEMBERSHIP, mreq)
        except OSError as e:
            print(f"  ✗ 无法加入组播组: {e}")
            sock.close()
            return False

        sock.close()
        return True

    except Exception as e:
        print(f"  ✗ 检查失败: {e}")
        return False

def test_mdns_query(interface_ip):
    """测试 mDNS 查询"""
    print(f"\n[TEST] 从接口 {interface_ip} 发送 mDNS 查询...")

    try:
        # 创建 UDP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # 绑定到指定接口
        sock.bind((interface_ip, 0))

        # 设置组播 TTL
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 255)

        # 发送 mDNS 查询 (简化版, 仅用于测试)
        mcast_group = ('224.0.0.251', 5353)
        message = b'\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00'  # 简化的 DNS 查询头

        sock.sendto(message, mcast_group)
        print(f"  ✓ 成功发送组播查询到 {mcast_group}")

        sock.close()
        return True

    except Exception as e:
        print(f"  ✗ 发送失败: {e}")
        return False

def main():
    """主函数"""
    print("=" * 60)
    print("mDNS 网络诊断工具")
    print("=" * 60)

    # 获取所有网络接口
    print_section("网络接口信息")
    interfaces = get_all_interfaces()

    if not interfaces:
        print("[ERROR] 未找到网络接口")
        return

    print(f"[INFO] 找到 {len(interfaces)} 个网络接口:")
    for iface in interfaces:
        print(f"  - {iface['ip']}")

    # 检查每个接口的组播支持
    print_section("组播支持检查")
    for iface in interfaces:
        ip = iface['ip']
        is_hotspot = ip.startswith('192.168.137.')

        if is_hotspot:
            print(f"\n[WARNING] 接口 {ip} 是 Windows 移动热点")

        supported = check_multicast_support(ip)

        if supported:
            test_mdns_query(ip)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[INFO] 用户中断")
    except Exception as e:
        print(f"\n[ERROR] 发生错误: {e}")
        import traceback
        traceback.print_exc()

