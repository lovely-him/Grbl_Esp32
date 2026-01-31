#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TCP Socket 文件传输客户端

基于 tcp_client.dart 的 Python 实现
"""

import socket
import time
from typing import Optional, Dict, Callable


class FileTransferClient:
    """TCP Socket 文件传输客户端"""

    # 协议常量
    MAGIC_BYTE = 0xAB
    CMD_HEADER = 0x01
    CMD_DATA = 0x02
    CMD_CANCEL = 0x03

    # 状态码
    STATUS_SUCCESS = 0x01
    STATUS_CHECKSUM_ERROR = 0x02
    STATUS_TIMEOUT_ERROR = 0x03

    # 数据包大小
    MAX_PACKET_SIZE = 32 * 1024  # 32KB

    def __init__(self, host: str, port: int = 8080, timeout: int = 5, debug: bool = False):
        """初始化客户端

        Args:
            host: 服务器 IP 地址
            port: 服务器端口
            timeout: 超时时间（秒）
            debug: 是否启用调试日志
        """
        self.host = host
        self.port = port
        self.timeout = timeout
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.debug = debug

    def connect(self) -> bool:
        """连接到服务器

        Returns:
            连接是否成功
        """
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"[INFO] 已连接到 {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"[ERROR] 连接失败: {e}")
            self.connected = False
            return False

    def disconnect(self):
        """断开连接"""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
        self.connected = False
        print("[INFO] 已断开连接")

    def upload_file(self, filename: str, data: bytes,
                   progress_callback: Optional[Callable[[int], None]] = None) -> bool:
        """上传文件

        Args:
            filename: 文件名
            data: 文件数据
            progress_callback: 进度回调函数 (percent: int)

        Returns:
            上传是否成功
        """
        if not self.connected:
            print("[ERROR] 未连接到服务器")
            return False

        try:
            # 1. 发送文件信息包
            print(f"[INFO] 发送文件信息: {filename}")
            response = self.send_header_packet(filename)
            if not response['success']:
                print(f"[ERROR] 文件信息包失败: {response.get('error', 'Unknown')}")
                return False

            # 2. 分包发送数据
            total_packets = (len(data) + self.MAX_PACKET_SIZE - 1) // self.MAX_PACKET_SIZE
            print(f"[INFO] 开始传输 {len(data)} 字节，共 {total_packets} 个数据包")

            for packet_index in range(1, total_packets + 1):
                start = (packet_index - 1) * self.MAX_PACKET_SIZE
                end = min(start + self.MAX_PACKET_SIZE, len(data))
                chunk = data[start:end]

                # 发送数据包
                response = self.send_data_packet(packet_index, chunk)
                if not response['success']:
                    print(f"[ERROR] 数据包 {packet_index} 失败: {response.get('error', 'Unknown')}")
                    return False

                # 更新进度
                if progress_callback:
                    progress = int((packet_index / total_packets) * 100)
                    progress_callback(progress)

                print(f"[INFO] 数据包 {packet_index}/{total_packets} 已发送")

            print("[INFO] 文件传输完成")
            return True

        except Exception as e:
            print(f"[ERROR] 上传失败: {e}")
            return False

    def send_header_packet(self, filename: str) -> Dict:
        """发送文件信息包

        Args:
            filename: 文件名

        Returns:
            响应字典 {'success': bool, 'next_packet_index': int, 'error': str}
        """
        packet = self._build_header_packet(filename)
        return self._send_and_receive(packet)

    def send_data_packet(self, index: int, data: bytes) -> Dict:
        """发送数据包

        Args:
            index: 包序号
            data: 数据内容

        Returns:
            响应字典
        """
        packet = self._build_data_packet(index, data)
        return self._send_and_receive(packet)

    def send_cancel_packet(self) -> Dict:
        """发送取消包

        Returns:
            响应字典
        """
        packet = self._build_cancel_packet()
        return self._send_and_receive(packet)

    def _build_header_packet(self, filename: str) -> bytes:
        """构建文件信息包

        格式: [0xAB][0x01][长度2B][序号2B][文件名UTF-8][校验2B]
        """
        filename_bytes = filename.encode('utf-8')
        packet_len = 6 + len(filename_bytes) + 2

        packet = bytearray()
        packet.append(self.MAGIC_BYTE)
        packet.append(self.CMD_HEADER)
        packet.extend(packet_len.to_bytes(2, 'big'))
        packet.extend((0).to_bytes(2, 'big'))  # 序号固定为 0
        packet.extend(filename_bytes)
        packet.extend((0).to_bytes(2, 'big'))  # 校验和占位

        # 计算并添加校验和
        checksum = self._calculate_checksum(packet)
        packet[-2] = (checksum >> 8) & 0xFF
        packet[-1] = checksum & 0xFF

        if self.debug:
            print(f"[DEBUG] >>> 发送文件名包: {self._format_packet(packet, show_data=True)}")

        return bytes(packet)

    def _build_data_packet(self, index: int, data: bytes) -> bytes:
        """构建数据包

        格式: [0xAB][0x02][长度2B][序号2B][数据][校验2B]
        """
        packet_len = 6 + len(data) + 2

        packet = bytearray()
        packet.append(self.MAGIC_BYTE)
        packet.append(self.CMD_DATA)
        packet.extend(packet_len.to_bytes(2, 'big'))
        packet.extend(index.to_bytes(2, 'big'))
        packet.extend(data)
        packet.extend((0).to_bytes(2, 'big'))  # 校验和占位

        # 计算并添加校验和
        checksum = self._calculate_checksum(packet)
        packet[-2] = (checksum >> 8) & 0xFF
        packet[-1] = checksum & 0xFF

        if self.debug:
            print(f"[DEBUG] >>> 发送数据包: {self._format_packet(packet, show_data=False)}")

        return bytes(packet)

    def _build_cancel_packet(self) -> bytes:
        """构建取消包

        格式: [0xAB][0x03][0x00][0x06][校验2B]
        """
        packet = bytearray([self.MAGIC_BYTE, self.CMD_CANCEL, 0x00, 0x06, 0x00, 0x00])

        # 计算并添加校验和
        checksum = self._calculate_checksum(packet)
        packet[-2] = (checksum >> 8) & 0xFF
        packet[-1] = checksum & 0xFF

        if self.debug:
            print(f"[DEBUG] >>> 发送取消包: {self._format_packet(packet, show_data=True)}")

        return bytes(packet)

    def _calculate_checksum(self, data: bytes) -> int:
        """计算校验和（累加和的低 16 位）

        Args:
            data: 数据包（包含校验和占位符）

        Returns:
            校验和（16位）
        """
        # 累加除了最后 2 字节（校验和）之外的所有字节
        checksum = sum(data[:-2]) & 0xFFFF
        return checksum

    def _send_and_receive(self, packet: bytes) -> Dict:
        """发送数据包并接收响应

        Args:
            packet: 要发送的数据包

        Returns:
            响应字典
        """
        try:
            # 发送数据包
            self.socket.sendall(packet)

            # 接收响应（最少 7 字节）
            response = self.socket.recv(1024)
            if len(response) < 7:
                return {'success': False, 'error': 'Invalid response length'}

            # 调试日志：显示响应
            if self.debug:
                print(f"[DEBUG] <<< 收到应答: {self._format_response(response)}")

            # 解析响应
            return self._parse_response(response)

        except socket.timeout:
            return {'success': False, 'error': 'Timeout'}
        except Exception as e:
            return {'success': False, 'error': str(e)}

    def _parse_response(self, data: bytes) -> Dict:
        """解析服务器响应

        响应格式（适配 Dart 客户端）:
        [0xAB][STATUS][LEN_H][LEN_L][IDX_H][IDX_L][CHK_H][CHK_L]
        - 固定 8 字节
        - STATUS: 0x01=成功, 0x02=校验错误, 0x03=超时/错误
        - IDX: 下一个包序号（所有响应都包含）

        Args:
            data: 响应数据

        Returns:
            响应字典
        """
        if len(data) < 8:
            return {'success': False, 'error': 'Response too short'}

        # 验证魔术字节
        if data[0] != self.MAGIC_BYTE:
            return {'success': False, 'error': 'Invalid magic byte'}

        # 提取状态码（位置 1）
        status_code = data[1]

        # 提取下一个包序号（位置 4-5，大端序）
        next_index = (data[4] << 8) | data[5]

        if status_code == self.STATUS_SUCCESS:
            return {
                'success': True,
                'status_code': status_code,
                'next_packet_index': next_index
            }

        elif status_code == self.STATUS_CHECKSUM_ERROR:
            return {
                'success': False,
                'status_code': status_code,
                'error': 'Checksum error'
            }

        elif status_code == self.STATUS_TIMEOUT_ERROR:
            return {
                'success': False,
                'status_code': status_code,
                'error': 'Timeout or other error'
            }

        else:
            return {
                'success': False,
                'status_code': status_code,
                'error': f'Unknown status code: {status_code}'
            }

    def _format_packet(self, packet: bytes, show_data: bool = False) -> str:
        """格式化数据包用于调试显示

        Args:
            packet: 数据包
            show_data: 是否显示数据部分（文件名包和取消包显示，数据包不显示）

        Returns:
            格式化的字符串
        """
        if len(packet) < 6:
            return f"[长度不足: {len(packet)}字节]"

        magic = packet[0]
        cmd = packet[1]
        length = (packet[2] << 8) | packet[3]
        index = (packet[4] << 8) | packet[5]
        checksum = (packet[-2] << 8) | packet[-1]

        cmd_name = {0x01: "文件名", 0x02: "数据", 0x03: "取消"}
        cmd_str = cmd_name.get(cmd, f"未知(0x{cmd:02X})")

        # 构建输出
        result = f"({len(packet)}字节)\n"

        # 显示十六进制数据
        if cmd == 0x02 and len(packet) > 100:
            # 数据包：只显示头部和尾部
            hex_header = ' '.join(f'{b:02X}' for b in packet[:6])
            hex_footer = ' '.join(f'{b:02X}' for b in packet[-2:])
            data_len = len(packet) - 8
            result += f"    HEX头: {hex_header} [... {data_len}字节数据 ...]\n"
            result += f"    HEX尾: {hex_footer}\n"
        else:
            # 短包：显示完整数据
            hex_str = ' '.join(f'{b:02X}' for b in packet)
            result += f"    HEX: {hex_str}\n"

        # 显示解析内容
        result += f"    解析: [魔术:0x{magic:02X}] [命令:{cmd_str}] [长度:{length}] [序号:{index}]"

        # 如果是文件名包，显示文件名
        if cmd == 0x01 and show_data and len(packet) > 8:
            filename = packet[6:-2].decode('utf-8', errors='ignore')
            result += f" [文件名:{filename}]"

        # 如果是数据包，显示数据长度
        if cmd == 0x02:
            data_len = len(packet) - 8
            result += f" [数据:{data_len}字节]"

        result += f" [校验:0x{checksum:04X}]"

        return result

    def _format_response(self, response: bytes) -> str:
        """格式化响应用于调试显示

        Args:
            response: 响应数据

        Returns:
            格式化的字符串
        """
        if len(response) < 8:
            return f"[长度不足: {len(response)}字节]"

        magic = response[0]
        status = response[1]
        length = (response[2] << 8) | response[3]
        next_index = (response[4] << 8) | response[5]
        checksum = (response[6] << 8) | response[7]

        status_name = {0x01: "成功", 0x02: "校验错误", 0x03: "超时/错误"}
        status_str = status_name.get(status, f"未知(0x{status:02X})")

        # 构建输出
        result = f"({len(response)}字节)\n"

        # 显示十六进制数据
        hex_str = ' '.join(f'{b:02X}' for b in response)
        result += f"    HEX: {hex_str}\n"

        # 显示解析内容
        result += f"    解析: [魔术:0x{magic:02X}] [状态:{status_str}] [长度:{length}] [下一包:{next_index}] [校验:0x{checksum:04X}]"

        return result

    def __enter__(self):
        """上下文管理器入口"""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器出口"""
        self.disconnect()


# 测试代码
if __name__ == '__main__':
    # 简单测试
    client = FileTransferClient('192.168.8.177', 8080)

    if client.connect():
        # 测试上传小文件
        test_data = b"Hello World"
        success = client.upload_file('test.txt', test_data)
        print(f"上传结果: {'成功' if success else '失败'}")

        client.disconnect()
