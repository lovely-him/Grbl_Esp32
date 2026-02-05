/**
 * @file FileTransferServer.h
 * @brief TCP Socket 文件传输服务器
 *
 * 提供基于 TCP Socket 的文件传输功能, 支持通过 WiFi 将文件上传到 SD 卡.
 * 协议采用自定义二进制格式, 包含文件名包, 数据包和取消包三种类型.
 *
 * 主要特性:
 * - 支持最大 32KB 的单包数据传输
 * - 16 位校验和验证
 * - 包序号验证和重传机制
 * - 堆内存分配 (避免 BSS 段溢出)
 * - 错误自动断开连接, 确保状态一致性
 *
 * @author 建豪 (JianHao)
 * @date 2026-01-29
 * @version 1.2
 * @copyright Copyright (c) 2024 Grbl_Esp32 Contributors
 */

#pragma once

#include "../Config.h"

class WiFiServer;
class WiFiClient;

namespace WebUI {
    /**
     * @brief TCP Socket 文件传输服务器类
     *
     * 实现基于 TCP 的文件传输协议, 支持将文件上传到 SD 卡.
     * 采用状态机设计, 确保传输过程的可靠性和一致性.
     */
    class FileTransferServer {
    public:
        FileTransferServer();
        ~FileTransferServer();

        /**
         * @brief 启动文件传输服务器
         *
         * 创建 TCP 服务器实例并开始监听指定端口.
         *
         * @return true 启动成功, false 启动失败
         */
        bool begin();

        /**
         * @brief 停止文件传输服务器
         *
         * 关闭服务器, 断开所有客户端连接, 释放所有资源.
         */
        void end();

        /**
         * @brief 处理客户端连接和数据接收
         *
         * 在主循环中周期性调用此函数, 处理客户端连接, 数据接收和协议解析.
         *
         * @note 必须在主循环中频繁调用以保证实时性
         */
        void handle();

        /**
         * @brief 获取服务器端口号
         *
         * @return 服务器监听的端口号
         */
        static uint16_t port() { return _port; }

        /**
         * @brief 检查是否正在传输文件
         *
         * @return true 正在传输文件, false 空闲状态
         */
        bool isTransferring() const { return _state != TransferState::Idle; }

    private:
        // 服务器管理
        static bool        _setupdone;          /**< 服务器是否已启动 */
        static uint16_t    _port;               /**< 服务器端口号 */
        static WiFiServer* _transferServer;     /**< WiFi 服务器实例 */
        static WiFiClient  _transferClient;     /**< 当前连接的客户端 */

        /**
         * @brief 协议常量定义
         */
        static constexpr uint8_t  MAGIC_BYTE            = 0xAB;    /**< 魔术字节 */
        static constexpr uint8_t  RESPONSE_PACKET_SIZE  = 8;       /**< 响应包固定长度 */
        static constexpr uint8_t  MIN_PACKET_SIZE       = 6;       /**< 最小包长度 */
        static constexpr uint16_t TRANSFER_TIMEOUT_MS   = 10000;   /**< 超时时间 (毫秒) */
        static constexpr size_t   STREAM_BUFFER_SIZE    = 1536;    /**< 流式缓冲区大小 (1.5KB, 足够 1 个 MTU) */

        // 流式接收缓冲区 (只需存储 1 个 MTU 的数据)
        static uint8_t _streamBuffer[STREAM_BUFFER_SIZE];  /**< 流式数据缓冲区 */

        /**
         * @brief 数据包类型枚举
         */
        enum class PacketType : uint8_t {
            Header = 0x01,  /**< 文件名包 */
            Data   = 0x02,  /**< 数据包 */
            Cancel = 0x03   /**< 取消包 */
        };

        /**
         * @brief 传输状态枚举
         */
        enum class TransferState : uint8_t {
            Idle,            /**< 空闲状态, 等待客户端连接 */
            ReceivingHeader, /**< 接收文件名包 */
            ReceivingData,   /**< 接收数据包 */
            Error            /**< 错误状态 (保留, 未使用) */
        };
        TransferState _state;  /**< 当前传输状态 */

        /**
         * @brief 包接收状态机枚举 (流式处理)
         */
        enum class PacketReceiveState : uint8_t {
            WaitingHeader,      /**< 等待帧头 (0xAB) */
            ReceivingHeader,    /**< 接收包头部 (6 字节) */
            StreamingData,      /**< 流式接收并写入数据 */
            VerifyingChecksum   /**< 验证校验和 (2 字节) */
        };
        PacketReceiveState _packetState;  /**< 当前包接收状态 */

        /**
         * @brief 响应状态码枚举
         */
        enum class ResponseStatus : uint8_t {
            Success       = 0x01,  /**< 成功, 请求下一个包 */
            ProtocolError = 0x02,  /**< 协议错误 (校验, 格式, 命令等客户端问题) */
            ServerError   = 0x03   /**< 服务器错误 (超时, SD卡, 状态机等服务器问题) */
        };

        // 流式接收状态变量
        uint8_t  _headerBuffer[6];              /**< 帧头缓冲区 (6 字节) */
        size_t   _headerBytesReceived;          /**< 已接收的帧头字节数 */
        uint16_t _currentPacketLen;             /**< 当前包总长度 */
        uint16_t _currentPacketIndex;           /**< 当前包序号 */
        size_t   _dataBytesReceived;            /**< 已接收的数据字节数 */
        size_t   _dataLen;                      /**< 数据部分长度 (不含头部和校验和) */
        uint32_t _checksumAccum;                /**< 累加校验和 */
        uint8_t  _checksumBuffer[2];            /**< 校验和缓冲区 (2 字节) */
        size_t   _checksumBytesReceived;        /**< 已接收的校验和字节数 */
        size_t   _packetStartPos;               /**< 包起始文件位置 (用于回滚) */

        // 文件传输状态
        File     _uploadFile;           /**< 当前上传的文件对象 */
        char     _fileName[256];        /**< 当前文件名 (固定大小, 避免堆碎片) */
        uint16_t _expectedPacketIndex;  /**< 期望的下一个数据包序号 */
        uint32_t _lastActivityTime;     /**< 最后一次活动时间 (毫秒) */
        bool     _skipSDWrite;          /**< 跳过 SD 卡写入 (SD 卡不可用时) */
        size_t   _totalBytesWritten;    /**< 累计理论写入字节数 (接收到的数据量) */

        // 协议处理方法

        /**
         * @brief 处理已连接客户端的数据接收和超时检查
         *
         * 检查超时, 读取可用数据, 解析数据包.
         * 在 handle() 中被调用.
         */
        void handleClient();

        /**
         * @brief 流式处理接收到的数据 (一次处理一个 MTU 包)
         *
         * 状态机主循环, 根据当前状态处理缓冲区中的数据:
         * - WaitingHeader: 查找帧头 0xAB
         * - ReceivingHeader: 接收剩余 5 字节头部
         * - StreamingData: 边接收边写入 SD 卡
         * - VerifyingChecksum: 接收并验证校验和
         *
         * @param[in] buffer 接收到的数据缓冲区
         * @param[in] len 缓冲区长度
         */
        void processStreamData(uint8_t* buffer, size_t len);

        /**
         * @brief 解析头部并准备流式接收数据
         *
         * 提取包长度, 包序号, 命令类型, 根据命令类型准备流式接收.
         *
         * @return true 解析成功并准备好流式接收, false 解析失败
         */
        bool parseHeaderAndPrepareStream();

        /**
         * @brief 处理 StreamingData 状态的数据接收
         *
         * 根据 TransferState 和 PacketType 处理数据:
         * - ReceivingHeader + Header: 累积文件名
         * - ReceivingData + Data: 写入 SD 卡
         * - ReceivingData + Cancel: 跳过数据
         *
         * @param[in] buffer 接收到的数据缓冲区
         * @param[in] len 缓冲区长度
         * @param[in,out] offset 当前处理位置 (引用传递) 
         * @return true 继续处理, false 发生错误需要退出
         */
        bool handleStreamingData(uint8_t* buffer, size_t len, size_t& offset);

        /**
         * @brief 验证校验和并发送响应
         *
         * 验证累加的校验和, 校验失败时回滚文件位置.
         * 校验成功时发送 ACK 响应.
         */
        void verifyChecksumAndRespond();

        /**
         * @brief 处理文件名包 (Header Packet)
         *
         * 检查 SD 卡状态, 规范化文件名, 创建文件, 切换到接收数据状态.
         * 直接操作 _fileName 成员变量.
         *
         * @param[in] nameLen 文件名长度
         * @return true 处理成功, false 处理失败
         */
        bool processHeaderPacket(size_t nameLen);


        /**
         * @brief 发送响应包给客户端
         *
         * 根据状态码构造响应包并发送.
         * 响应格式: [0xAB][STATUS][LEN_H][LEN_L][IDX_H][IDX_L][CHK_H][CHK_L]
         * 总长度固定为 8 字节.
         *
         * @param[in] status 响应状态码
         * @param[in] nextPacketIndex 下一个包序号 (所有响应都包含此字段)
         */
        void sendResponse(ResponseStatus status, uint16_t nextPacketIndex = 0);

        /**
         * @brief 清理传输状态和资源
         *
         * 统一的清理函数, 自动处理所有资源释放:
         * - 关闭文件 (如果已打开)
         * - 恢复 SD 卡状态 (如果需要)
         * - 重置接收缓冲区长度
         * - 重置所有状态变量
         *
         * @note 不会主动断开客户端连接, 只清理内部状态
         */
        void cleanup();

        /**
         * @brief 重置所有状态变量
         *
         * 统一的状态重置函数, 在构造函数, begin() 和 cleanup() 中调用.
         * 避免代码重复, 确保重置逻辑一致.
         */
        void resetState();
    };

    extern FileTransferServer file_transfer_server;
}
