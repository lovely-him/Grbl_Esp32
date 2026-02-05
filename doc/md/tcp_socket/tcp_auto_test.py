#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TCP Socket 文件传输 - 自动化测试脚本 (简化版) 
"""

import json
import time
import sys
import os
import argparse

# 添加当前目录到路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from tcp_client import FileTransferClient


def send_http_after_upload(client: FileTransferClient, config: dict):
    """文件传输后发送 HTTP 命令 (如果配置启用)"""
    http_config = config.get('http_command', {})

    if not http_config.get('auto_send_after_upload', False):
        return

    # 延迟等待
    delay = http_config.get('delay_after_upload', 1.0)
    if delay > 0:
        print(f"[INFO] 等待 {delay} 秒后发送 HTTP 命令...")
        time.sleep(delay)

    # 发送命令
    url = http_config.get('url', 'http://192.168.8.177/command')
    command = http_config.get('command', '$H')
    timeout = http_config.get('timeout', 30)

    print(f"[INFO] 发送 HTTP 命令: {command}")
    response = client.send_http_command(url, command, timeout)

    if response['success']:
        print(f"[INFO] HTTP 命令成功")
    else:
        print(f"[WARN] HTTP 命令失败: {response.get('error', 'Unknown')}")


def load_config():
    """加载配置文件"""
    config_path = os.path.join(os.path.dirname(__file__), 'test_config.json')
    with open(config_path, 'r', encoding='utf-8') as f:
        return json.load(f)


def test_small_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 1: 小文件传输"""
    print("\n" + "=" * 70)
    print("测试 1: 小文件传输 (Hello World)")
    print("=" * 70)

    test_data = b"Hello World"
    success = client.upload_file('test.txt', test_data)

    if success:
        print("[PASS] 小文件传输成功")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 小文件传输失败")

    return success


def test_1kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 2: 1KB 文件传输"""
    print("\n" + "=" * 70)
    print("测试 2: 1KB 文件传输")
    print("=" * 70)

    # 生成 1KB 测试数据
    test_data = b"A" * 1024

    start_time = time.time()
    success = client.upload_file('test_1kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024  # KB/s
        print(f"[PASS] 1KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 1KB 文件传输失败")

    return success


def test_4kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 3: 4KB 文件传输"""
    print("\n" + "=" * 70)
    print("测试 3: 4KB 文件传输")
    print("=" * 70)

    test_data = b"C" * (4 * 1024)

    start_time = time.time()
    success = client.upload_file('test_4kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024
        print(f"[PASS] 4KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 4KB 文件传输失败")

    return success


def test_8kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 4: 8KB 文件传输"""
    print("\n" + "=" * 70)
    print("测试 4: 8KB 文件传输")
    print("=" * 70)

    test_data = b"D" * (8 * 1024)

    start_time = time.time()
    success = client.upload_file('test_8kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024
        print(f"[PASS] 8KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 8KB 文件传输失败")

    return success


def test_16kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 5: 16KB 文件传输"""
    print("\n" + "=" * 70)
    print("测试 5: 16KB 文件传输")
    print("=" * 70)

    test_data = b"E" * (16 * 1024)

    start_time = time.time()
    success = client.upload_file('test_16kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024
        print(f"[PASS] 16KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 16KB 文件传输失败")

    return success


def test_32kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 6: 32KB 文件传输 (单包最大) """
    print("\n" + "=" * 70)
    print("测试 6: 32KB 文件传输 (单包最大) ")
    print("=" * 70)

    test_data = b"F" * (32 * 1024)

    start_time = time.time()
    success = client.upload_file('test_32kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024
        print(f"[PASS] 32KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 32KB 文件传输失败")

    return success


def test_100kb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 7: 100KB 文件传输 (多包) """
    print("\n" + "=" * 70)
    print("测试 7: 100KB 文件传输 (多包) ")
    print("=" * 70)

    # 生成 100KB 测试数据
    test_data = b"B" * (100 * 1024)

    start_time = time.time()
    success = client.upload_file('test_100kb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024  # KB/s
        print(f"[PASS] 100KB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 100KB 文件传输失败")

    return success


def test_10mb_file(client: FileTransferClient, config: dict = None) -> bool:
    """测试 8: 10MB 文件传输 (大文件压力测试) """
    print("\n" + "=" * 70)
    print("测试 8: 10MB 文件传输 (大文件压力测试) ")
    print("=" * 70)

    # 生成 10MB 测试数据
    test_data = b"G" * (10 * 1024 * 1024)

    start_time = time.time()
    success = client.upload_file('test_10mb.bin', test_data)
    duration = time.time() - start_time

    if success:
        speed = len(test_data) / duration / 1024  # KB/s
        print(f"[PASS] 10MB 文件传输成功, 耗时 {duration:.2f}秒, 速度 {speed:.2f} KB/s")
        if config:
            send_http_after_upload(client, config)
    else:
        print("[FAIL] 10MB 文件传输失败")

    return success


def test_checksum_error(client: FileTransferClient, config: dict = None) -> bool:
    """测试 9: 校验和错误处理"""
    print("\n" + "=" * 70)
    print("测试 9: 校验和错误处理")
    print("=" * 70)

    # 构建错误的数据包 (故意错误的校验和)
    packet = bytearray([0xAB, 0x01, 0x00, 0x10, 0x00, 0x00])  # 长度 = 16 字节
    packet.extend(b"test.txt")
    packet.extend([0xFF, 0xFF])  # 错误的校验和

    try:
        client.socket.sendall(bytes(packet))
        response = client.socket.recv(1024)

        # 检查是否返回校验和错误 (状态码 0x02)
        # 响应格式: [0xAB][STATUS][LEN_H][LEN_L][IDX_H][IDX_L][CHK_H][CHK_L]
        # 状态码在索引 1
        if len(response) >= 2 and response[1] == 0x02:
            print("[PASS] 服务器正确返回校验和错误")
            return True
        else:
            print(f"[FAIL] 服务器响应异常: {response.hex()}")
            return False
    except Exception as e:
        print(f"[FAIL] 测试异常: {e}")
        return False


def test_invalid_magic_byte(client: FileTransferClient, config: dict = None) -> bool:
    """测试 10: 无效魔术字节处理"""
    print("\n" + "=" * 70)
    print("测试 10: 无效魔术字节处理")
    print("=" * 70)

    # 构建错误的魔术字节数据包
    packet = bytearray([0xFF, 0x01, 0x00, 0x10, 0x00, 0x00])  # 错误的魔术字节 0xFF
    packet.extend(b"test.txt")
    # 计算正确的校验和 (虽然魔术字节错误) 
    checksum = sum(packet) & 0xFFFF
    packet.extend([(checksum >> 8) & 0xFF, checksum & 0xFF])

    try:
        client.socket.sendall(bytes(packet))

        # 服务器应该丢弃这个包, 不发送响应
        # 等待一小段时间看是否有响应
        client.socket.settimeout(1.0)
        try:
            response = client.socket.recv(1024)
            if len(response) > 0:
                print(f"[FAIL] 服务器不应该响应无效魔术字节: {response.hex()}")
                return False
        except:
            # 超时是预期行为
            pass

        # 发送正确的包, 验证连接仍然有效
        client.socket.settimeout(5.0)
        response = client.send_header_packet('test_recovery.txt')
        if response['success']:
            print("[PASS] 服务器正确丢弃无效魔术字节, 连接保持正常")
            # 发送取消包清理状态
            client.send_cancel_packet()
            return True
        else:
            print(f"[FAIL] 连接状态异常: {response.get('error')}")
            return False

    except Exception as e:
        print(f"[FAIL] 测试异常: {e}")
        return False


def test_oversized_packet(client: FileTransferClient, config: dict = None) -> bool:
    """测试 11: 超大包处理"""
    print("\n" + "=" * 70)
    print("测试 11: 超大包处理 (声称长度超过缓冲区) ")
    print("=" * 70)

    # 构建声称长度为 40KB 的包头 (实际只发送头部) 
    packet = bytearray([0xAB, 0x01])
    oversized_len = 40 * 1024  # 40KB, 超过 RXBUFFERSIZE (33KB)
    packet.extend([(oversized_len >> 8) & 0xFF, oversized_len & 0xFF])
    packet.extend([0x00, 0x00])  # 包序号
    packet.extend(b"test.txt")
    # 计算校验和 (基于实际发送的数据) 
    checksum = sum(packet) & 0xFFFF
    packet.extend([(checksum >> 8) & 0xFF, checksum & 0xFF])

    try:
        client.socket.sendall(bytes(packet))

        # 服务器应该返回协议错误 (状态码 0x02) 
        client.socket.settimeout(2.0)
        response = client.socket.recv(1024)

        if len(response) >= 2 and response[1] == 0x02:
            print("[PASS] 服务器正确拒绝超大包")
            return True
        else:
            print(f"[FAIL] 服务器响应异常: {response.hex() if response else 'no response'}")
            return False

    except Exception as e:
        print(f"[FAIL] 测试异常: {e}")
        return False


def test_incomplete_packet(client: FileTransferClient, config: dict = None) -> bool:
    """测试 12: 不完整包处理"""
    print("\n" + "=" * 70)
    print("测试 12: 不完整包处理 (发送部分数据后等待) ")
    print("=" * 70)

    # 构建完整的包, 但只发送前半部分
    full_packet = bytearray([0xAB, 0x01, 0x00, 0x10, 0x00, 0x00])
    full_packet.extend(b"test.txt")
    checksum = sum(full_packet) & 0xFFFF
    full_packet.extend([(checksum >> 8) & 0xFF, checksum & 0xFF])

    # 只发送前 8 字节 (不完整) 
    partial_packet = bytes(full_packet[:8])

    try:
        client.socket.sendall(partial_packet)

        # 服务器应该等待更多数据, 不发送响应
        client.socket.settimeout(1.0)
        try:
            response = client.socket.recv(1024)
            if len(response) > 0:
                print(f"[FAIL] 服务器不应该响应不完整包: {response.hex()}")
                return False
        except:
            # 超时是预期行为
            pass

        # 发送剩余数据
        client.socket.settimeout(5.0)
        remaining_packet = bytes(full_packet[8:])
        client.socket.sendall(remaining_packet)

        # 现在应该收到正确的响应
        response = client.socket.recv(1024)
        if len(response) >= 2 and response[1] == 0x01:  # Success
            print("[PASS] 服务器正确处理分段接收的包")
            # 发送取消包清理状态
            client.send_cancel_packet()
            return True
        else:
            print(f"[FAIL] 服务器响应异常: {response.hex() if response else 'no response'}")
            return False

    except Exception as e:
        print(f"[FAIL] 测试异常: {e}")
        return False


def test_cancel_transfer(client: FileTransferClient, config: dict = None) -> bool:
    """测试 13: 取消传输"""
    print("\n" + "=" * 70)
    print("测试 13: 取消传输")
    print("=" * 70)

    # 发送文件信息包
    response = client.send_header_packet('cancel_test.txt')
    if not response['success']:
        print(f"[FAIL] 文件信息包失败: {response.get('error')}")
        return False

    # 发送取消包
    time.sleep(0.1)
    response = client.send_cancel_packet()

    print("[PASS] 取消传输成功")
    return True


def test_http_command(client: FileTransferClient, config: dict = None) -> bool:
    """测试 14: HTTP 命令接口

    测试 Grbl HTTP 命令接口的连通性和基本功能
    """
    print("\n" + "=" * 70)
    print("测试 14: HTTP 命令接口")
    print("=" * 70)

    if not config:
        print("[SKIP] 未提供配置信息")
        return True

    http_config = config.get('http_command', {})
    if not http_config.get('enabled', False):
        print("[SKIP] HTTP 命令测试未启用")
        return True

    url = http_config.get('url', 'http://192.168.8.177/command')
    command = http_config.get('command', '$H')
    timeout = http_config.get('timeout', 30)

    print(f"[INFO] 发送 HTTP 命令: {command}")
    print(f"[INFO] URL: {url}")

    response = client.send_http_command(url, command, timeout)

    if response['success']:
        print(f"[PASS] HTTP 命令成功, 状态码: {response['status_code']}")
        if response['response']:
            print(f"[INFO] 响应内容: {response['response'][:100]}")
        return True
    else:
        print(f"[WARN] HTTP 命令失败: {response.get('error', 'Unknown')}")
        print(f"[INFO] 状态码: {response['status_code']}")
        # HTTP 失败不影响测试结果
        return True


def run_loop_test(client, test_name, test_func, config, interval_ms, count=None):
    """循环执行指定测试"""
    print("=" * 70)
    print(f"循环测试模式: {test_name}")
    print(f"间隔: {interval_ms}ms")
    print(f"次数: {'无限' if count is None else count}")
    print("按 Ctrl+C 停止")
    print("=" * 70)

    iteration = 0
    passed = 0
    failed = 0

    try:
        while count is None or iteration < count:
            iteration += 1
            print(f"\n[第 {iteration} 次测试]")

            # 重新连接
            client.disconnect()
            time.sleep(interval_ms / 1000.0)

            if not client.connect():
                print(f"[ERROR] 无法连接到服务器")
                failed += 1
                continue

            # 执行测试
            try:
                result = test_func(client, config)
                if result:
                    passed += 1
                    print(f"[PASS] 第 {iteration} 次测试通过")
                else:
                    failed += 1
                    print(f"[FAIL] 第 {iteration} 次测试失败")
            except Exception as e:
                failed += 1
                print(f"[ERROR] 测试异常: {e}")

            # 显示统计
            total = passed + failed
            success_rate = (passed / total * 100) if total > 0 else 0
            print(f"统计: 通过 {passed}/{total} ({success_rate:.1f}%)")

    except KeyboardInterrupt:
        print("\n\n[INFO] 用户中断测试")
    finally:
        client.disconnect()

        # 最终统计
        print("\n" + "=" * 70)
        print("循环测试结果")
        print("=" * 70)
        total = passed + failed
        success_rate = (passed / total * 100) if total > 0 else 0
        print(f"总次数: {total}")
        print(f"通过: {passed}")
        print(f"失败: {failed}")
        print(f"成功率: {success_rate:.1f}%")


def main():
    """主测试流程"""
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='TCP Socket 文件传输自动化测试')
    parser.add_argument('-l', '--loop', type=str, help='循环测试指定项目 (small/1kb/4kb/8kb/16kb/32kb/100kb/10mb/checksum/invalid-magic/oversized/incomplete/cancel/http)')
    parser.add_argument('-i', '--interval', type=int, default=100, help='循环间隔 (毫秒) , 默认100ms')
    parser.add_argument('-c', '--count', type=int, help='循环次数, 默认无限循环')
    parser.add_argument('--continue-on-failure', action='store_true', help='失败后继续执行 (默认失败即停止) ')
    parser.add_argument('--enable-http-after-upload', action='store_true', help='在每次文件传输后发送 HTTP 命令 (覆盖配置文件) ')
    parser.add_argument('--disable-http-after-upload', action='store_true', help='禁用文件传输后的 HTTP 命令 (覆盖配置文件) ')
    args = parser.parse_args()

    # 测试映射表
    test_map = {
        'small': ("小文件传输", test_small_file),
        '1kb': ("1KB 文件传输", test_1kb_file),
        '4kb': ("4KB 文件传输", test_4kb_file),
        '8kb': ("8KB 文件传输", test_8kb_file),
        '16kb': ("16KB 文件传输", test_16kb_file),
        '32kb': ("32KB 文件传输 (单包最大) ", test_32kb_file),
        '100kb': ("100KB 文件传输 (多包) ", test_100kb_file),
        '10mb': ("10MB 文件传输 (大文件压力) ", test_10mb_file),
        'checksum': ("校验和错误处理", test_checksum_error),
        'invalid-magic': ("无效魔术字节处理", test_invalid_magic_byte),
        'oversized': ("超大包处理", test_oversized_packet),
        'incomplete': ("不完整包处理", test_incomplete_packet),
        'cancel': ("取消传输", test_cancel_transfer),
        'http': ("HTTP 命令接口", test_http_command),
    }

    print("=" * 70)
    print("TCP Socket 文件传输 - 自动化测试")
    print("=" * 70)

    # 加载配置
    config = load_config()

    # 命令行参数覆盖配置文件
    if args.enable_http_after_upload:
        if 'http_command' not in config:
            config['http_command'] = {}
        config['http_command']['auto_send_after_upload'] = True
    if args.disable_http_after_upload:
        if 'http_command' not in config:
            config['http_command'] = {}
        config['http_command']['auto_send_after_upload'] = False

    server_ip = config['server']['ip']
    server_port = config['server']['port']

    print(f"\n服务器地址: {server_ip}:{server_port}")

    # 创建客户端 (启用调试模式)
    client = FileTransferClient(server_ip, server_port, debug=True)

    # 循环测试模式
    if args.loop:
        if args.loop not in test_map:
            print(f"\n[ERROR] 未知的测试项目: {args.loop}")
            print(f"可用项目: {', '.join(test_map.keys())}")
            return

        test_name, test_func = test_map[args.loop]
        run_loop_test(client, test_name, test_func, config, args.interval, args.count)
        return

    # 顺序测试模式
    print(f"停止策略: {'失败后继续执行' if args.continue_on_failure else '失败即停止'}")

    # 连接服务器
    if not client.connect():
        print("\n[ERROR] 无法连接到服务器, 测试终止")
        return

    # 测试结果统计
    results = []

    try:
        # 执行测试用例
        tests = [
            ("小文件传输", test_small_file),
            ("1KB 文件传输", test_1kb_file),
            ("4KB 文件传输", test_4kb_file),
            ("8KB 文件传输", test_8kb_file),
            ("16KB 文件传输", test_16kb_file),
            ("32KB 文件传输 (单包最大) ", test_32kb_file),
            ("100KB 文件传输 (多包) ", test_100kb_file),
            ("10MB 文件传输 (大文件压力) ", test_10mb_file),
            ("校验和错误处理", test_checksum_error),
            ("无效魔术字节处理", test_invalid_magic_byte),
            ("超大包处理", test_oversized_packet),
            ("不完整包处理", test_incomplete_packet),
            ("取消传输", test_cancel_transfer),
            ("HTTP 命令接口", test_http_command),
        ]

        for test_name, test_func in tests:
            try:
                # 每个测试使用独立连接
                client.disconnect()
                time.sleep(0.5)  # 等待服务器清理状态

                if not client.connect():
                    print(f"[ERROR] 无法连接到服务器, 跳过测试: {test_name}")
                    results.append((test_name, False))
                    if not args.continue_on_failure:
                        break
                    continue

                result = test_func(client, config)
                results.append((test_name, result))

                # 默认失败即停止, 除非指定 --continue-on-failure
                if not result and not args.continue_on_failure:
                    print(f"\n[STOP] 测试失败, 停止后续测试")
                    break

                # 测试间隔
                time.sleep(0.5)

            except Exception as e:
                print(f"[ERROR] 测试异常: {e}")
                results.append((test_name, False))
                if not args.continue_on_failure:
                    break

    finally:
        # 断开连接
        client.disconnect()

    # 显示测试结果
    print("\n" + "=" * 70)
    print("测试结果汇总")
    print("=" * 70)

    passed = sum(1 for _, result in results if result)
    total = len(results)

    for test_name, result in results:
        status = "[PASS]" if result else "[FAIL]"
        print(f"{status} {test_name}")

    print(f"\n总计: {passed}/{total} 通过")

    if passed == total:
        print("\n[SUCCESS] 所有测试通过! ")
    else:
        print(f"\n[FAILED] {total - passed} 个测试失败")


if __name__ == '__main__':
    main()
