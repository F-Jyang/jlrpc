#include "tcp_connection.h"

#include <utils/easy_log.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/connect.hpp>
#include <asio/io_context.hpp>

namespace jl
{
    TcpConnection::TcpConnection(asio::io_context &ioct, net::tcp::socket &&socket, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(std::move(socket)),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kActived),
          is_reading_(false)
    {
    }

    TcpConnection::TcpConnection(asio::io_context &ioct, std::size_t max_buffer_size)
        : ioct_(ioct),
          socket_(ioct),
          read_buffer_(max_buffer_size),
          state_(ConnectionState::kClosed),
          is_reading_(false)
    {
    }

    bool TcpConnection::Connect(const std::string &ip, unsigned short port)
    {
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
        state_ = ConnectionState::kActived;
        return true;
    }

    void TcpConnection::AsyncReadLen(std::size_t n)
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            auto self = shared_from_this();
            asio::async_read(
                this->socket_,
                read_buffer_,
                asio::transfer_exactly(n),
                [self](const std::error_code &ec, std::size_t bytes_transferred)
                {
                    if (self->state_ != ConnectionState::kClosed)
                    {
                        bool expected = true;
                        self->is_reading_.compare_exchange_strong(expected, false);
                        self->OnRead(ec, bytes_transferred);
                    }
                });
        }
        else
        {
            // 禁止同一时刻注册两个async_read事件
            // assert(false);
        }
    }

    std::string TcpConnection::SyncReadLen(std::size_t n, int timeout)
    {
        std::error_code ec;
        std::string result = SyncReadLen(n, timeout, ec);
        if (ec)
        {
            asio::detail::throw_error(ec, "read_len");
        }
        return result;
    }

    std::string TcpConnection::SyncReadLen(std::size_t n, int timeout, std::error_code &ec)
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            std::size_t size;
            ec = asio::error::would_block;
            asio::async_read(
                socket_,
                read_buffer_,
                asio::transfer_exactly(n),
                [&](const std::error_code &block, std::size_t bytes_transffered)
                {
                    ec = block;
                    size = bytes_transffered;
                });
			Wait(std::chrono::seconds(timeout));
            expected = true;
            is_reading_.compare_exchange_strong(expected, false);
            // warn: 超时、断开重连时需要读取缓存在read_buffer_中的数据，否则下次read会导致读取的是read_buffer_缓存的数据
            if (!ec || ec == asio::error::operation_aborted || ec == asio::error::timed_out || ec == asio::error::eof) 
            {
                std::string result(size, '\0');
                std::istream is(&read_buffer_);
                is.read(result.data(), size);
                return result;
            }
            return "";
        }
        ec = asio::error::in_progress;
        return "";
    }

    void TcpConnection::AsyncReadUtil(std::string_view end)
    {
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
                    if (self->state_ != ConnectionState::kClosed)
                    {
                        bool expected = true;
                        self->is_reading_.compare_exchange_strong(expected, false);
                        self->OnRead(ec, bytes_transferred);
                    }
                });
        }
    }

    std::string TcpConnection::SyncReadUtil(std::string_view end, int timeout)
    {
        std::error_code ec;
        std::string result = SyncReadUtil(end, timeout, ec);
        if (ec)
        {
            asio::detail::throw_error(ec, "read_until");
        }
        return result;
    }

    std::string TcpConnection::SyncReadUtil(std::string_view end, int timeout, std::error_code &ec)
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            std::size_t size;
            ec = asio::error::would_block;
            asio::async_read_until(
                this->socket_,
                read_buffer_,
                end,
                [&](const std::error_code& block, std::size_t bytes_transffered)
                {
                    ec = block;
                    size = bytes_transffered;
                });
			Wait(std::chrono::seconds(timeout));
            expected = true;
            is_reading_.compare_exchange_strong(expected, false);
			// warn: 超时、断开重连时需要读取缓存在read_buffer_中的数据，否则下次read会导致读取的是read_buffer_缓存的数据
			if (!ec || ec == asio::error::operation_aborted || ec == asio::error::timed_out || ec == asio::error::eof)
			{
				std::string result(size, '\0');
				std::istream is(&read_buffer_);
				is.read(result.data(), size);
				return result;
			}
            return "";
        }
        ec = asio::error::in_progress;
        return "";
    }

    TcpConnection::~TcpConnection()
    {
    }

    void TcpConnection::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        std::string read_str;
        if (!ec)
        {
            read_str.resize(bytes_transferred);
            std::istream is(&read_buffer_);
            is.read(read_str.data(), bytes_transferred);
        }
        if (read_callback_)
        {
            read_callback_(shared_from_this(), read_str, ec);
        }
    }

    void TcpConnection::AsyncWrite(std::string_view data)
    {
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

    std::size_t TcpConnection::SyncWrite(std::string_view data, int timeout)
    {
        std::error_code ec;
        std::size_t n = SyncWrite(data, timeout, ec);
        if (ec)
        {
            asio::detail::do_throw_error(ec);
        }
        return n;
    }

    std::size_t TcpConnection::SyncWrite(std::string_view data, int timeout, std::error_code &ec)
    {
        std::size_t size;
        ec = asio::error::would_block;
        asio::async_write(
            socket_,
            asio::buffer(data),
            asio::transfer_exactly(data.size()),
            [&](const std::error_code &block, std::size_t bytes_transffered)
            {
                ec = block;
                size = bytes_transffered;
            });
        Wait(std::chrono::seconds(timeout));
        return size;
    }

    void TcpConnection::DoWrite()
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

    void TcpConnection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
    {
        if (write_callback_)
        {
            write_callback_(shared_from_this(), bytes_transferred, ec);
        }
    }

    void TcpConnection::OnTimeout(const std::error_code& ec)
    {
        //if (ec && ec != asio::error::operation_aborted)
        //{
        //    LOG_DEBUG << __FUNCTION__ << ": " << ec.message();
        //}
        if (timeout_callback_)
        {
            timeout_callback_(shared_from_this(), ec);
        }
    };

    void TcpConnection::Close()
    {
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

    void TcpConnection::Cancel()
    {
        std::error_code ec;
        Cancel(ec);
        if (ec)
        {
            asio::detail::throw_error(ec);
        }
    }

    void TcpConnection::Cancel(std::error_code &ec)
    {
        socket_.cancel(ec);
        //LOG_DEBUG << std::to_string(socket_.available());
    }

    bool TcpConnection::IsConnected()
    {
        return state_.load() == ConnectionState::kActived;
    }

    ConnectionInfo TcpConnection::GetConnectionInfo() const
    {
        ConnectionInfo info;
        info.remote_ip = socket_.remote_endpoint().address().to_string();
        info.remote_port = socket_.remote_endpoint().port();
        info.local_ip = socket_.local_endpoint().address().to_string();
        info.local_port = socket_.local_endpoint().port();
        info.protocol = "TCP";
        return info;
    }

    const asio::any_io_executor &TcpConnection::GetIoExecutor()
    {
        return socket_.get_executor();
    }
}
