#include "tcp_connection.h"

#include <utils/easy_log.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/connect.hpp>
#include <asio/io_context.hpp>

namespace jl
{
    AsyncConnection::AsyncConnection(asio::io_context &ioct, net::tcp::socket &&socket, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(std::move(socket)),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kActived),
          is_reading_(false),
          conn_info_(nullptr)
    {
        conn_info_ = new ConnectionInfo(socket_);
    }

    AsyncConnection::AsyncConnection(asio::io_context &ioct, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(ioct),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kClosed),
          is_reading_(false),
          conn_info_(nullptr)
    {
    }

    bool AsyncConnection::Connect(const std::string &ip, unsigned short port)
    {
        AssertInThread();
        std::error_code ec;
        // 关键：确保 socket 是打开状态
        if (!socket_.is_open())
        {
            socket_.open(asio::ip::tcp::v4(), ec);
            if (ec)
            {
                // log error: "Failed to open socket"
                return false;
            }
            // 重新设置 socket 选项（close 后会丢失）
            socket_.set_option(asio::ip::tcp::no_delay(true), ec);
            socket_.set_option(asio::socket_base::reuse_address(true), ec);
        }
        // TODO: 实现异步connect
        asio::ip::tcp::resolver resolver(socket_.get_executor());
        // resolver.async_resolve()
        // asio::async_connect()

        auto resolve_result = resolver.resolve(net::tcp::endpoint(net::make_address(ip), port));
        auto iter = asio::connect(socket_, resolve_result.begin(), resolve_result.end(), ec);
        if (ec || iter == resolve_result.end())
        {
            // log error
            socket_.close(ec);
            return false;
        }
        if (conn_info_)
            delete conn_info_;
        conn_info_ = new ConnectionInfo(socket_);
        state_ = ConnectionState::kActived;
        return true;
    }

    void AsyncConnection::AsyncReadLen(std::size_t n)
    {
        AssertInThread();
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            std::size_t readable = read_buffer_.size();
            // read_buffer_中剩余的字节超过n。解析HTTP，先ReadUntil后ReadLen可能进入
            if (readable >= n)
            {
                std::error_code ec;
                OnRead(ec, n);
                return;
            }

            auto self = shared_from_this();
            asio::async_read(
                this->socket_,
                read_buffer_,
                asio::transfer_exactly(n - readable),
                [self, readable](const std::error_code &ec, std::size_t bytes_transferred)
                {
                    self->OnRead(ec, readable + bytes_transferred);
                });
        }
        else
        {
            // 禁止同一时刻注册两个async_read事件
            assert(false);
        }
    }

    void AsyncConnection::AsyncReadUtil(std::string_view end)
    {
        AssertInThread();
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            auto self = shared_from_this();
            asio::async_read_until(
                this->socket_,
                read_buffer_,
                end,
                [self](const std::error_code &ec, std::size_t bytes_transferred)
                {
                    self->OnRead(ec, bytes_transferred);
                });
        }
    }

    AsyncConnection::~AsyncConnection()
    {
        if (conn_info_)
        {
            delete conn_info_;
        }
        LOG_DEBUG << __FUNCTION__;
    }

    void AsyncConnection::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        if (state_ != ConnectionState::kClosed)
        {
            std::string read_str;
            if (!ec)
            {
                read_str.resize(bytes_transferred);
                std::istream is(&read_buffer_);
                is.read(read_str.data(), bytes_transferred);
            }
            bool expected = true;
            is_reading_.compare_exchange_strong(expected, false);
            if (read_callback_)
            {
                read_callback_(shared_from_this(), read_str, ec);
            }
        }
    }

    void AsyncConnection::AsyncWrite(std::string_view data)
    {
        AssertInThread();
        auto self = shared_from_this();
        std::string copy = std::string(data.data(), data.size());
        asio::post(
            socket_.get_executor(), // 保证send_queue线程安全
            [self, copy]()
            {
                const bool is_writing = !self->write_queue_.empty();
                self->write_queue_.emplace(copy);
                // LOG_DEBUG << std::to_string(self->write_queue_.size());
                if (!is_writing)
                {
                    self->DoWrite();
                }
            });
    }

    void AsyncConnection::DoWrite()
    {
        auto self = shared_from_this();
        LOG_DEBUG << "Write response";
        asio::async_write(
            socket_,
            asio::buffer(this->write_queue_.front()),
            asio::transfer_exactly(this->write_queue_.front().size()),
            [self](const std::error_code &ec, size_t bytes_transferred)
            {
                if (self->state_ != ConnectionState::kClosed) // 连接已断开
                {
                    self->OnWrite(ec, bytes_transferred);
                    if (!ec)
                    {
                        self->write_queue_.pop();
                        if (!self->write_queue_.empty())
                        {
                            self->DoWrite();
                        }
                    }
                }
            });
    }

    void AsyncConnection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
    {
        if (write_callback_)
        {
            write_callback_(shared_from_this(), bytes_transferred, ec);
        }
    }

    void AsyncConnection::OnConnect(const std::error_code &ec)
    {
        if (connect_callback_)
        {
            close_callback_(shared_from_this(), ec);
        }
    }

    void AsyncConnection::Close()
    {
        AssertInThread();
        ConnectionState expected = ConnectionState::kActived;
        if (state_.compare_exchange_strong(expected, ConnectionState::kClosed))
        {
            std::error_code ignore;
            socket_.shutdown(net::tcp::socket::shutdown_both, ignore);
            socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
            auto self = shared_from_this();
            if (this->close_callback_)
            {

                this->close_callback_(self, ignore);
            }
        }
    }

    void AsyncConnection::Cancel()
    {
        AssertInThread();
        std::error_code ec;
        Cancel(ec);
        if (ec)
        {
            asio::detail::throw_error(ec);
        }
    }

    void AsyncConnection::Cancel(std::error_code &ec)
    {
        socket_.cancel(ec);
        // LOG_DEBUG << std::to_string(socket_.available());
    }

    bool AsyncConnection::IsConnected()
    {
        return state_.load() == ConnectionState::kActived;
    }

    const ConnectionInfo &AsyncConnection::GetConnectionInfo()
    {
        return *conn_info_;
    }

    const asio::any_io_executor &AsyncConnection::GetIoExecutor()
    {
        return socket_.get_executor();
    }

    SyncConnection::SyncConnection(asio::io_context &ioct, net::tcp::socket &&socket, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(std::move(socket)),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kActived),
          conn_info_(nullptr)
    {
    }

    SyncConnection::SyncConnection(asio::io_context &ioct, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(ioct),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kClosed),
          conn_info_(nullptr)
    {
    }

    bool SyncConnection::Connect(const std::string &ip, unsigned short port)
    {
        AssertInThread();
        std::error_code ec;
        // 关键：确保 socket 是打开状态
        if (!socket_.is_open())
        {
            socket_.open(asio::ip::tcp::v4(), ec);
            if (ec)
            {
                // log error: "Failed to open socket"
                return false;
            }
            // 重新设置 socket 选项（close 后会丢失）
            socket_.set_option(asio::ip::tcp::no_delay(true), ec);
            socket_.set_option(asio::socket_base::reuse_address(true), ec);
        }
        asio::ip::tcp::resolver resolver(socket_.get_executor());
        auto resolve_result = resolver.resolve(net::tcp::endpoint(net::make_address(ip), port));
        auto iter = asio::connect(socket_, resolve_result.begin(), resolve_result.end(), ec);
        if (ec || iter == resolve_result.end())
        {
            // log error
            socket_.close(ec);
            return false;
        }
        if (conn_info_)
            delete conn_info_;
        conn_info_ = new ConnectionInfo(socket_);
        state_ = ConnectionState::kActived;
        return true;
    }

    std::string SyncConnection::SyncReadLen(std::size_t n, int timeout)
    {
        AssertInThread();
        std::error_code ec;
        std::string result = SyncReadLen(n, timeout, ec);
        if (ec)
        {
            asio::detail::throw_error(ec, "read_len");
        }
        return result;
    }

    std::string SyncConnection::SyncReadLen(std::size_t n, int timeout, std::error_code &ec)
    {
        AssertInThread();
        ec.clear();
        std::size_t readable = read_buffer_.size();
        std::size_t size = 0;
        // 读取read_buffer_中残留的字节
        if (readable >= n)
        {
            size = n;
        }
        else
        {
            ec = asio::error::timed_out;
            asio::async_read(
                socket_,
                read_buffer_,
                asio::transfer_exactly(n - readable),
                /* bytes_transffered: Number of bytes copied into the buffers. If an error occurred, this will be the number of bytes successfully transferred prior to the error. */
                [&](const std::error_code &result_ec, std::size_t bytes_transffered)
                {
                    ec = result_ec;
                    size = readable + bytes_transffered;
                });
            Wait(std::chrono::seconds(timeout));
        }
        // warn: 超时、断开重连时需要读取缓存在read_buffer_中的数据，否则下次read会导致读取的是read_buffer_缓存的数据
        std::string result(size, '\0');
        std::istream is(&read_buffer_);
        is.read(result.data(), size);
        if (ec == asio::error::operation_aborted) // 被中断，说明超时
        {
            ec = asio::error::timed_out;
        }
        // if (ec) ec = asio::error::timed_out; // 超时统一处理为 timed_out
        // if (!ec || ec == asio::error::operation_aborted || ec == asio::error::timed_out || ec == asio::error::eof)
        return result;
    }

    std::string SyncConnection::SyncReadUtil(std::string_view end, int timeout)
    {
        std::error_code ec;
        std::string result = SyncReadUtil(end, timeout, ec);
        if (ec)
        {
            asio::detail::throw_error(ec, "read_until");
        }
        return result;
    }

    std::string SyncConnection::SyncReadUtil(std::string_view end, int timeout, std::error_code &ec)
    {
        AssertInThread();
        std::size_t size = 0;
        ec = asio::error::timed_out;
        asio::async_read_until(
            this->socket_,
            read_buffer_,
            end,
            /* bytes_transffered: The number of bytes in the streambuf's get area up to and including the delimiter. 0 if an error occurred */
            [&](const std::error_code &result_ec, std::size_t bytes_transffered)
            {
                ec = result_ec;
                size = bytes_transffered;
            });
        Wait(std::chrono::seconds(timeout));
        std::string result(size, '\0');
        std::istream is(&read_buffer_);
        is.read(result.data(), size);
        if (ec == asio::error::operation_aborted) // 被中断，说明超时
        {
            ec = asio::error::timed_out;
        }
        // warn: 超时、断开重连时需要读取缓存在read_buffer_中的数据，否则下次read会导致读取的是read_buffer_缓存的数据
        // if (!ec || ec == asio::error::operation_aborted || ec == asio::error::timed_out || ec == asio::error::eof) // 超时统一 ec 为 timed_out
        // if(ec) ec = asio::error::timed_out;
        return result;
    }

    SyncConnection::~SyncConnection()
    {
        if (conn_info_)
        {
            delete conn_info_;
        }
        LOG_DEBUG << __FUNCTION__;
    }

    std::size_t SyncConnection::SyncWrite(std::string_view data, int timeout)
    {
        AssertInThread();
        std::error_code ec;
        std::size_t n = SyncWrite(data, timeout, ec);
        if (ec)
        {
            asio::detail::do_throw_error(ec);
        }
        return n;
    }

    std::size_t SyncConnection::SyncWrite(std::string_view data, int timeout, std::error_code &ec)
    {
        AssertInThread();
        std::size_t size = 0;
        ec = asio::error::timed_out;
        asio::async_write(
            socket_,
            asio::buffer(data),
            asio::transfer_exactly(data.size()),
            [&](const std::error_code &result_ec, std::size_t bytes_transffered)
            {
                ec = result_ec;
                size = bytes_transffered;
            });
        Wait(std::chrono::seconds(timeout));
        if (ec == asio::error::operation_aborted) // 被中断，说明超时
        {
            ec = asio::error::timed_out;
        }
        return size;
    }

    void SyncConnection::Close()
    {
        AssertInThread();
        ConnectionState expected = ConnectionState::kActived;
        if (state_.compare_exchange_strong(expected, ConnectionState::kClosed))
        {
            std::error_code ignore;
            socket_.shutdown(net::tcp::socket::shutdown_both, ignore);
            socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
        }
    }

    void SyncConnection::Cancel()
    {
        AssertInThread();
        std::error_code ec;
        Cancel(ec);
        if (ec)
        {
            asio::detail::throw_error(ec);
        }
    }

    void SyncConnection::Cancel(std::error_code &ec)
    {
        socket_.cancel(ec);
        // LOG_DEBUG << std::to_string(socket_.available());
    }

    bool SyncConnection::IsConnected()
    {
        return state_.load() == ConnectionState::kActived;
    }

    const ConnectionInfo &SyncConnection::GetConnectionInfo()
    {
        return *conn_info_;
    }

    const asio::any_io_executor &SyncConnection::GetIoExecutor()
    {
        return socket_.get_executor();
    }
}
