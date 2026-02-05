/**
 * @file FileTransferServer.cpp
 * @brief TCP Socket 文件传输服务器实现
 */

#include "../Grbl.h"

#if defined(ENABLE_WIFI) && defined(ENABLE_FILE_TRANSFER)

#    include "FileTransferServer.h"
#    include "WifiServices.h"
#    include "WebSettings.h"
#    include <WiFi.h>

// ============================================================================
// 配置宏定义
// ============================================================================

/**
 * @brief SD 卡错误处理模式
 *
 * 0 = 生产模式 (默认, 与 Jk_tcp 一致)
 *     - SD 卡不可用时: 不发送响应, 直接返回失败
 *     - 文件打开失败时: 不发送响应, 直接返回失败
 *     - 适用于: 生产环境, 确保数据完整性
 *
 * 1 = 测试模式 (用于无 SD 卡测试)
 *     - SD 卡不可用时: 跳过 SD 卡操作, 继续接收数据 (不写入)
 *     - 文件打开失败时: 跳过 SD 卡操作, 继续接收数据 (不写入)
 *     - 适用于: 协议测试, 网络测试, 无 SD 卡环境
 */
#ifndef FTS_SKIP_SD_ON_ERROR
#define FTS_SKIP_SD_ON_ERROR 1
#endif

// ============================================================================
// 日志宏定义
// ============================================================================

// 日志等级控制 (0=关闭, 1=Error, 2=Warning, 3=Info, 4=Debug)
#define FTS_LOG_LEVEL 3
#define FTS_LOG_FORMAT(letter, format)  "[" #letter "][%s:%u] " format "\r\n", __FUNCTION__, __LINE__

// 日志宏定义 (使用 ESP32 标准日志系统)
#if FTS_LOG_LEVEL >= 1
#    define FTS_ERROR(fmt, ...) grbl_sendf(CLIENT_SERIAL, FTS_LOG_FORMAT(E, fmt), ##__VA_ARGS__)
#else
#    define FTS_ERROR(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 2
#    define FTS_WARN(fmt, ...) grbl_sendf(CLIENT_SERIAL, FTS_LOG_FORMAT(W, fmt), ##__VA_ARGS__)
#else
#    define FTS_WARN(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 3
#    define FTS_INFO(fmt, ...) grbl_sendf(CLIENT_SERIAL, FTS_LOG_FORMAT(I, fmt), ##__VA_ARGS__)
#else
#    define FTS_INFO(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 4
#    define FTS_DEBUG(fmt, ...) grbl_sendf(CLIENT_SERIAL, FTS_LOG_FORMAT(D, fmt), ##__VA_ARGS__)
#else
#    define FTS_DEBUG(fmt, ...)
#endif

// ============================================================================
// FileTransferServer 实现
// ============================================================================

namespace WebUI {
    FileTransferServer file_transfer_server;

    // 静态成员定义
    bool        FileTransferServer::_setupdone      = false;
    uint16_t    FileTransferServer::_port           = 0;
    WiFiServer* FileTransferServer::_transferServer = NULL;
    WiFiClient  FileTransferServer::_transferClient;
    uint8_t     FileTransferServer::_streamBuffer[FileTransferServer::STREAM_BUFFER_SIZE];

    FileTransferServer::FileTransferServer() {
        resetState();
    }

    void FileTransferServer::resetState() {
        _state                  = TransferState::Idle;
        _packetState            = PacketReceiveState::WaitingHeader;
        _headerBytesReceived    = 0;
        _dataBytesReceived      = 0;
        _checksumBytesReceived  = 0;
        _dataLen                = 0;
        _currentPacketLen       = 0;
        _currentPacketIndex     = 0;
        _checksumAccum          = 0;
        _packetStartPos         = 0;
        _expectedPacketIndex    = 0;
        _lastActivityTime       = 0;
        _skipSDWrite            = false;
        _totalBytesWritten      = 0;
        _fileName[0]            = '\0';
    }

    bool FileTransferServer::begin() {
        end();

        // 从设置中读取端口号
        _port = file_transfer_port->get();

        // 重置状态变量
        resetState();

        // 创建服务器实例
        _transferServer = new WiFiServer(_port);
        if (_transferServer == nullptr) {
            FTS_ERROR("Failed to create file transfer server");
            return false;
        }
        _transferServer->setNoDelay(true);

        // 启动服务器
        _transferServer->begin();
        _setupdone = true;

        char msg[128];
        sprintf(msg, "[MSG:Server started on port %d (stream buffer: %d bytes)]\r\n",
                _port, STREAM_BUFFER_SIZE);
        grbl_send(CLIENT_SERIAL, msg);
        return true;
    }

    void FileTransferServer::end() {
        _setupdone = false;

        if (_state != TransferState::Idle) {
            cleanup();
        }

        if (_transferServer) {
            delete _transferServer;
            _transferServer = NULL;
        }
    }

    void FileTransferServer::handle() {
        if (!_setupdone || _transferServer == NULL) {
            return;
        }

        COMMANDS::wait(0);

        // 检查新客户端连接
        if (_transferServer->hasClient()) {
            WiFiClient newClient = _transferServer->available();
            if (newClient) {
                cleanup();
                _transferClient = newClient;
                _state = TransferState::ReceivingHeader;
                _packetState = PacketReceiveState::WaitingHeader;
                _headerBytesReceived = 0;
                _lastActivityTime = millis();
                IPAddress clientIP = _transferClient.remoteIP();
                char msg[128];
                sprintf(msg, "[MSG:FileTransfer Client(0) connected from %d.%d.%d.%d]\r\n",
                        clientIP[0], clientIP[1], clientIP[2], clientIP[3]);
                grbl_send(CLIENT_SERIAL, msg);
            }
        }

        // 处理现有客户端
        if (_transferClient && _transferClient.connected()) {
            handleClient();
        } else {
            // 客户端断开连接, 统一处理 (不区分状态)
            if (_state != TransferState::Idle) {
                grbl_send(CLIENT_SERIAL, "[MSG:FileTransfer Client(0) disconnected]\r\n");
                cleanup();
            }
        }
    }

    void FileTransferServer::handleClient() {
        // 1. 检查超时 (只判断一次) 
        if (_state != TransferState::Idle) {
            if (millis() - _lastActivityTime > TRANSFER_TIMEOUT_MS) {
                FTS_INFO("Timeout - transfer failed");
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                cleanup();
                return;
            }
        }

        // 2. 一次性读取可用数据 (最多 1 个 MTU 包) 
        int available = _transferClient.available();
        if (available > 0) {
            size_t toRead = min((size_t)available, STREAM_BUFFER_SIZE);
            int bytesRead = _transferClient.read(_streamBuffer, toRead);

            if (bytesRead > 0) {
                _lastActivityTime = millis();  // 更新活动时间 (只更新一次) 
                processStreamData(_streamBuffer, bytesRead);  // 处理这个 MTU 包
            }
        }
    }

    void FileTransferServer::processStreamData(uint8_t* buffer, size_t len) {
        size_t offset = 0;

        while (offset < len) {
            switch (_packetState) {
                case PacketReceiveState::WaitingHeader:
                    while (offset < len && buffer[offset] != MAGIC_BYTE) {
                        offset++;
                    }

                    if (offset < len) {
                        _headerBuffer[0] = MAGIC_BYTE;
                        _headerBytesReceived = 1;
                        _packetState = PacketReceiveState::ReceivingHeader;
                        offset++;
                    }
                    break;

                case PacketReceiveState::ReceivingHeader:
                    while (_headerBytesReceived < 6 && offset < len) {
                        _headerBuffer[_headerBytesReceived++] = buffer[offset++];
                    }

                    if (_headerBytesReceived >= 6) {
                        if (parseHeaderAndPrepareStream()) {
                            _packetState = PacketReceiveState::StreamingData;
                        } else {
                            _packetState = PacketReceiveState::WaitingHeader;
                        }
                    }
                    break;

                case PacketReceiveState::StreamingData: {
                    if (!handleStreamingData(buffer, len, offset)) {
                        return;
                    }
                    break;
                }

                case PacketReceiveState::VerifyingChecksum:
                    while (_checksumBytesReceived < 2 && offset < len) {
                        _checksumBuffer[_checksumBytesReceived++] = buffer[offset++];
                    }

                    if (_checksumBytesReceived >= 2) {
                        verifyChecksumAndRespond();
                    }
                    break;
            }

            COMMANDS::wait(0);
        }
    }

    bool FileTransferServer::parseHeaderAndPrepareStream() {
        uint8_t cmd = _headerBuffer[1];
        _currentPacketLen = (_headerBuffer[2] << 8) | _headerBuffer[3];

        // 验证包长度
        if (_currentPacketLen < MIN_PACKET_SIZE) {
            FTS_ERROR("Invalid packet length: %d", _currentPacketLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            return false;
        }

        // 特殊处理 Cancel 包 (6 字节, 无序号字段) 
        if (cmd == static_cast<uint8_t>(PacketType::Cancel)) {
            if (_currentPacketLen != 6) {
                FTS_ERROR("Invalid Cancel packet length: %d (expected 6)", _currentPacketLen);
                sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                return false;
            }
            // Cancel 包: 校验和只累加前 4 字节
            _checksumAccum = 0;
            for (int i = 0; i < 4; i++) {
                _checksumAccum += _headerBuffer[i];
            }
            _dataLen = 0;  // 无数据部分
            _dataBytesReceived = 0;

            // 验证状态
            if (_state != TransferState::ReceivingData) {
                FTS_ERROR("Cancel packet in wrong state: %d", (int)_state);
                sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                return false;
            }
            return true;
        }

        // 标准包处理 (Header 和 Data) 
        // 验证包长度必须 >= 8 (6字节头部 + 2字节校验和) 
        if (_currentPacketLen < 8) {
            FTS_ERROR("Invalid standard packet length: %d (minimum 8)", _currentPacketLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            return false;
        }

        _currentPacketIndex = (_headerBuffer[4] << 8) | _headerBuffer[5];

        // 校验和累加前 6 字节 (包含序号) 
        _checksumAccum = 0;
        for (int i = 0; i < 6; i++) {
            _checksumAccum += _headerBuffer[i];
        }

        _dataLen = _currentPacketLen - 8;
        _dataBytesReceived = 0;

        switch (_state) {
            case TransferState::ReceivingHeader:
                if (cmd == static_cast<uint8_t>(PacketType::Header)) {
                    if (_currentPacketIndex != 0) {
                        FTS_ERROR("Header packet index must be 0, got %d", _currentPacketIndex);
                        sendResponse(ResponseStatus::ProtocolError, 0);
                        return false;
                    }
                    return true;
                } else if (cmd == static_cast<uint8_t>(PacketType::Data)) {
                    FTS_ERROR("Data packet in wrong state: %d", (int)_state);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    return false;
                } else {
                    FTS_ERROR("Unknown command in ReceivingHeader: 0x%02X", cmd);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    return false;
                }

            case TransferState::ReceivingData:
                if (cmd == static_cast<uint8_t>(PacketType::Data)) {
                    if (_currentPacketIndex != _expectedPacketIndex) {
                        FTS_WARN("Packet index mismatch: expected %d, got %d", _expectedPacketIndex, _currentPacketIndex);
                        sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                        return false;
                    }
                    if (!_skipSDWrite && _uploadFile) {
                        _packetStartPos = _uploadFile.position();
                    }
                    return true;
                } else if (cmd == static_cast<uint8_t>(PacketType::Header)) {
                    FTS_ERROR("Header packet in wrong state: %d", (int)_state);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    return false;
                } else {
                    FTS_ERROR("Unknown command in ReceivingData: 0x%02X", cmd);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    return false;
                }

            default:
                FTS_ERROR("parseHeaderAndPrepareStream in unexpected state: %d", (int)_state);
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                return false;
        }
    }

    bool FileTransferServer::handleStreamingData(uint8_t* buffer, size_t len, size_t& offset) {
        size_t remaining = _dataLen - _dataBytesReceived;
        size_t toCopy = min(remaining, len - offset);

        // 累加校验和
        for (size_t i = 0; i < toCopy; i++) {
            _checksumAccum += buffer[offset + i];
        }

        uint8_t packetType = _headerBuffer[1];

        switch (_state) {
            case TransferState::ReceivingHeader:
                if (packetType == static_cast<uint8_t>(PacketType::Header)) {
                    // 累积文件名到 _fileName
                    if (_dataBytesReceived + toCopy <= sizeof(_fileName) - 1) {
                        memcpy(_fileName + _dataBytesReceived, buffer + offset, toCopy);
                    } else {
                        FTS_ERROR("Filename too long: %zu bytes", _dataBytesReceived + toCopy);
                        sendResponse(ResponseStatus::ProtocolError, 0);
                        _packetState = PacketReceiveState::WaitingHeader;
                        return false;
                    }
                } else {
                    // 不应该发生 (parseHeaderAndPrepareStream 已检查) 
                    FTS_ERROR("Unexpected packet type %d in ReceivingHeader state", packetType);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    _packetState = PacketReceiveState::WaitingHeader;
                    return false;
                }
                break;

            case TransferState::ReceivingData:
                if (packetType == static_cast<uint8_t>(PacketType::Data)) {
                    // 写入 SD 卡
                    if (!_skipSDWrite && _uploadFile) {
                        size_t written = _uploadFile.write(buffer + offset, toCopy);
                        if (written != toCopy) {
                            FTS_ERROR("SD write failed: wrote %zu/%zu bytes", written, toCopy);
                            // 回滚失败 = 中止传输
                            if (!_uploadFile.seek(_packetStartPos)) {
                                FTS_ERROR("seek rollback failed, aborting transfer");
                                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                                cleanup();
                                return false;
                            }
                            FTS_INFO("Rolled back to position %zu", _packetStartPos);
                            sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                            _packetState = PacketReceiveState::WaitingHeader;
                            return false;
                        }
                    }
                    _totalBytesWritten += toCopy;
                } else if (packetType == static_cast<uint8_t>(PacketType::Cancel)) {
                    // Cancel 包: 数据长度为 0, 无需处理
                } else {
                    // 不应该发生 (parseHeaderAndPrepareStream 已检查) 
                    FTS_ERROR("Unexpected packet type %d in ReceivingData state", packetType);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    _packetState = PacketReceiveState::WaitingHeader;
                    return false;
                }
                break;

            default:
                FTS_ERROR("StreamingData in unexpected state: %d", (int)_state);
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                _packetState = PacketReceiveState::WaitingHeader;
                return false;
        }

        _dataBytesReceived += toCopy;
        offset += toCopy;

        // 数据接收完成, 转到校验阶段
        if (_dataBytesReceived >= _dataLen) {
            _packetState = PacketReceiveState::VerifyingChecksum;
            _checksumBytesReceived = 0;
        }

        return true;
    }

    void FileTransferServer::verifyChecksumAndRespond() {
        uint16_t receivedChecksum = (_checksumBuffer[0] << 8) | _checksumBuffer[1];
        uint16_t calculatedChecksum = _checksumAccum & 0xFFFF;
        uint8_t packetType = _headerBuffer[1];

        // 校验和验证失败
        if (calculatedChecksum != receivedChecksum) {
            FTS_ERROR("Checksum error: expected 0x%04X, got 0x%04X", calculatedChecksum, receivedChecksum);

            switch (_state) {
                case TransferState::ReceivingHeader:
                    // Header 包校验失败: 保持 ReceivingHeader 状态
                    sendResponse(ResponseStatus::ProtocolError, 0);
                    break;

                case TransferState::ReceivingData:
                    if (packetType == static_cast<uint8_t>(PacketType::Data)) {
                        // Data 包校验失败: 回滚文件位置
                        if (!_skipSDWrite && _uploadFile) {
                            if (!_uploadFile.seek(_packetStartPos)) {
                                FTS_ERROR("seek rollback failed, aborting transfer");
                                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                                cleanup();
                                _packetState = PacketReceiveState::WaitingHeader;
                                return;
                            }
                            FTS_INFO("Rolled back to position %zu", _packetStartPos);
                        }
                        sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    } else if (packetType == static_cast<uint8_t>(PacketType::Cancel)) {
                        // Cancel 包校验失败: 忽略
                        FTS_WARN("Cancel packet checksum error, ignored");
                        sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    } else {
                        FTS_ERROR("Checksum error in unexpected packet type: %d", packetType);
                        sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                    }
                    break;

                default:
                    FTS_ERROR("Checksum error in unexpected state: %d", (int)_state);
                    sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                    break;
            }

            _packetState = PacketReceiveState::WaitingHeader;
            return;
        }

        // 校验成功, 根据状态和包类型进行处理
        switch (_state) {
            case TransferState::ReceivingHeader:
                if (packetType == static_cast<uint8_t>(PacketType::Header)) {
                    // Header 包校验成功: 处理文件名, 转换状态
                    if (processHeaderPacket(_dataBytesReceived)) {
                        // processHeaderPacket 内部会转换状态并发送响应
                    }
                } else {
                    FTS_ERROR("Unexpected packet type in ReceivingHeader: %d", packetType);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                }
                break;

            case TransferState::ReceivingData:
                if (packetType == static_cast<uint8_t>(PacketType::Data)) {
                    // Data 包校验成功: 递增序号
                    _expectedPacketIndex++;

                    if (_expectedPacketIndex % 10 == 0) {
                        FTS_INFO("Progress: packet %d received", _expectedPacketIndex);
                    }

                    sendResponse(ResponseStatus::Success, _expectedPacketIndex);
                } else if (packetType == static_cast<uint8_t>(PacketType::Cancel)) {
                    // Cancel 包校验成功: 清理状态, 转到 Idle
                    FTS_INFO("User cancelled transfer");
                    sendResponse(ResponseStatus::Success, 0);
                    cleanup();
                } else {
                    FTS_ERROR("Unexpected packet type in ReceivingData: %d", packetType);
                    sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                }
                break;

            default:
                FTS_ERROR("verifyChecksumAndRespond in unexpected state: %d", (int)_state);
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                break;
        }

        _packetState = PacketReceiveState::WaitingHeader;
    }

    bool FileTransferServer::processHeaderPacket(size_t nameLen) {
        if (nameLen >= sizeof(_fileName) - 1) {
            FTS_ERROR("Filename too long: %zu bytes", nameLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            return false;
        }

        // 规范化文件名: 去除前导斜杠, 然后添加单个斜杠
        // 直接操作 _fileName 成员变量

        // 1. 找到第一个非斜杠字符的位置
        size_t startPos = 0;
        while (startPos < nameLen && _fileName[startPos] == '/') {
            startPos++;
        }

        // 2. 计算实际文件名长度
        size_t actualLen = nameLen - startPos;
        if (startPos > 0 && actualLen > 0) {
            memmove(_fileName + 1, _fileName + startPos, actualLen);
        } else if (startPos == 0 && actualLen > 0) {
            memmove(_fileName + 1, _fileName, actualLen);
        }

        // 3. 添加前导斜杠并终止字符串
        _fileName[0] = '/';
        _fileName[actualLen + 1] = '\0';

        SDState sdState = get_sd_state(true);
        if (sdState != SDState::Idle) {
            if (sdState == SDState::NotPresent) {
                FTS_ERROR("No SD Card");
            } else {
                FTS_ERROR("SD Card Busy");
            }

#if FTS_SKIP_SD_ON_ERROR
            _skipSDWrite = true;
#else
            sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
            return false;
#endif
        }

        if (!_skipSDWrite) {
            _uploadFile = SD.open(_fileName, FILE_WRITE);
            if (!_uploadFile) {
                FTS_ERROR("Failed to create file: %s", _fileName);
#if FTS_SKIP_SD_ON_ERROR
                _skipSDWrite = true;
#else
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                return false;
#endif
            }

            if (!_skipSDWrite) {
                set_sd_state(SDState::BusyUploading);
            }
        }

        _state = TransferState::ReceivingData;
        _expectedPacketIndex = 1;
        _lastActivityTime = millis();

        FTS_INFO("Receiving file: %s", _fileName);
        sendResponse(ResponseStatus::Success, 1);
        return true;
    }

    void FileTransferServer::sendResponse(ResponseStatus status, uint16_t nextPacketIndex) {
        // 安全检查: 确保客户端有效且已连接
        if (!_transferClient || !_transferClient.connected()) {
            FTS_ERROR("sendResponse: client not connected");
            return;
        }

        // 响应格式 (适配 Dart 客户端) :
        // [0xAB][STATUS][LEN_H][LEN_L][IDX_H][IDX_L][CHK_H][CHK_L]
        uint8_t response[RESPONSE_PACKET_SIZE];
        response[0] = MAGIC_BYTE;             // 魔术字节
        response[1] = (uint8_t)status;        // 状态码 (1=成功, 2=校验错误, 3=超时)
        response[2] = 0x00;                   // 长度高字节
        response[3] = RESPONSE_PACKET_SIZE;   // 长度低字节 (固定 8 字节)
        response[4] = (nextPacketIndex >> 8) & 0xFF;  // 下一包序号高字节
        response[5] = nextPacketIndex & 0xFF;         // 下一包序号低字节

        // 计算校验和 (累加前 6 字节)
        uint32_t sum = 0;
        for (int i = 0; i < 6; i++) {
            sum += response[i];
        }
        response[6] = (sum >> 8) & 0xFF;  // 校验和高字节
        response[7] = sum & 0xFF;         // 校验和低字节

        // 发送响应并检查返回值
        size_t written = _transferClient.write(response, RESPONSE_PACKET_SIZE);
        if (written != RESPONSE_PACKET_SIZE) {
            FTS_ERROR("sendResponse: status=0x%02X, nextIdx=%d, write failed, wrote %d/%d bytes",
                (uint8_t)status, nextPacketIndex, written, RESPONSE_PACKET_SIZE);
        } else {
            FTS_DEBUG("sendResponse: status=0x%02X, nextIdx=%d, sent successfully",
                (uint8_t)status, nextPacketIndex);
        }
    }

    void FileTransferServer::cleanup() {
        // 1. 关闭文件并输出统计日志
        int packets = (int)_expectedPacketIndex - 1;
        if (_uploadFile) {
            _uploadFile.flush();
            size_t fileSize = _uploadFile.size();
            _uploadFile.close();
            FTS_INFO("File closed: %s, packets=%d, written=%zu, size=%zu",
                _fileName, packets, _totalBytesWritten, fileSize);
        } else if (_fileName[0] != '\0') {
            if (packets == -1) {
                _fileName[0] = '/';
                _fileName[1] = '\0';
            }
            FTS_INFO("File closed (no write): %s, packets=%d, written=%zu",
                _fileName, packets, _totalBytesWritten);
        }
        _uploadFile = File();

        // 2. 恢复 SD 卡状态 (只在未跳过写入时)
        if (!_skipSDWrite) {
            set_sd_state(SDState::Idle);
        }

        // 3. 不再主动断开客户端连接, 让客户端自己断开
        // 如果已经断开连接, 那 `_transferClient` 会直接判断为空, 也不知道是哪里赋值清空的
        if (_transferClient && _transferClient.connected()) {
            FTS_INFO("Client connection preserved");
        }

        // 4. 重置所有状态变量
        resetState();
    }

    FileTransferServer::~FileTransferServer() {
        end();
    }
}

#endif  // ENABLE_WIFI && ENABLE_FILE_TRANSFER
