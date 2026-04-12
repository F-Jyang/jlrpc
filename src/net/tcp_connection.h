#pragma once

#include <asio/ip/tcp.hpp>
#include <asio/streambuf.hpp>
#include <atomic>
#include <queue>

namespace jl
{
    namespace net = asio::ip;

    constexpr int kTotalLenSize = 4;
    constexpr std::size_t kDefaultBufferSize = 4 * 1024;

    class TcpConnection;
    using ConnectionPtr = std::shared_ptr<TcpConnection>;

    using ReadCallback = std::function<void(const ConnectionPtr &, const std::string&)>;  
    using WriteCallback = std::function<void(const ConnectionPtr &, std::size_t bytes_transferred)>;
    using CloseCallback = std::function<void(const ConnectionPtr &)>;

    enum class ConnectionState
    {
        kActived = 0,
        kClosed,
    };

    struct ConnectionInfo
    {
        std::string remote_ip;
        unsigned short remote_port;
        std::string local_ip;
        unsigned short local_port;
        std::string protocol;

        std::size_t hash()
        {
            std::stringstream ss;
            ss << local_ip << ":" << local_port << "-" << remote_ip << ":" << remote_port << "-" << protocol;
            return std::hash<std::string>{}(ss.str());
        }
    };

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(net::tcp::socket &&socket, std::size_t max_buffer_size = kDefaultBufferSize);

        bool Connect(const std::string& ip, unsigned short port);

        // std::size_t GetId() const;

        /// @brief 读取指定长度字节
        /// @param n
        void ReadLen(std::size_t n);

        /// @brief 读取指定结束符。如果字节数超过max_buffer_size还没有读取到end，会直接返回
        /// @param end
        void ReadUtil(const std::string &end);

        /// @brief 发送response string
        /// @param data
        void Write(const std::string &data);

        /// @brief 关闭连接
        void Close();

        ConnectionInfo GetConnectionInfo() const;

        const asio::any_io_executor& GetIoExecutor();

        void SetReadCallback(const ReadCallback &callback)
        {
            read_callback_ = callback;
        }

        void SetWriteCallback(const WriteCallback &callback)
        {
            write_callback_ = callback;
        }

        void SetCloseCallback(const CloseCallback &callback)
        {
            close_callback_ = callback;
        }

        ~TcpConnection();

    private:

        void OnRead(const std::error_code &ec, size_t bytes_transferred);

        void DoWrite();

        void OnWrite(const std::error_code &ec, size_t bytes_transferred);

        void OnTimeout(const std::error_code &ec);

        void HandleError(const std::error_code &ec);
    
    private:
        net::tcp::socket socket_;
        // std::atomic<std::chrono::steady_clock::time_point> timeout_point_;
        std::atomic<ConnectionState> state_;
        std::atomic<bool> is_reading_;
        // std::unique_ptr<ICoder> coder_;
        asio::streambuf read_buffer_;
        std::queue<std::string> write_queue_;
        ReadCallback read_callback_;
        WriteCallback write_callback_;
        CloseCallback close_callback_;
    };

}