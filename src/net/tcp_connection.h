#pragma once

#include <asio/ip/tcp.hpp>
#include <asio/streambuf.hpp>
#include <atomic>
#include <optional>
#include <queue>

namespace jl
{
    namespace net = asio::ip;

    constexpr int kTotalLenSize = 4;
    constexpr std::size_t kDefaultBufferSize = 4 * 1024;

    class TcpConnection;
    using ConnectionPtr = std::shared_ptr<TcpConnection>;

    using ReadCallback = std::function<void(const ConnectionPtr &, std::string_view, const std::error_code&)>;  
    using WriteCallback = std::function<void(const ConnectionPtr &, std::size_t bytes_transferred, const std::error_code&)>;
    using CloseCallback = std::function<void(const ConnectionPtr &, const std::error_code&)>;
    using TimeoutCallback = std::function<void(const ConnectionPtr&, const std::error_code&)>;

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
        TcpConnection(asio::io_context &ioct, net::tcp::socket &&socket, std::size_t max_buffer_size = kDefaultBufferSize);

        TcpConnection(asio::io_context &ioct, std::size_t max_buffer_size = kDefaultBufferSize);

        bool Connect(const std::string& ip, unsigned short port);

        // std::size_t GetId() const;

        /// @brief 异步从socket中读取指定长度字节。与AsyncReadUntil行为不同的是，该函数不会从read_buffer_中读取，而是直接从socket读取。
        /// @param n
        void AsyncReadLen(std::size_t n);

		/// @brief 同步读取指定长度字节
		/// @param n
        std::string SyncReadLen(std::size_t n, int timeout);

		/// @brief 同步读取指定长度字节
		/// @param n 
		/// @param ec 
		/// @return 
		std::string SyncReadLen(std::size_t n, int timeout, std::error_code& ec);

        /// @brief 异步读取指定结束符，如果read_buffer_中存在end则会直接调用回调，不存在end则从socket中读取，读取的字节数可能会多余end，多余的部分会存储在read_buffer_中。如果字节数超过max_buffer_size还没有读取到end，会直接返回
        /// @param end
        void AsyncReadUtil(std::string_view end);

		/// @brief 读取指定结束符。如果字节数超过max_buffer_size还没有读取到end，会直接返回
		/// @param end
        std::string SyncReadUtil(std::string_view end, int timeout);


        /// @brief 读取指定结束符。如果字节数超过max_buffer_size还没有读取到end，会直接返回
        /// @param end 
		/// @param timeout 超时时间 
		/// @param ec 
        /// @return 
        std::string SyncReadUtil(std::string_view end, int timeout, std::error_code& ec);

        /// @brief 异步发送数据，禁止与同步发送数据同时使用
        /// @param data
        void AsyncWrite(std::string_view data);

		/// @brief 同步发送数据，禁止与异步发送同时使用，线程不安全
		/// @param data 
		/// @param timeout 超时时间 
		/// @return 
		std::size_t SyncWrite(std::string_view data, int timeout);

		/// @brief 同步发送数据，禁止与异步发送同时使用，线程不安全
		/// @param data 
		/// @param timeout 超时时间 
		/// @param ec 
		/// @return 
		std::size_t SyncWrite(std::string_view data, int timeout, std::error_code& ec);

        /// @brief 关闭连接
        void Close();

        /// 取消socket上的所有操作
        void Cancel();

        void Cancel(std::error_code& ec);

        /// @brief 判断是否连接
        bool IsConnected();

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

        void SetTimeoutCallback(const TimeoutCallback& callback)
        {
            timeout_callback_ = callback;
        }

        ~TcpConnection();

    private:

        void OnRead(const std::error_code &ec, size_t bytes_transferred);

        void DoWrite();

        void OnWrite(const std::error_code &ec, size_t bytes_transferred);

        void OnTimeout(const std::error_code &ec);

		template <typename Rep, typename Period>
		void Wait(const std::chrono::duration<Rep, Period>& timeout)
		{
			ioct_.restart();
			ioct_.run_for(timeout);
			if (!ioct_.stopped())
			{
				this->OnTimeout(std::make_error_code(std::errc::timed_out));
				// run the io_context again until the operation completes. 处理掉超时后被中断的任务，任务的异步回调中ec应该是operation_abort
				ioct_.run();
			}
		}
    
    private:
        asio::io_context& ioct_;
        net::tcp::socket socket_;
        std::atomic<ConnectionState> state_;
        std::atomic<bool> is_reading_;
        asio::streambuf read_buffer_;
        std::queue<std::string> write_queue_;
        ReadCallback read_callback_;
        WriteCallback write_callback_;
        CloseCallback close_callback_;
        TimeoutCallback timeout_callback_;
    };

}