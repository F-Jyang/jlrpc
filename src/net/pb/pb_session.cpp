#include "pb_session.h"

#include <net/pb/pb_data.h>
#include <asio/read.hpp>
#include <asio/write.hpp>

namespace jl
{
    PbSession::PbSession(int64_t session_id, net::tcp::socket&& socket) :
        session_id_(session_id),
        socket_(std::move(socket))
    {
    }

    int64_t PbSession::GetId() const
    {
        return session_id_;
    }

    void PbSession::Read()
    {
        this->ReadRequestLen();
    }

    void PbSession::ReadRequestLen()
    {
        bool expected = false;
        if (is_reading_.compare_exchange_strong(expected, true))
        {
            auto self = shared_from_this();
            asio::async_read(
                this->socket_,
                read_buffer_,
                asio::transfer_exactly(kTotalLenSize),
                [self](const std::error_code &ec, std::size_t bytes_transferred)
                {
                    if (ec)
                    {
                        self->HandleError(ec);
                        return;
                    }

                    if (self->state_ != ConnectionState::kClosed)
                    {
                        std::istream is(&self->read_buffer_);
                        std::size_t len;
                        is.read((char *)(&len), bytes_transferred); // 暂时不做大小端处理
                        self->ReadRequest(len);
                    }
                });
        }
    }

    void PbSession::ReadRequest(std::size_t req_len)
    {
        auto self = shared_from_this();
        asio::async_read(
            this->socket_,
            read_buffer_,
            asio::transfer_exactly(req_len),
            [self](const std::error_code &ec, std::size_t bytes_transferred)
            {
                if (self->state_ != ConnectionState::kClosed)
                {
                    self->OnRead(ec, bytes_transferred);
                    bool expected = true;
                    self->is_reading_.compare_exchange_strong(expected, false);
                }
            });
    }

    void PbSession::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        if (!ec)
        {
            if (read_callback_)
            {
                read_callback_(shared_from_this(), read_buffer_);
            }
            else
            {
                // rpc failed, handle no callback set，send error code
                
                // define a global func ???
                // OnNoReadCallback(req_opt.value());
            }
        }
        else
        {
            if (ec != asio::error::eof)
            {
                std::error_code ignore;
                // log
            }
            Close();
        }
    }

    void PbSession::Write(const std::string& data)
    {
        auto self = shared_from_this();
        asio::post(
            socket_.get_executor(), // 保证send_queue线程安全
            [self, copy = std::move(data)]()
            {
                const bool is_writing = !self->write_queue_.empty();
                self->write_queue_.emplace(copy);
                if (!is_writing)
                {
                    self->DoWrite();
                }
            });
    }

    void PbSession::DoWrite()
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

    void PbSession::OnWrite(const std::error_code &ec, size_t bytes_transferred)
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

    void PbSession::OnTimeout(const std::error_code& ec)
    {
        if (ec != asio::error::operation_aborted)
        {

        }
    }

    void PbSession::Close()
    {
        ConnectionState expected = ConnectionState::kActived;
        if (state_.compare_exchange_strong(expected, ConnectionState::kClosed))
        {
            std::error_code ignore;
            socket_.shutdown(net::tcp::socket::shutdown_both, ignore);
            socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
            if (this->close_callback_)
            {
                this->close_callback_(shared_from_this());
            }
        }
    }

    void PbSession::HandleError(const std::error_code &ec)
    {
    }
}
