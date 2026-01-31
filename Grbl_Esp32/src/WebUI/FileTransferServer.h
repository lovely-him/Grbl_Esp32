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
        static constexpr uint32_t RXBUFFERSIZE          = 34816;   /**< 接收缓冲区大小 (33KB, 支持 32KB 数据包 + 头部) */

        // 接收缓冲区 (堆分配, 避免 BSS 段溢出)
        uint8_t* _rxBuffer;  /**< 接收缓冲区 (堆分配, 构造时创建) */

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
         * @brief 响应状态码枚举
         */
        enum class ResponseStatus : uint8_t {
            Success       = 0x01,  /**< 成功, 请求下一个包 */
            ProtocolError = 0x02,  /**< 协议错误 (校验, 格式, 命令等客户端问题) */
            ServerError   = 0x03   /**< 服务器错误 (超时, SD卡, 状态机等服务器问题) */
        };

        size_t   _rxBufferSize;         /**< 缓冲区中当前数据长度 */

        // 文件传输状态
        File     _uploadFile;           /**< 当前上传的文件对象 */
        char     _fileName[256];        /**< 当前文件名 (固定大小, 避免堆碎片) */
        uint16_t _expectedPacketIndex;  /**< 期望的下一个数据包序号 */
        uint32_t _lastActivityTime;     /**< 最后一次活动时间 (毫秒) */
        bool     _skipSDWrite;          /**< 跳过 SD 卡写入 (SD 卡不可用时) */

        // 协议处理方法

        /**
         * @brief 处理已连接客户端的数据接收和超时检查
         *
         * 检查超时, 读取可用数据, 解析数据包.
         * 在 handle() 中被调用.
         */
        void handleClient();

        /**
         * @brief 解析接收缓冲区中的数据包
         *
         * 验证魔术字节, 提取包长度, 验证校验和, 根据命令类型分发处理.
         *
         * @return true 成功解析并处理一个完整数据包, false 数据不足或解析失败
         */
        bool parsePacket();

        /**
         * @brief 处理文件名包 (Header Packet)
         *
         * 检查 SD 卡状态, 提取文件名, 创建文件, 切换到接收数据状态.
         *
         * @param[in] data 数据包内容指针
         * @param[in] len 数据包长度
         * @return true 处理成功, 可以继续; false 已调用 cleanup(), 必须立即退出
         */
        bool processHeaderPacket(const uint8_t* data, uint16_t len);

        /**
         * @brief 处理数据包 (Data Packet)
         *
         * 验证包序号, 提取数据内容, 写入 SD 卡, 发送 ACK.
         *
         * @param[in] data 数据包内容指针
         * @param[in] len 数据包长度
         * @return true 处理成功, 可以继续; false 已调用 cleanup(), 必须立即退出
         */
        bool processDataPacket(const uint8_t* data, uint16_t len);


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
         * @brief 验证数据包校验和
         *
         * 计算数据包的 16 位累加和并与包中的校验和字段比较.
         * 校验和算法: 累加除最后 2 字节外的所有字节, 取低 16 位.
         *
         * @param[in] data 数据包内容指针
         * @param[in] len 数据包长度
         * @return true 校验和正确, false 校验和错误
         */
        bool verifyChecksum(const uint8_t* data, uint16_t len);

        /**
         * @brief 清理传输状态和资源
         *
         * 统一的清理函数, 自动处理所有资源释放:
         * - 关闭文件 (如果已打开)
         * - 恢复 SD 卡状态 (如果需要)
         * - 断开客户端连接 (如果已连接)
         * - 重置接收缓冲区长度
         * - 重置所有状态变量
         */
        void cleanup();
    };

    extern FileTransferServer file_transfer_server;
}
