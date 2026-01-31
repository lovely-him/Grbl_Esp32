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
// 日志宏定义 (编译时优化)
// ============================================================================

// 日志等级控制 (0=关闭, 1=Error, 2=Warning, 3=Info, 4=Debug)
#define FTS_LOG_LEVEL 3

// 日志标签
#define TAG_FTS "[FTS ]"

// 回车换行
#define CR_LF "\r\n"

// 获取毫秒时间戳
#define LOG_TIMESTAMP() (millis())

// 日志宏定义
#if FTS_LOG_LEVEL >= 1
#    define FTS_ERROR(fmt, ...) grbl_sendf(CLIENT_SERIAL, "[E][%6lu]%s " fmt CR_LF, LOG_TIMESTAMP(), TAG_FTS, ##__VA_ARGS__)
#else
#    define FTS_ERROR(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 2
#    define FTS_WARN(fmt, ...) grbl_sendf(CLIENT_SERIAL, "[W][%6lu]%s " fmt CR_LF, LOG_TIMESTAMP(), TAG_FTS, ##__VA_ARGS__)
#else
#    define FTS_WARN(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 3
#    define FTS_INFO(fmt, ...) grbl_sendf(CLIENT_SERIAL, "[I][%6lu]%s " fmt CR_LF, LOG_TIMESTAMP(), TAG_FTS, ##__VA_ARGS__)
#else
#    define FTS_INFO(fmt, ...)
#endif

#if FTS_LOG_LEVEL >= 4
#    define FTS_DEBUG(fmt, ...) grbl_sendf(CLIENT_SERIAL, "[D][%6lu]%s " fmt CR_LF, LOG_TIMESTAMP(), TAG_FTS, ##__VA_ARGS__)
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

    FileTransferServer::FileTransferServer() {
        _rxBuffer            = nullptr;  // 延迟分配，在 begin() 中分配
        _state               = TransferState::Idle;
        _rxBufferSize        = 0;
        _expectedPacketIndex = 0;
        _lastActivityTime    = 0;
        _skipSDWrite         = false;
    }

    bool FileTransferServer::begin() {
        end();

        // 分配接收缓冲区 (延迟分配，避免在静态初始化阶段分配)
        if (!_rxBuffer) {
            _rxBuffer = new uint8_t[RXBUFFERSIZE];
            if (!_rxBuffer) {
                FTS_ERROR("Failed to allocate RX buffer (%d bytes)", RXBUFFERSIZE);
                return false;
            }
            FTS_INFO("RX buffer allocated: %d bytes", RXBUFFERSIZE);
        }

        // 从设置中读取端口号
        _port = file_transfer_port->get();

        // 重置状态变量
        _state               = TransferState::Idle;
        _rxBufferSize        = 0;
        _expectedPacketIndex = 0;
        _lastActivityTime    = 0;
        _skipSDWrite         = false;

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

        FTS_INFO("Server started on port %d", _port);
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

        _rxBufferSize = 0;
    }

    void FileTransferServer::handle() {
        if (!_setupdone || _transferServer == NULL) {
            return;
        }

        COMMANDS::wait(0);

        // 检查新客户端连接
        if (_transferServer->hasClient()) {
            WiFiClient newClient = _transferServer->available();

            // 已有客户端连接, 断开旧客户端, 接受新客户端
            if (_transferClient && _transferClient.connected()) {
                FTS_WARN("New client connecting, disconnecting old client");
                cleanup();
            }

            // 接受新客户端
            _transferClient = newClient;
            _state = TransferState::ReceivingHeader;
            _rxBufferSize = 0;
            _lastActivityTime = millis();
            FTS_INFO("Client connected");
        }

        // 处理现有客户端
        if (_transferClient && _transferClient.connected()) {
            handleClient();
        } else {
            // 客户端断开连接, 统一处理 (不区分状态)
            if (_state != TransferState::Idle) {
                cleanup();
            }
        }
    }

    void FileTransferServer::handleClient() {
        // 安全检查: 确保缓冲区已分配
        if (!_rxBuffer) {
            FTS_ERROR("RX buffer not allocated");
            cleanup();
            return;
        }

        // 检查超时
        if (_state != TransferState::Idle) {
            if (millis() - _lastActivityTime > TRANSFER_TIMEOUT_MS) {
                // 服务器超时 = 传输失败 (无论什么状态)
                FTS_INFO("Timeout - transfer failed");
                sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                cleanup();
                return;
            }
        }

        // 读取可用数据
        int available = _transferClient.available();
        if (available > 0) {
            if (RXBUFFERSIZE <= _rxBufferSize || RXBUFFERSIZE <= (_rxBufferSize + available)) {
                FTS_ERROR("RX buffer overflow (%d,%d,%d bytes)", RXBUFFERSIZE, _rxBufferSize, available);
                _rxBufferSize = 0;
            }

            int readSize = min(available, (int)(RXBUFFERSIZE - _rxBufferSize));
            if (readSize > 0) {
                int bytesRead = _transferClient.read(_rxBuffer + _rxBufferSize, readSize);
                if (bytesRead > 0) {
                    _rxBufferSize += (size_t)bytesRead;
                    _lastActivityTime = millis();
                }
            }

            // 解析和处理数据包
            while (parsePacket()) {
                COMMANDS::wait(0); // 让出 CPU 给实时任务
            }
        }
    }

    bool FileTransferServer::parsePacket() {
        // 至少需要 6 字节: [0xAB][CMD][LEN_H][LEN_L][...][CHK_H][CHK_L]
        if (_rxBufferSize < MIN_PACKET_SIZE) {
            return false;
        }

        // 验证魔术字节
        if (_rxBuffer[0] != MAGIC_BYTE) {
            // 帧头错误, 直接丢弃整个缓冲区, 等待下一个完整帧
            FTS_WARN("Invalid magic byte: 0x%02X, discarding %zu bytes", _rxBuffer[0], _rxBufferSize);
            _rxBufferSize = 0;
            return false;
        }

        // 提取包长度 (大端序)
        uint16_t packetLen = (_rxBuffer[2] << 8) | _rxBuffer[3];

        // 验证包长度范围
        if (packetLen < MIN_PACKET_SIZE || packetLen >= RXBUFFERSIZE) {
            FTS_ERROR("Invalid packet length: %d", packetLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            cleanup();
            return false;
        }

        // 检查完整包是否可用
        if (_rxBufferSize < packetLen) {
            // FTS_DEBUG("Incomplete packet: need %d, have %d", packetLen, _rxBufferSize);
            return false;
        }

        // 验证校验和
        if (!verifyChecksum(_rxBuffer, packetLen)) {
            FTS_ERROR("Checksum error: cmd=0x%02X, len=%d", _rxBuffer[1], packetLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            cleanup();
            return false;
        }

        // 根据命令类型和当前状态处理
        uint8_t cmd = _rxBuffer[1];
        bool shouldContinue = true;
        FTS_DEBUG("Packet: cmd=0x%02X, len=%d, bufSize=%zu", cmd, packetLen, _rxBufferSize);

        switch (cmd) {
            case static_cast<uint8_t>(PacketType::Header): {  // Header Packet
                // 只在 ReceivingHeader 状态接受
                if (_state != TransferState::ReceivingHeader) {
                    FTS_ERROR("Header packet in wrong state: %d", (int)_state);
                    sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                } else {
                    shouldContinue = processHeaderPacket(_rxBuffer, packetLen);
                }
            }
            break;
            case static_cast<uint8_t>(PacketType::Data): {  // Data Packet
                // 只在 ReceivingData 状态接受
                if (_state != TransferState::ReceivingData) {
                    FTS_ERROR("Data packet in wrong state: %d", (int)_state);
                    sendResponse(ResponseStatus::ServerError, _expectedPacketIndex);
                } else {
                    shouldContinue = processDataPacket(_rxBuffer, packetLen);
                }
            }
            break;
            case static_cast<uint8_t>(PacketType::Cancel): {  // Cancel Packet
                // 任何非 Idle 状态都可以取消, 不发送响应, 直接清理并断开连接
                if (_state == TransferState::Idle) {
                    FTS_WARN("Cancel packet in Idle state, ignoring");
                } else {
                    FTS_INFO("User cancelled transfer");
                    shouldContinue = false;
                }
            }
            break;
            default: {
                // 未知命令 - 发送错误响应, 丢弃这个包, 继续等待
                FTS_ERROR("Unknown command: 0x%02X", cmd);
                sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            }
            break;
        }

        // 如果处理失败, 统一调用 cleanup() 并退出
        if (!shouldContinue) {
            cleanup();
            return false;
        }

        // 移除已处理的包 (只有在处理成功时才执行)
        if (_rxBufferSize > packetLen) {
            memmove(_rxBuffer, _rxBuffer + packetLen, _rxBufferSize - packetLen);
            _rxBufferSize -= packetLen;
        } else {
            _rxBufferSize = 0;
        }

        return true;
    }

    bool FileTransferServer::processHeaderPacket(const uint8_t* data, uint16_t len) {
        // 提取文件名 (UTF-8)
        // 包格式: [0xAB][0x01][LEN_H][LEN_L][IDX_H][IDX_L][filename][CHK_H][CHK_L]
        size_t nameLen = len - 8;  // 排除头部(6) + 校验和(2)

        // 验证文件名长度 (预留 '/' 前缀和 null 终止符)
        if (nameLen >= sizeof(_fileName) - 1) {
            FTS_ERROR("Filename too long: %zu bytes", nameLen);
            sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
            return false;
        }

        // 确保文件名以 '/' 开头 (直接使用指针, 避免临时变量)
        const char* srcFileName = (const char*)(data + 6);
        if (srcFileName[0] == '/') {
            memcpy(_fileName, srcFileName, nameLen);
            _fileName[nameLen] = '\0';
        } else {
            _fileName[0] = '/';
            memcpy(_fileName + 1, srcFileName, nameLen);
            _fileName[nameLen + 1] = '\0';
        }

        // 检查 SD 卡状态
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

        // 如果未跳过写入, 则创建文件
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
                set_sd_state(SDState::Busy);
            }
        }

        _state               = TransferState::ReceivingData;
        _expectedPacketIndex = 1;
        _lastActivityTime    = millis();

        FTS_INFO("Receiving file: %s", _fileName);
        sendResponse(ResponseStatus::Success, 1);  // 请求第一个数据包
        return true;
    }

    bool FileTransferServer::processDataPacket(const uint8_t* data, uint16_t len) {
        // 提取包序号
        uint16_t packetIndex = (data[4] << 8) | data[5];

        // 验证序号
        if (packetIndex != _expectedPacketIndex) {
            // 序号不匹配, 请求正确的包, 丢弃当前包
            FTS_WARN("Packet index mismatch: expected %d, got %d", _expectedPacketIndex, packetIndex);
            sendResponse(ResponseStatus::Success, _expectedPacketIndex);
            return true;
        }

        // 提取数据内容
        // 包格式: [0xAB][0x02][LEN_H][LEN_L][IDX_H][IDX_L][data][CHK_H][CHK_L]
        size_t         dataLen = len - 8;  // 排除头部(6) + 校验和(2)
        const uint8_t* payload = data + 6;

        // 如果未跳过写入, 则写入 SD 卡
        if (!_skipSDWrite) {
            // 安全检查: 确保文件已打开
            if (!_uploadFile) {
                FTS_ERROR("File not open");
                sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                return false;
            }

            // 写入 SD 卡 (流式写入)
            size_t written = _uploadFile.write(payload, dataLen);
            if (written != dataLen) {
                FTS_ERROR("SD write failed: wrote %zu/%zu bytes", written, dataLen);
                sendResponse(ResponseStatus::ProtocolError, _expectedPacketIndex);
                return false;
            }
        }

        _expectedPacketIndex++;
        _lastActivityTime = millis();

        // 添加进度日志 (每10个包)
        if (_expectedPacketIndex % 10 == 0) {
            FTS_INFO("Progress: packet %d received", _expectedPacketIndex);
        }

        // 发送 ACK, 请求下一个包
        sendResponse(ResponseStatus::Success, _expectedPacketIndex);
        return true;
    }

    void FileTransferServer::sendResponse(ResponseStatus status, uint16_t nextPacketIndex) {
        // 安全检查: 确保客户端有效
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

    bool FileTransferServer::verifyChecksum(const uint8_t* data, uint16_t len) {
        // 计算校验和 (累加除了最后 2 字节之外的所有字节)
        uint32_t sum = 0;
        for (uint16_t i = 0; i < len - 2; i++) {
            sum += data[i];
        }
        uint16_t calculated = (uint16_t)(sum & 0xFFFF);

        // 提取接收到的校验和
        uint16_t received = (data[len - 2] << 8) | data[len - 1];

        return calculated == received;
    }

    void FileTransferServer::cleanup() {
        // 1. 关闭文件 (如果已打开)
        if (_uploadFile) {
            _uploadFile.close();
        }
        _uploadFile = File();  // 重置为空 File 对象

        // 2. 恢复 SD 卡状态 (只在未跳过写入时)
        if (!_skipSDWrite) {
            set_sd_state(SDState::Idle);
        }

        // 3. 断开客户端连接 (如果已连接)
        if (_transferClient && _transferClient.connected()) {
            _transferClient.flush();
            _transferClient.stop();
            FTS_INFO("Client disconnected Initiate");
        } else {
            FTS_INFO("Client already disconnected");
        }

        // 4. 重置接收缓冲区长度 (缓冲区本身是静态的, 不需要释放)
        _rxBufferSize = 0;

        // 5. 重置所有状态变量
        _state               = TransferState::Idle;
        _fileName[0]         = '\0';
        _expectedPacketIndex = 0;
        _lastActivityTime    = 0;
        _skipSDWrite         = false;
    }

    FileTransferServer::~FileTransferServer() {
        end();

        // 释放接收缓冲区
        if (_rxBuffer) {
            delete[] _rxBuffer;
            _rxBuffer = nullptr;
        }
    }
}

#endif  // ENABLE_WIFI && ENABLE_FILE_TRANSFER
