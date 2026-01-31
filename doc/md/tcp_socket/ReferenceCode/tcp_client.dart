//
//  tcp_client.dart
//  flutter_common_utils
//
//  Created by lynch.zeng on 2024-11-22.
//  Copyright © 2024 尚睿科技股份有限公司. All rights reserved.
//

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/foundation.dart';

class TCPClient {
  Socket? _socket;
  List<Uint8List> _packetList = [];
  int _currentPacketIndex = 0;

  /// 已发送超时定时器
  Timer? _sentTimeoutTimer;

  /// 进度回调
  Function(int progress)? _onProgress;

  /// 传输错误
  Function(int errorCode)? _onError;

  /// 连接服务器
  /// [host] 地址
  /// [port] 端口
  /// [timeout] 连接超时时间，默认 3 秒
  Future<void> connect({
    required String host,
    required int port,
    Duration timeout = const Duration(seconds: 3),
    Function? onError,
  }) async {
    try {
      _socket = await Socket.connect(
        host,
        port,
        timeout: timeout,
      );
      _socket?.listen(
        _onDataReceived,
        onError: onError,
        onDone: _onDoneReceived,
      );
    } on SocketException catch (e) {
      disconnect();
      onError?.call(500);
    }
  }

  // 断开连接
  void disconnect() {
    if (_socket == null) return;
    _socket?.destroy();
    _socket = null;
    _currentPacketIndex = 0;
    _packetList.clear();
    _cancelSentTimeoutTimer();
    debugPrint('Disconnected from server');
  }

  /// 接收数据
  void _onDataReceived(Uint8List data) {
    debugPrint('$data');
    // 解析返回的包序号
    int nextPacketIndex = _parseResponse(data);

    // 判断序号是否有问题
    // 有问题会返回当前包序号
    // 没有问题会返回要接收的下一个包的序号
    if (nextPacketIndex == _currentPacketIndex) return;

    if (_onProgress != null) {
      // 更新接收进度
      int progress = ((nextPacketIndex) / _packetList.length * 100).toInt();
      debugPrint(
          'Received packet $_currentPacketIndex, Next packet $nextPacketIndex, Progress: $progress%');
      _onProgress!(progress);
    }

    if (nextPacketIndex < _packetList.length) {
      _currentPacketIndex = nextPacketIndex; // 更新当前序号包
      _sendPacket(nextPacketIndex); // 发送下一个包
    }
  }

  /// 解析服务器响应
  int _parseResponse(Uint8List data) {
    int stateCode = data[1];
    switch (stateCode) {
      case 1:
        if (data.length >= 6) return (data[4] << 8) | data[5];
        break;
      case 2:
        if (_onError != null) _onError!(stateCode);
        break;
      case 3:
        if (_onError != null) _onError!(stateCode);
        break;
      default:
    }

    return _currentPacketIndex;
  }

  /// 处理连接断开
  void _onDoneReceived() {
    debugPrint('_onDoneReceived：Connection closed by server');
  }

  /// 上传文件
  ///
  /// 发送数据包的原理：
  /// 先连接上设备，然后调用该方法传进来 [fileName] 文件名、要发送的 [fileData] 字节数据、缓存区大小，发送进度
  /// 先把字节数据分成一个一个的数据包，保存到 [_packetList] 里，每个包最大的容量 1024 * 32 字节
  /// 然后发送第一个数据包，下位机收到数据包后会校验完整性
  /// 没有问题会返回要接受的下一个包的序号
  /// 有问题会返回当前包序号。
  /// 上位机收到后再发送下位机需要的数据包，如果返回的包超过包个数，说明已经发送完成了
  ///
  /// [fileName] 文件名
  /// [fileData] 文件数据
  /// [bufferSize] 缓冲区大小
  /// [onProgress] 已接收进度回调
  /// [onError] 接收过程出现错误回调
  /// 接收过程出现错误 2 是校验错误 3 是超时错误，收到错误信号就要终止发送数据
  void uploadFile({
    required String fileName,
    required Uint8List fileData,
    Function(int progress)? onProgress,
    Function(int errorCode)? onError,
  }) {
    _onProgress = onProgress;
    _onError = onError;
    _packetList = _createPacketList(fileName, fileData);
    _currentPacketIndex = 0;
    _sendPacket(_currentPacketIndex);
  }

  // 发送取消指令
  void sendCancelCommand() {
    final cancelPacket = _createCancelCommandPacket();
    _socket?.add(cancelPacket);
    // disconnect();
  }

  // 发送单个数据包
  Future<void> _sendPacket(int packetIndex) async {
    if (packetIndex < _packetList.length) {
      _socket?.add(_packetList[packetIndex]);
      _runSentTimeoutTimer(packetIndex);
    } else {
      debugPrint('All packets sent');
    }
  }

  // 创建分包
  List<Uint8List> _createPacketList(
    String fileName,
    Uint8List fileData,
  ) {
    List<Uint8List> packetList = [];

    // 构造第一个包（文件信息）
    final headerPacket = _createHeaderPacket(fileName);
    packetList.add(headerPacket);
    // 每个包的最大的容量 32kb
    int packetSize = 1024 * 32;
    // 将文件数据分割为多个包
    for (int i = 0; i < fileData.length; i += packetSize) {
      int end =
          (i + packetSize > fileData.length) ? fileData.length : i + packetSize;
      final dataChunk = fileData.sublist(i, end);
      final dataPacket = _createDataPacket(packetList.length, dataChunk);
      packetList.add(dataPacket);
    }

    return packetList;
  }

  /// 创建第一个包（文件信息包）
  Uint8List _createHeaderPacket(String fileName) {
    // final fileNameBytes = fileName.codeUnits;
    final fileNameBytes = Utf8Encoder().convert(fileName);
    final buffer = BytesBuilder();
    buffer.add([0xAB, 0x01]); // 包头与命令码
    buffer.add(_intToBytes(8 + fileNameBytes.length, 2)); // 包长度
    buffer.add(_intToBytes(0, 2)); // 包序号
    buffer.add(fileNameBytes); // 文件名
    buffer.add([0x00, 0x00]); // 校验占位
    return _addChecksum(buffer.toBytes());
  }

  /// 创建数据包
  Uint8List _createDataPacket(int packetIndex, Uint8List dataChunk) {
    final buffer = BytesBuilder();
    buffer.add([0xAB, 0x02]); // 包头与命令码
    buffer.add(_intToBytes(8 + dataChunk.length, 2)); // 包长度
    buffer.add(_intToBytes(packetIndex, 2)); // 包序号
    buffer.add(dataChunk); // 数据内容
    buffer.add([0x00, 0x00]); // 校验占位
    return _addChecksum(buffer.toBytes());
  }

  /// 创建取消指令包
  Uint8List _createCancelCommandPacket() {
    final buffer = BytesBuilder();
    buffer.add([0xAB, 0x03]); // 包头与命令码
    buffer.add(_intToBytes(6, 2)); // 包长度
    buffer.add([0x00, 0x00]); // 校验占位
    return _addChecksum(buffer.toBytes());
  }

  /// 添加校验码
  Uint8List _addChecksum(Uint8List data) {
    int checksum = data.take(data.length - 2).reduce((a, b) => a + b);
    data[data.length - 2] = (checksum >> 8) & 0xFF; // high
    data[data.length - 1] = checksum & 0xFF; // low
    return data;
  }

  /// 转换整数为指定字节数的 Uint8List
  Uint8List _intToBytes(int value, int length) {
    final bytes = Uint8List(length);
    for (int i = 0; i < length; i++) {
      bytes[length - i - 1] = (value >> (8 * i)) & 0xFF;
    }
    return bytes;
  }

  /// 运行已发送超时定时器
  void _runSentTimeoutTimer(int packetIndex) {
    _cancelSentTimeoutTimer();
    _sentTimeoutTimer = Timer.periodic(
      const Duration(seconds: 5),
      (timer) async {
        if (packetIndex == _currentPacketIndex) {
          _cancelSentTimeoutTimer();
          if (_onError != null) _onError!(404);
        }
      },
    );
  }

  /// 取消已发送超时定时器
  void _cancelSentTimeoutTimer() {
    if (_sentTimeoutTimer == null) return;
    _sentTimeoutTimer?.cancel();
    _sentTimeoutTimer = null;
  }
}
