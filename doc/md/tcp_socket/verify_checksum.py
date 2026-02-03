#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TCP Socket 文件传输协议 - 校验和验证工具
"""

def calculate_checksum(data_hex):
    """计算校验和

    Args:
        data_hex: 十六进制字符串, 例如 "AB 01 00 11 00 00 74 65 73 74 2E 74 78 74"

    Returns:
        校验和的十六进制字符串, 例如 "02 2D"
    """
    bytes_list = [int(b, 16) for b in data_hex.split()]
    checksum = sum(bytes_list) & 0xFFFF
    return f"{(checksum >> 8):02X} {(checksum & 0xFF):02X}"

def verify_packet(packet_hex):
    """验证数据包的校验和

    Args:
        packet_hex: 完整数据包的十六进制字符串 (包含校验和) 

    Returns:
        (is_valid, calculated_checksum, received_checksum)
    """
    bytes_list = [int(b, 16) for b in packet_hex.split()]

    # 提取数据部分 (除了最后2字节) 
    data_bytes = bytes_list[:-2]

    # 计算校验和
    calculated = sum(data_bytes) & 0xFFFF

    # 提取接收到的校验和
    received = (bytes_list[-2] << 8) | bytes_list[-1]

    is_valid = calculated == received

    return is_valid, f"{(calculated >> 8):02X} {(calculated & 0xFF):02X}", f"{(received >> 8):02X} {(received & 0xFF):02X}"

def main():
    print("=" * 70)
    print("TCP Socket 文件传输协议 - 校验和验证")
    print("=" * 70)
    print()

    # 测试 1: 文件信息包 (test.txt)
    print(" [测试 1] 文件信息包 - 文件名: test.txt")
    print("-" * 70)

    # 文件名 "test.txt" 的 ASCII 编码
    filename = "test.txt"
    filename_hex = ' '.join(f'{ord(c):02X}' for c in filename)
    print(f"文件名: {filename}")
    print(f"文件名编码: {filename_hex}")
    print()

    # 构建数据包 (不含校验和) 
    data = f"AB 01 00 11 00 00 {filename_hex}"
    print(f"数据部分: {data}")

    # 计算校验和
    checksum = calculate_checksum(data)
    print(f"计算的校验和: {checksum}")

    # 完整数据包
    full_packet = f"{data} {checksum}"
    print(f"完整数据包: {full_packet}")
    print()

    # 验证
    is_valid, calc, recv = verify_packet(full_packet)
    print(f"验证结果: {'[OK]' if is_valid else '[ERROR]'}")
    print()

    # 测试 2: 数据包 1 (Hello)
    print(" [测试 2] 数据包 1 - 内容: Hello")
    print("-" * 70)

    content = "Hello"
    content_hex = ' '.join(f'{ord(c):02X}' for c in content)
    print(f"内容: {content}")
    print(f"内容编码: {content_hex}")
    print()

    data2 = f"AB 02 00 0D 00 01 {content_hex}"
    print(f"数据部分: {data2}")

    checksum2 = calculate_checksum(data2)
    print(f"计算的校验和: {checksum2}")

    full_packet2 = f"{data2} {checksum2}"
    print(f"完整数据包: {full_packet2}")
    print()

    is_valid2, calc2, recv2 = verify_packet(full_packet2)
    print(f"验证结果: {'[OK]' if is_valid2 else '[ERROR]'}")
    print()

    # 测试 3: 数据包 2 ( World)
    print(" [测试 3] 数据包 2 - 内容:  World")
    print("-" * 70)

    content3 = " World"
    content_hex3 = ' '.join(f'{ord(c):02X}' for c in content3)
    print(f"内容: '{content3}'")
    print(f"内容编码: {content_hex3}")
    print()

    data3 = f"AB 02 00 0E 00 02 {content_hex3}"
    print(f"数据部分: {data3}")

    checksum3 = calculate_checksum(data3)
    print(f"计算的校验和: {checksum3}")

    full_packet3 = f"{data3} {checksum3}"
    print(f"完整数据包: {full_packet3}")
    print()

    is_valid3, calc3, recv3 = verify_packet(full_packet3)
    print(f"验证结果: {'[OK]' if is_valid3 else '[ERROR]'}")
    print()

    # 测试 4: 取消包
    print(" [测试 4] 取消包")
    print("-" * 70)

    data4 = "AB 03 00 06"
    print(f"数据部分: {data4}")

    checksum4 = calculate_checksum(data4)
    print(f"计算的校验和: {checksum4}")

    full_packet4 = f"{data4} {checksum4}"
    print(f"完整数据包: {full_packet4}")
    print()

    is_valid4, calc4, recv4 = verify_packet(full_packet4)
    print(f"验证结果: {'[OK]' if is_valid4 else '[ERROR]'}")
    print()

    # 测试用户发送的数据包
    print(" [测试 5] 验证用户发送的数据包")
    print("-" * 70)

    user_packet = "AB 01 00 11 00 00 74 65 73 74 2E 74 78 74 02 2D"
    print(f"用户发送的数据包: {user_packet}")

    is_valid5, calc5, recv5 = verify_packet(user_packet)
    print(f"计算的校验和: {calc5}")
    print(f"接收的校验和: {recv5}")
    print(f"验证结果: {'[OK]' if is_valid5 else '[ERROR]'}")

    if not is_valid5:
        print()
        print("WARNING: 校验和不匹配! ")
        print(f"   应该使用: {calc5}")
        print(f"   实际使用: {recv5}")
        print()
        print(f"正确的数据包应该是:")
        correct_packet = user_packet.rsplit(' ', 2)[0] + f" {calc5}"
        print(f"   {correct_packet}")

    print()
    print("=" * 70)
    print("测试完成")
    print("=" * 70)

if __name__ == "__main__":
    main()
