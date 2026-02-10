#!/usr/bin/env python3
"""
mDNS Service Provider - 注册一个测试服务
需要安装: pip install zeroconf
"""

import argparse
import socket
import time
from zeroconf import ServiceInfo, Zeroconf, InterfaceChoice


def get_local_ip():
    """获取本机 IP 地址"""
    try:
        # 创建一个 UDP socket 连接到外部地址 (不会真正发送数据)
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


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


def main():
    parser = argparse.ArgumentParser(
        description='mDNS 服务提供者 - 注册一个测试服务',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                                    # 使用默认参数
  %(prog)s --name MyDevice --port 8080       # 自定义服务名和端口
  %(prog)s --hostname mydevice --type _http._tcp.local.  # 自定义主机名和服务类型
        """
    )

    parser.add_argument(
        '--name',
        default='TestDevice',
        help='服务名称 (默认: TestDevice)'
    )

    parser.add_argument(
        '--hostname',
        default='testdevice',
        help='主机名 (不含 .local 后缀) (默认: testdevice)'
    )

    parser.add_argument(
        '--port',
        type=int,
        default=8080,
        help='服务端口 (默认: 8080)'
    )

    parser.add_argument(
        '--type',
        dest='service_type',
        default='_http._tcp.local.',
        help='服务类型 (默认: _http._tcp.local.)'
    )

    parser.add_argument(
        '--ip',
        help='指定 IP 地址 (默认自动检测) '
    )

    args = parser.parse_args()

    print("=" * 60)
    print("mDNS Service Provider - 服务提供者")
    print("=" * 60)

    # 获取本机 IP
    local_ip = args.ip if args.ip else get_local_ip()
    print(f"\n[INFO] 本机 IP: {local_ip}")

    # 显示所有本地 IP
    all_ips = get_all_local_ips()
    if all_ips:
        print(f"[INFO] 监听所有网络接口: {', '.join(all_ips)}")
    else:
        print(f"[INFO] 监听所有网络接口")

    # 创建 Zeroconf 实例 (监听所有网络接口)
    zeroconf = Zeroconf(interfaces=InterfaceChoice.All)

    # 构建服务名称
    service_name = f"{args.name}.{args.service_type}"
    hostname = f"{args.hostname}.local."

    # 创建服务信息
    info = ServiceInfo(
        args.service_type,
        service_name,
        addresses=[socket.inet_aton(local_ip)],
        port=args.port,
        properties={
            b'version': b'1.0',
            b'device': args.name.encode('utf-8'),
            b'description': b'mDNS Test Service'
        },
        server=hostname
    )

    print(f"\n[INFO] 注册 mDNS 服务:")
    print(f"  - 服务类型: {args.service_type}")
    print(f"  - 服务名称: {service_name}")
    print(f"  - 主机名: {hostname}")
    print(f"  - IP 地址: {local_ip}")
    print(f"  - 端口: {args.port}")
    print(f"  - 属性: {info.properties}")

    try:
        # 注册服务 (允许自动重命名以避免冲突)
        print("\n[INFO] 正在注册服务...")
        zeroconf.register_service(info, allow_name_change=True)

        # 检查是否被重命名
        actual_name = info.name
        if actual_name != service_name:
            print(f"[WARNING] 服务名称已被自动重命名: {actual_name}")
            print(f"[WARNING] 原因: 网络上已存在 {service_name}")

        print("[SUCCESS] 服务注册成功! ")

        print("\n[INFO] 服务正在运行, 按 Ctrl+C 停止...")
        print("[INFO] 你现在可以运行 test_mdns_browser.py 来查询此服务")

        # 保持运行
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n\n[INFO] 正在注销服务...")
    finally:
        zeroconf.unregister_service(info)
        zeroconf.close()
        print("[INFO] 服务已停止")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        import traceback
        print(f"\n[ERROR] 发生错误: {type(e).__name__}: {e}")

        # 针对常见错误提供解决方案
        error_type = type(e).__name__
        if error_type == "NonUniqueNameException":
            print("\n[原因] 网络上已存在同名的 mDNS 服务")
            print("[解决方案]")
            print("  1. 使用不同的主机名: --hostname grblesp-4")
            print("  2. 或者关闭网络上的同名设备")
            print("  3. 脚本已自动启用重命名功能, 请重新运行")
        elif error_type in ["PermissionError", "OSError"]:
            print("\n[原因] 权限不足或网络接口问题")
            print("[解决方案]")
            print("  1. 以管理员权限运行脚本")
            print("  2. 检查防火墙是否阻止 UDP 端口 5353")
            print("  3. 检查网络接口是否正常")

        print("\n[TRACEBACK] 详细错误信息:")
        traceback.print_exc()
        print("\n[HINT] 请确保已安装 zeroconf 库:")
        print("       pip install zeroconf")
