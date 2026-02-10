#!/usr/bin/env python3
"""
mDNS Service Browser - 查询和发现服务
需要安装: pip install zeroconf
"""

import argparse
import socket
import time
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf, InterfaceChoice


def get_all_local_ips():
    """获取所有本地 IP 地址"""
    ips = []
    try:
        hostname = socket.gethostname()
        # 获取所有地址信息
        for info in socket.getaddrinfo(hostname, None):
            ip = info[4][0]
            # 过滤掉 IPv6 和 127.0.0.1
            if ':' not in ip and ip != '127.0.0.1':
                if ip not in ips:
                    ips.append(ip)
    except Exception as e:
        pass
    return ips


class MyListener(ServiceListener):
    """服务监听器"""

    def __init__(self):
        self.services_found = []

    def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        if info:
            print(f"\n[UPDATE] 服务更新: {name}")
            print(f"  - 类型: {type_}")
            print(f"  - 主机名: {info.server}")

            # 解析 IP 地址
            addresses = [socket.inet_ntoa(addr) for addr in info.addresses]
            print(f"  - IP 地址: {', '.join(addresses)}")

            print(f"  - 端口: {info.port}")

            # 解析属性
            if info.properties:
                print(f"  - 属性:")
                for key, value in info.properties.items():
                    try:
                        decoded_value = value.decode('utf-8')
                    except:
                        decoded_value = str(value)
                    print(f"      {key.decode('utf-8')}: {decoded_value}")
        else:
            print(f"\n[UPDATE] 服务更新: {name} (无法获取详细信息)")

    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"\n[REMOVE] 服务移除: {name}")

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        if info:
            print(f"\n[FOUND] 发现服务: {name}")
            print(f"  - 类型: {type_}")
            print(f"  - 主机名: {info.server}")

            # 解析 IP 地址
            addresses = [socket.inet_ntoa(addr) for addr in info.addresses]
            print(f"  - IP 地址: {', '.join(addresses)}")

            print(f"  - 端口: {info.port}")

            # 解析属性
            if info.properties:
                print(f"  - 属性:")
                for key, value in info.properties.items():
                    try:
                        decoded_value = value.decode('utf-8')
                    except:
                        decoded_value = str(value)
                    print(f"      {key.decode('utf-8')}: {decoded_value}")

            self.services_found.append(name)
        else:
            print(f"\n[FOUND] 发现服务: {name} (无法获取详细信息)")


def query_host(hostname, timeout=5):
    """查询主机名对应的 IP 地址"""
    print(f"\n[QUERY] 查询主机名: {hostname}")

    # 确保主机名以 .local 结尾
    if not hostname.endswith('.local'):
        hostname = hostname + '.local'

    print(f"[INFO] 正在解析 {hostname}...")

    # 显示所有本地 IP
    local_ips = get_all_local_ips()
    if local_ips:
        print(f"[INFO] 本地 IP: {', '.join(local_ips)}")

    try:
        # 使用 socket.getaddrinfo 解析 .local 域名
        # 这依赖于系统的 mDNS 解析器 (Windows 的 DNS Client 服务) 
        result = socket.getaddrinfo(hostname, None, socket.AF_INET)

        if result:
            ips = [info[4][0] for info in result]
            print(f"\n[SUCCESS] 找到主机: {hostname}")
            print(f"  - IP 地址: {', '.join(ips)}")
            return ips
        else:
            print(f"\n[FAILED] 未找到主机: {hostname}")
            return []

    except socket.gaierror as e:
        print(f"\n[FAILED] 无法解析主机名: {hostname}")
        print(f"  - 错误: {e}")
        print(f"\n[HINT] 可能的原因:")
        print(f"  1. 主机不存在或未运行")
        print(f"  2. 主机未注册 mDNS 服务")
        print(f"  3. 不在同一局域网内")
        print(f"  4. DNS Client 服务未运行 (Windows) ")
        return []
    except Exception as e:
        print(f"\n[ERROR] 查询失败: {e}")
        return []


def browse_services(service_type, duration=0):
    """浏览指定类型的服务"""
    print(f"\n[BROWSE] 浏览服务类型: {service_type}")
    if duration == 0:
        print(f"[INFO] 监听模式: 持续监听 (按 Ctrl+C 停止) ")
    else:
        print(f"[INFO] 监听时长: {duration} 秒")

    # 显示所有本地 IP
    local_ips = get_all_local_ips()
    if local_ips:
        print(f"[INFO] 监听所有网络接口: {', '.join(local_ips)}")
    else:
        print(f"[INFO] 监听所有网络接口")

    zeroconf = Zeroconf(interfaces=InterfaceChoice.All)
    listener = MyListener()

    try:
        # 创建服务浏览器
        browser = ServiceBrowser(zeroconf, service_type, listener)

        print(f"\n[INFO] 正在监听 {service_type} 服务...")
        print("[INFO] 等待服务广播...")

        if duration == 0:
            # 持续监听模式
            print("[INFO] 持续监听中, 按 Ctrl+C 停止...")
            while True:
                time.sleep(1)
        else:
            # 定时监听模式
            time.sleep(duration)

        # 显示结果 (只在定时模式下显示) 
        print(f"\n{'=' * 60}")
        print(f"[RESULT] 共发现 {len(listener.services_found)} 个服务")
        if listener.services_found:
            print("[RESULT] 服务列表:")
            for service in listener.services_found:
                print(f"  - {service}")
        else:
            print("[RESULT] 未发现任何服务")
        print(f"{'=' * 60}")

        return len(listener.services_found)

    except KeyboardInterrupt:
        # 用户中断, 显示结果
        print(f"\n\n{'=' * 60}")
        print(f"[RESULT] 监听已停止, 共发现 {len(listener.services_found)} 个服务")
        if listener.services_found:
            print("[RESULT] 服务列表:")
            for service in listener.services_found:
                print(f"  - {service}")
        else:
            print("[RESULT] 未发现任何服务")
        print(f"{'=' * 60}")
        return len(listener.services_found)
    finally:
        zeroconf.close()


def main():
    parser = argparse.ArgumentParser(
        description='mDNS 服务消费者 - 查询和发现服务/主机',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  # 服务发现模式
  %(prog)s                                    # 持续监听 HTTP 服务 (默认) 
  %(prog)s --duration 10                     # 监听 10 秒后停止
  %(prog)s --type _printer._tcp.local.       # 持续监听打印机服务

  # 主机查询模式
  %(prog)s --hostname testdevice             # 查询主机名 testdevice.local
  %(prog)s --hostname grbl-esp32             # 查询 ESP32 设备
        """
    )

    parser.add_argument(
        '--type',
        dest='service_type',
        default='_http._tcp.local.',
        help='服务类型 (默认: _http._tcp.local.)'
    )

    parser.add_argument(
        '--duration',
        type=int,
        default=0,
        help='监听时长 (秒) (0 = 持续监听, 默认: 0)'
    )

    parser.add_argument(
        '--hostname',
        help='查询主机名 (不含 .local 后缀) '
    )

    args = parser.parse_args()

    print("=" * 60)
    print("mDNS Service Browser - 服务消费者")
    print("=" * 60)

    # 如果指定了主机名, 执行主机查询
    if args.hostname:
        ips = query_host(args.hostname)
        if not ips:
            exit(1)
    else:
        # 否则执行服务发现
        count = browse_services(args.service_type, args.duration)

        # 只在定时模式且未发现服务时显示提示
        if count == 0 and args.duration > 0:
            print("\n[HINT] 未发现服务, 可能的原因:")
            print("  1. 服务提供者未运行 (运行 test_mdns_provider.py)")
            print("  2. 防火墙阻止了 UDP 5353 端口")
            print("  3. 不在同一局域网内")
            print("  4. 网络不支持组播 (224.0.0.251)")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n[INFO] 用户中断")
    except Exception as e:
        print(f"\n[ERROR] 发生错误: {e}")
        print("\n[HINT] 请确保已安装 zeroconf 库:")
        print("       pip install zeroconf")
