#include "tcp_connection.h"

#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/connect.hpp>

namespace jl
{
    TcpConnection::TcpConnection(net::tcp::socket &&socket, ConnectionState state, std::size_t max_buffer_size)
        : socket_(std::move(socket)),
          read_buffer_(max_buffer_size),
          state_(state),
          is_reading_(false)
    {
    }

    bool TcpConnection::Connect(const std::string &ip, unsigned short port)
    {
        std::error_code ec;
        asio::ip::tcp::resolver resolver(socket_.get_executor());
        auto resolve_result = resolver.resolve(net::tcp::endpoint(net::make_address(ip), port));
        auto iter = asio::connect(socket_, resolve_result.begin(), resolve_result.end(), ec);
        if (ec || iter != resolve_result.end())
        {
            // log error
            return false;
        }
        state_ = ConnectionState::kActived;
        return true;
    }

    void TcpConnection::ReadLen(std::size_t n)
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

    std::string TcpConnection::SyncReadLen(std::size_t n)
    {
        std::error_code ec;
        std::string result = SyncReadLen(n, ec);
        asio::detail::throw_error(ec, "read_len"); // if(ec) throw std::system_error(xxx);
        return result;
    }

    std::string TcpConnection::SyncReadLen(std::size_t n, std::error_code &ec)
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, false))
        {
            expected = true;
            std::size_t len = asio::read(socket_, read_buffer_, asio::transfer_exactly(n), ec);
            if (ec)
            {
                is_reading_.compare_exchange_strong(expected, false);
                // log error
                return "";
            }
            std::string result(len, '\0');
            std::ostream os(&read_buffer_);
            os.write(result.c_str(), len);
            is_reading_.compare_exchange_strong(expected, false);

            return result;
        }
        ec = asio::error::in_progress;
        return "";
    }

    void TcpConnection::ReadUtil(const std::string &end)
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
                    if (ec)
                    {
                        self->HandleError(ec);
                        return;
                    }
                    if (self->state_ != ConnectionState::kClosed)
                    {
                        self->OnRead(ec, bytes_transferred);
                        bool expected = true;
                        self->is_reading_.compare_exchange_strong(expected, false);
                    }
                });
        }
    }

    std::string TcpConnection::SyncReadUtil(std::string_view end)
    {
        std::error_code ec;
        std::string result = SyncReadUtil(end, ec);
        asio::detail::throw_error(ec, "read_until");
        return result;
    }

    std::string TcpConnection::SyncReadUtil(std::string_view end, std::error_code &ec)
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            expected = true;
            std::size_t n = asio::read_until(this->socket_, read_buffer_, end, ec);
            if (ec)
            {
                // log error
                is_reading_.compare_exchange_strong(expected, false);
                return "";
            }
            std::string result(n, '\0');
            std::ostream os(&read_buffer_);
            os.write(result.c_str(), n);
            is_reading_.compare_exchange_strong(expected, false);
            return result;
        }
        ec = asio::error::in_progress;
        return "";
    }

    TcpConnection::~TcpConnection()
    {
    }

    void TcpConnection::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        if (!ec)
        {
            if (read_callback_)
            {
                std::string read_str(bytes_transferred, '\0');
                std::istream is(&read_buffer_);
                is.read(read_str.data(), bytes_transferred);
                read_callback_(shared_from_this(), read_str);
            }
            else
            {
                // TODO: rpc failed, handle no callback set，send error code

                // define a global func ???
                // OnNoReadCallback(req_opt.value());
            }
        }
        else
        {
            if (ec != asio::error::eof)
            {
                std::error_code ignore;
            }
            Close();
        }
    }

    void TcpConnection::Write(const std::string &data)
    {
        auto self = shared_from_this();
        asio::post(
            socket_.get_executor(), // 保证send_queue线程安全
            [self, copy = std::move(data)]()
            {
                const bool is_writing = !self->write_queue_.empty();
                self->write_queue_.emplace(std::move(copy));
                // LOG_DEBUG << std::to_string(self->write_queue_.size());
                if (!is_writing)
                {
                    self->DoWrite();
                }
            });
    }

    void TcpConnection::DoWrite()
    {
        auto self = shared_from_this();
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
        if (!ec)
        {
            if (write_callback_)
            {
                write_callback_(shared_from_this(), bytes_transferred);
            }
        }
        else
        {
            std::error_code ignore;
            // log
            Close();
        }
    }

    void TcpConnection::OnTimeout(const std::error_code &ec)
    {
        if (ec != asio::error::operation_aborted)
        {
        }
    }

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

                this->close_callback_(self);
            }
        }
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

    void TcpConnection::HandleError(const std::error_code &ec)
    {
        // LOG_DEBUG << std::to_string(GetId()) << " handle error:" + ec.message();
        if (ec == asio::error::eof)
        {
            Close();
        }
    }
}
