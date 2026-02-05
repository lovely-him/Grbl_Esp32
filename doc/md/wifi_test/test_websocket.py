#!/usr/bin/env python3
"""
WebSocket Server 测试脚本
测试 Grbl_ESP32 的 WebSocket 服务功能
"""

import sys
import socket
import struct
import base64
import hashlib
import argparse
from typing import Tuple, Optional

def create_websocket_handshake(host: str, port: int) -> bytes:
    """创建 WebSocket 握手请求"""
    key = base64.b64encode(b"grbl_test_key_12").decode()

    request = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        f"Sec-WebSocket-Version: 13\r\n"
        f"\r\n"
    )
    return request.encode()

def parse_handshake_response(response: bytes) -> bool:
    """解析 WebSocket 握手响应"""
    try:
        response_str = response.decode('utf-8', errors='ignore')
        lines = response_str.split('\r\n')

        if not lines[0].startswith('HTTP/1.1 101'):
            print(f"  ✗ 握手失败: {lines[0]}")
            return False

        headers = {}
        for line in lines[1:]:
            if ':' in line:
                key, value = line.split(':', 1)
                headers[key.strip().lower()] = value.strip()

        if headers.get('upgrade', '').lower() != 'websocket':
            print(f"  ✗ 协议错误: Upgrade header = {headers.get('upgrade')}")
            return False

        return True
    except Exception as e:
        print(f"  ✗ 解析响应失败: {e}")
        return False

def create_websocket_frame(message: str) -> bytes:
    """创建 WebSocket 数据帧 (客户端发送, 需要掩码)"""
    payload = message.encode()
    frame = bytearray()

    # FIN=1, Opcode=1 (text frame)
    frame.append(0x81)

    # Mask=1, Payload length
    length = len(payload)
    if length < 126:
        frame.append(0x80 | length)
    elif length < 65536:
        frame.append(0x80 | 126)
        frame.extend(struct.pack('!H', length))
    else:
        frame.append(0x80 | 127)
        frame.extend(struct.pack('!Q', length))

    # Masking key (4 bytes)
    mask_key = b'\x00\x00\x00\x00'
    frame.extend(mask_key)

    # Masked payload
    frame.extend(payload)

    return bytes(frame)

def parse_websocket_frame(data: bytes) -> Optional[str]:
    """解析 WebSocket 数据帧"""
    try:
        if len(data) < 2:
            return None

        opcode = data[0] & 0x0F
        masked = bool(data[1] & 0x80)
        length = data[1] & 0x7F

        offset = 2
        if length == 126:
            if len(data) < 4:
                return None
            length = struct.unpack('!H', data[2:4])[0]
            offset = 4
        elif length == 127:
            if len(data) < 10:
                return None
            length = struct.unpack('!Q', data[2:10])[0]
            offset = 10

        if masked:
            mask_key = data[offset:offset+4]
            offset += 4

        if len(data) < offset + length:
            return None

        payload = data[offset:offset+length]

        if masked:
            payload = bytes([payload[i] ^ mask_key[i % 4] for i in range(len(payload))])

        return payload.decode('utf-8', errors='ignore')
    except Exception:
        return None

def test_websocket_service(ip: str, port: int = 81) -> bool:
    """测试 WebSocket 基本连接和通信"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)

        print(f"连接到 WebSocket {ip}:{port}...")
        sock.connect((ip, port))
        print("✓ TCP 连接成功")

        # 发送握手请求
        handshake = create_websocket_handshake(ip, port)
        sock.sendall(handshake)
        print("✓ 发送握手请求")

        # 接收握手响应
        response = sock.recv(1024)
        if not parse_handshake_response(response):
            return False
        print("✓ WebSocket 握手成功")

        # 发送查询命令
        frame = create_websocket_frame("?")
        sock.sendall(frame)
        print("✓ 发送查询命令: ?")

        # 接收响应
        data = sock.recv(4096)
        if data:
            message = parse_websocket_frame(data)
            if message:
                print(f"✓ 收到响应: {message[:100]}...")
            else:
                print("✓ 收到数据 (无法解析)")

        sock.close()
        return True

    except socket.timeout:
        print("✗ 连接超时")
        return False
    except ConnectionRefusedError:
        print("✗ 连接被拒绝 (服务未启动?)")
        return False
    except Exception as e:
        print(f"✗ 测试失败: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='测试 Grbl_ESP32 WebSocket 服务')
    parser.add_argument('ip', help='设备 IP 地址')
    parser.add_argument('-p', '--port', type=int, default=81, help='WebSocket 端口 (默认: 81)')
    args = parser.parse_args()

    print(f"\n{'='*60}")
    print(f"WebSocket Server 测试")
    print(f"{'='*60}\n")

    result = test_websocket_service(args.ip, args.port)

    print(f"\n{'='*60}")
    if result:
        print("✓ WebSocket 服务测试通过")
    else:
        print("✗ WebSocket 服务测试失败")
    print(f"{'='*60}\n")

    return 0 if result else 1

if __name__ == '__main__':
    sys.exit(main())
