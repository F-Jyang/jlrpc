#pragma once

#include <interface/i_session.h>
#include <net/pb/pb_coder.h>
#include <asio/ip/tcp.hpp>
#include <atomic>
#include <queue>

namespace jl
{
    namespace net = asio::ip;

    class PbSession : public std::enable_shared_from_this<PbSession>, public ISession
    {
    public:
        PbSession(int64_t session_id, net::tcp::socket &&socket);

        int64_t GetId() const override;

        /// @brief 开始读取proto格式数据
        void StartRead() override;

        /// @brief 发送IRespnse
        /// @param response_ptr
        void Write(const ResponsePtr &resp_str) override;

        /// @brief 关闭连接
        void Close() override;

        void SetReadCallback(const ReadCallback &callback) override
        {
            read_callback_ = callback;
        }

        void SetWriteCallback(const WriteCallback &callback) override
        {
            write_callback_ = callback;
        }

        void SetCloseCallback(const CloseCallback &callback) override
        {
            close_callback_ = callback;
        }

    private:
        /// @brief 读取4B的request_len
        void ReadRequestLen();

        /// @brief 读取指定长度的request
        /// @param req_size 指定长度
        void ReadRequest(std::size_t req_len);

        void OnRead(const std::error_code &ec, size_t bytes_transferred);

        void DoWrite();

        void OnWrite(const std::error_code &ec, size_t bytes_transferred);

        void HandleError(const std::error_code &ec);

        void OnNoReadCallback(const RequestPtr &req_ptr);

    private:
        std::int64_t session_id_;
        std::atomic<ConnectionState> state_;
        std::atomic<bool> is_reading_;
        std::unique_ptr<ICoder> coder_;
        asio::streambuf read_buffer_;
        net::tcp::socket socket_;
        std::queue<std::string> write_queue_;
        ReadCallback read_callback_;
        WriteCallback write_callback_;
        CloseCallback close_callback_;
    };

}