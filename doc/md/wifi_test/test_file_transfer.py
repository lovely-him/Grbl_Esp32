#!/usr/bin/env python3
"""
File Transfer Server 测试脚本
测试 Grbl_ESP32 的 TCP 文件传输服务功能
"""

import sys
import socket
import struct
import argparse
from typing import Tuple, Optional

# 协议常量
FRAME_HEADER = 0xAB
CMD_HEADER = 0x01  # 文件名包
CMD_DATA = 0x02    # 数据包
CMD_CANCEL = 0x03  # 取消包

RESP_SUCCESS = 0x01
RESP_PROTOCOL_ERROR = 0x02
RESP_SERVER_ERROR = 0x03

def calculate_checksum(data: bytes) -> int:
    """计算 16 位累加校验和"""
    checksum = 0
    for byte in data:
        checksum = (checksum + byte) & 0xFFFF
    return checksum

def create_packet(cmd: int, packet_idx: int, data: bytes) -> bytes:
    """创建传输包"""
    data_len = len(data)

    # 构造包头 (不包含校验和)
    header = struct.pack('>BBHHH',
        FRAME_HEADER,
        cmd,
        data_len,
        packet_idx,
        0  # 校验和占位符
    )

    # 计算校验和 (包头前6字节 + 数据)
    checksum_data = header[:6] + data
    if cmd == CMD_CANCEL:
        checksum_data = header[:4]  # Cancel 包只计算前4字节

    checksum = calculate_checksum(checksum_data)

    # 重新构造包 (带校验和)
    packet = struct.pack('>BBHHH',
        FRAME_HEADER,
        cmd,
        data_len,
        packet_idx,
        checksum
    ) + data

    return packet

def parse_response(data: bytes) -> Tuple[int, int, int]:
    """解析响应包"""
    if len(data) < 8:
        raise ValueError(f"响应包过短: {len(data)} 字节")

    header, status, data_len, packet_idx, checksum = struct.unpack('>BBHHH', data[:8])

    if header != FRAME_HEADER:
        raise ValueError(f"无效的帧头: 0x{header:02X}")

    return status, packet_idx, checksum

def test_file_transfer_service(ip: str, port: int = 8081) -> bool:
    """测试 File Transfer 基本连接和协议"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)

        print(f"连接到 {ip}:{port}...")
        sock.connect((ip, port))
        print("✓ TCP 连接成功")

        # 测试 1: 发送文件名包
        filename = "test.txt"
        filename_bytes = filename.encode('utf-8')
        packet = create_packet(CMD_HEADER, 0, filename_bytes)

        print(f"\n发送文件名包: '{filename}' ({len(filename_bytes)} 字节)")
        sock.sendall(packet)

        # 接收响应
        response = sock.recv(8)
        status, idx, _ = parse_response(response)

        if status == RESP_SUCCESS:
            print(f"✓ 服务器接受文件名 (期望包序号: {idx})")
        else:
            print(f"✗ 服务器返回错误状态: {status}")
            return False

        # 测试 2: 发送数据包
        test_data = b"Hello, Grbl_ESP32!"
        packet = create_packet(CMD_DATA, 1, test_data)

        print(f"\n发送数据包: {len(test_data)} 字节")
        sock.sendall(packet)

        # 接收响应
        response = sock.recv(8)
        status, idx, _ = parse_response(response)

        if status == RESP_SUCCESS:
            print(f"✓ 服务器接受数据 (期望包序号: {idx})")
        else:
            print(f"✗ 服务器返回错误状态: {status}")
            return False

        # 测试 3: 发送取消包
        packet = create_packet(CMD_CANCEL, 0, b"")

        print("\n发送取消包...")
        sock.sendall(packet)

        # 接收响���
        response = sock.recv(8)
        status, idx, _ = parse_response(response)

        if status == RESP_SUCCESS:
            print("✓ 服务器确认取消")
        else:
            print(f"✗ 服务器返回错误状态: {status}")
            return False

        sock.close()
        print("\n✓ File Transfer Server 测试通过")
        return True

    except socket.timeout:
        print("✗ 连接超时")
        return False
    except ConnectionRefusedError:
        print(f"✗ 连接被拒绝 - 服务器可能未启动")
        return False
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_file_transfer_advanced(ip: str, port: int = 8081, filename: str = "/test_upload.gcode") -> bool:
    """高级测试: 上传完整文件"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)

        print(f"连接到 {ip}:{port}...")
        sock.connect((ip, port))
        print("✓ TCP 连接成功")

        # 发送文件名
        filename_bytes = filename.encode('utf-8')
        packet = create_packet(CMD_HEADER, 0, filename_bytes)

        print(f"\n上传文件: '{filename}'")
        sock.sendall(packet)

        response = sock.recv(8)
        status, next_idx, _ = parse_response(response)

        if status != RESP_SUCCESS:
            print(f"✗ 服务器拒绝文件名: 状态 {status}")
            return False

        print(f"✓ 文件名已接受")

        # 发送测试数据 (简单的 G-code)
        gcode_data = b"; Test G-code file\nG21 ; mm-mode\nG90 ; absolute positioning\nG0 X0 Y0\nM5 ; laser off\n"

        packet_idx = 1
        max_data_size = 1024  # 每包 1KB
        offset = 0

        while offset < len(gcode_data):
            chunk = gcode_data[offset:offset + max_data_size]
            packet = create_packet(CMD_DATA, packet_idx, chunk)

            print(f"发送数据包 {packet_idx}: {len(chunk)} 字节")
            sock.sendall(packet)

            response = sock.recv(8)
            status, next_idx, _ = parse_response(response)

            if status != RESP_SUCCESS:
                print(f"✗ 服务器返回错误: 状态 {status}")
                return False

            if next_idx != packet_idx + 1:
                print(f"✗ 包序号不匹配: 期望 {packet_idx + 1}, 收到 {next_idx}")
                return False

            offset += len(chunk)
            packet_idx += 1

        print(f"✓ 文件上传完成: {len(gcode_data)} 字节, {packet_idx - 1} 个数据包")

        sock.close()
        return True

    except Exception as e:
        print(f"✗ 高级测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    parser = argparse.ArgumentParser(description='测试 Grbl_ESP32 File Transfer Server')
    parser.add_argument('ip', help='设备 IP 地址')
    parser.add_argument('-p', '--port', type=int, default=8081, help='File Transfer 端口 (默认: 8081)')
    parser.add_argument('-a', '--advanced', action='store_true', help='运行高级测试 (上传完整文件)')
    parser.add_argument('-f', '--filename', default='/test_upload.gcode', help='上传文件名 (默认: /test_upload.gcode)')

    args = parser.parse_args()

    print("=" * 60)
    print("Grbl_ESP32 File Transfer Server 测试")
    print("=" * 60)

    # 基本测试
    if not test_file_transfer_service(args.ip, args.port):
        sys.exit(1)

    # 高级测试
    if args.advanced:
        print("\n" + "=" * 60)
        print("高级测试: 完整文件上传")
        print("=" * 60)
        if not test_file_transfer_advanced(args.ip, args.port, args.filename):
            sys.exit(1)

    print("\n✓ 所有测试通过!")

if __name__ == '__main__':
    main()
