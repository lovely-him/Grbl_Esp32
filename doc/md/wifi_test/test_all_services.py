#!/usr/bin/env python3
"""
Grbl_ESP32 网络服务完整测试脚本
测试所有网络服务的基本连接和功能
"""

import sys
import time
import socket
import argparse
from typing import Dict, Tuple
from urllib.request import urlopen, Request
from urllib.error import URLError

# 导入子模块 (如果存在) 
try:
    from test_http import test_http_service
    from test_telnet import test_telnet_service
    from test_websocket import test_websocket_service
    from test_file_transfer import test_file_transfer_service
    from test_mdns import test_mdns_service
except ImportError:
    # 如果子模块不存在, 定义简单的测试函数
    pass

# ANSI 颜色代码
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_header(text: str):
    """打印标题"""
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'=' * 60}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{text:^60}{Colors.RESET}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'=' * 60}{Colors.RESET}\n")

def print_test(service: str, status: str):
    """打印测试结果"""
    if status == "✓":
        color = Colors.GREEN
        status_text = "✓ 通过"
    elif status == "✗":
        color = Colors.RED
        status_text = "✗ 失败"
    else:
        color = Colors.YELLOW
        status_text = "⊘ 跳过"

    print(f"{service:.<40} {color}{status_text}{Colors.RESET}")

def test_tcp_port(ip: str, port: int, timeout: float = 2.0) -> Tuple[bool, str]:
    """测试 TCP 端口是否开放"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex((ip, port))
        sock.close()

        if result == 0:
            return True, "端口开放"
        else:
            return False, f"端口关闭 (错误代码: {result})"
    except socket.timeout:
        return False, "连接超时"
    except socket.error as e:
        return False, f"Socket 错误: {e}"
    except Exception as e:
        return False, f"未知错误: {e}"

def test_udp_port(ip: str, port: int, timeout: float = 2.0) -> Tuple[bool, str]:
    """测试 UDP 端口 (发送测试包)"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)

        # 发送测试数据
        sock.sendto(b"TEST", (ip, port))

        # 尝试接收响应 (可能没有响应) 
        try:
            data, addr = sock.recvfrom(1024)
            sock.close()
            return True, f"收到响应 ({len(data)} 字节)"
        except socket.timeout:
            # UDP 端口可能开放但无响应
            sock.close()
            return True, "已发送测试包 (无响应属正常)"

    except Exception as e:
        return False, f"错误: {e}"

def test_http(ip: str, port: int = 80) -> Tuple[bool, str]:
    """测试 HTTP 服务"""
    try:
        url = f"http://{ip}:{port}/"
        req = Request(url, headers={'User-Agent': 'Grbl_ESP32_Tester/1.0'})

        with urlopen(req, timeout=5) as response:
            status = response.getcode()
            if status == 200:
                return True, f"HTTP {status} OK"
            else:
                return False, f"HTTP {status}"
    except URLError as e:
        return False, f"URL 错误: {e.reason}"
    except Exception as e:
        return False, f"错误: {e}"

def test_telnet(ip: str, port: int = 8080) -> Tuple[bool, str]:
    """测试 Telnet 服务"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((ip, port))

        # 发送测试命令
        sock.sendall(b"?\n")

        # 接收响应
        data = sock.recv(1024)
        sock.close()

        if data:
            return True, f"收到响应 ({len(data)} 字节)"
        else:
            return False, "无响应"
    except socket.timeout:
        return False, "连接超时"
    except Exception as e:
        return False, f"错误: {e}"

def test_file_transfer(ip: str, port: int = 8081) -> Tuple[bool, str]:
    """测试 File Transfer 服务 (仅测试连接)"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((ip, port))
        sock.close()
        return True, "连接成功"
    except socket.timeout:
        return False, "连接超时"
    except Exception as e:
        return False, f"错误: {e}"

def test_mdns(hostname: str = "grblesp.local") -> Tuple[bool, str]:
    """测试 mDNS 域名解析"""
    try:
        ip = socket.gethostbyname(hostname)
        return True, f"解析到 {ip}"
    except socket.gaierror:
        return False, "域名解析失败"
    except Exception as e:
        return False, f"错误: {e}"

def ping_test(ip: str, count: int = 1, timeout: float = 2.0) -> Tuple[bool, str]:
    """Ping 测试 (使用 socket 连接测试)"""
    try:
        # 简化的 ping 测试 - 尝试连接一个应该开放的端口
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        start = time.time()
        result = sock.connect_ex((ip, 80))  # 尝试连接 HTTP 端口
        elapsed = (time.time() - start) * 1000  # 转换为毫秒
        sock.close()

        if result == 0 or result == 111:  # 0=成功连接, 111=连接被拒绝但主机可达
            return True, f"主机可达 (延迟: {elapsed:.1f}ms)"
        else:
            return False, f"主机不可达 (错误: {result})"
    except socket.timeout:
        return False, "超时"
    except Exception as e:
        return False, f"错误: {e}"

def run_all_tests(ip: str, hostname: str = "grblesp.local", verbose: bool = False) -> Dict[str, bool]:
    """运行所有测试"""
    results = {}

    print_header("Grbl_ESP32 网络服务测试")

    print(f"{Colors.BOLD}测试目标:{Colors.RESET}")
    print(f"  IP 地址: {Colors.YELLOW}{ip}{Colors.RESET}")
    print(f"  主机名: {Colors.YELLOW}{hostname}{Colors.RESET}")
    print()

    # 1. 网络连通性测试
    print(f"{Colors.BOLD}[1/7] 网络连通性测试{Colors.RESET}")
    success, msg = ping_test(ip)
    results['ping'] = success
    print_test("Ping 测试", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    time.sleep(0.5)

    # 2. HTTP 服务测试 (端口 80)
    print(f"\n{Colors.BOLD}[2/7] HTTP Web Server (端口 80){Colors.RESET}")
    success, msg = test_tcp_port(ip, 80)
    results['http_port'] = success
    print_test("端口连通性", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    if success:
        success, msg = test_http(ip, 80)
        results['http_service'] = success
        print_test("HTTP 服务", "✓" if success else "✗")
        if verbose:
            print(f"      → {msg}")
    else:
        results['http_service'] = False
        print_test("HTTP 服务", "⊘")

    time.sleep(0.5)

    # 3. WebSocket 服务测试 (端口 81)
    print(f"\n{Colors.BOLD}[3/7] WebSocket Server (端口 81){Colors.RESET}")
    success, msg = test_tcp_port(ip, 81)
    results['websocket'] = success
    print_test("端口连通性", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    time.sleep(0.5)

    # 4. Telnet 服务测试 (端口 8080)
    print(f"\n{Colors.BOLD}[4/7] Telnet Server (端口 8080){Colors.RESET}")
    success, msg = test_tcp_port(ip, 8080)
    results['telnet_port'] = success
    print_test("端口连通性", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    if success:
        success, msg = test_telnet(ip, 8080)
        results['telnet_service'] = success
        print_test("Telnet 服务", "✓" if success else "✗")
        if verbose:
            print(f"      → {msg}")
    else:
        results['telnet_service'] = False
        print_test("Telnet 服务", "⊘")

    time.sleep(0.5)

    # 5. File Transfer 服务测试 (端口 8081)
    print(f"\n{Colors.BOLD}[5/7] File Transfer Server (端口 8081){Colors.RESET}")
    success, msg = test_tcp_port(ip, 8081)
    results['filetransfer_port'] = success
    print_test("端口连通性", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    if success:
        success, msg = test_file_transfer(ip, 8081)
        results['filetransfer_service'] = success
        print_test("File Transfer 服务", "✓" if success else "✗")
        if verbose:
            print(f"      → {msg}")
    else:
        results['filetransfer_service'] = False
        print_test("File Transfer 服务", "⊘")

    time.sleep(0.5)

    # 6. mDNS 测试
    print(f"\n{Colors.BOLD}[6/7] mDNS Service{Colors.RESET}")
    success, msg = test_mdns(hostname)
    results['mdns'] = success
    print_test("mDNS 域名解析", "✓" if success else "✗")
    if verbose:
        print(f"      → {msg}")

    time.sleep(0.5)

    # 7. SSDP/UPnP 测试 (端口 1900 UDP)
    print(f"\n{Colors.BOLD}[7/7] SSDP/UPnP (端口 1900 UDP){Colors.RESET}")
    print_test("SSDP 服务", "⊘")
    print(f"      → UDP 服务, 需要专门的发现工具")
    results['ssdp'] = None  # 跳过

    return results

def print_summary(results: Dict[str, bool]):
    """打印测试总结"""
    print_header("测试总结")

    total_tests = len([v for v in results.values() if v is not None])
    passed_tests = sum([1 for v in results.values() if v is True])
    failed_tests = sum([1 for v in results.values() if v is False])
    skipped_tests = sum([1 for v in results.values() if v is None])

    print(f"总测试数: {total_tests}")
    print(f"{Colors.GREEN}通过: {passed_tests}{Colors.RESET}")
    print(f"{Colors.RED}失败: {failed_tests}{Colors.RESET}")
    if skipped_tests > 0:
        print(f"{Colors.YELLOW}跳过: {skipped_tests}{Colors.RESET}")

    if passed_tests == total_tests:
        print(f"\n{Colors.GREEN}{Colors.BOLD}✓ 所有测试通过! {Colors.RESET}")
        return 0
    else:
        print(f"\n{Colors.RED}{Colors.BOLD}✗ 部分测试失败{Colors.RESET}")
        print(f"\n{Colors.YELLOW}建议:{Colors.RESET}")

        if not results.get('ping'):
            print("  • 检查网络连接和 IP 地址")
        if not results.get('http_service'):
            print("  • 检查 HTTP 服务是否启用 (Config.h: ENABLE_HTTP)")
        if not results.get('telnet_service'):
            print("  • 检查 Telnet 服务是否启用 (Config.h: ENABLE_TELNET)")
        if not results.get('filetransfer_service'):
            print("  • 检查 File Transfer 服务是否启用 (Config.h: ENABLE_FILE_TRANSFER)")
        if not results.get('mdns'):
            print("  • 检查 mDNS 服务是否启用 (Config.h: ENABLE_MDNS)")
            print("  • Windows: 安装 Bonjour 服务")
            print("  • Linux: 安装 avahi-daemon")

        return 1

def main():
    parser = argparse.ArgumentParser(
        description='Grbl_ESP32 网络服务完整测试工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
  %(prog)s 192.168.8.177                 # 测试所有服务
  %(prog)s 192.168.8.177 -v              # 详细输出
  %(prog)s 192.168.8.177 --hostname mycnc.local  # 自定义主机名
        '''
    )

    parser.add_argument('ip', help='ESP32 设备的 IP 地址')
    parser.add_argument('-H', '--hostname', default='grblesp.local',
                        help='mDNS 主机名 (默认: grblesp.local)')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='显示详细输出')
    parser.add_argument('--no-color', action='store_true',
                        help='禁用彩色输出')

    args = parser.parse_args()

    # 禁用颜色
    if args.no_color:
        Colors.GREEN = Colors.RED = Colors.YELLOW = ''
        Colors.BLUE = Colors.CYAN = Colors.RESET = Colors.BOLD = ''

    try:
        results = run_all_tests(args.ip, args.hostname, args.verbose)
        exit_code = print_summary(results)
        sys.exit(exit_code)

    except KeyboardInterrupt:
        print(f"\n\n{Colors.YELLOW}测试被用户中断{Colors.RESET}")
        sys.exit(130)

    except Exception as e:
        print(f"\n{Colors.RED}错误: {e}{Colors.RESET}")
        sys.exit(1)

if __name__ == '__main__':
    main()
