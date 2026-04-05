#include "pb_client.h"

namespace jl
{
    jl::PbClient::PbClient(asio::io_context &ioct, std::size_t max_buffer_size)
        : state_(PbClientState::kReadTotalLen)
    {
        net::tcp::socket socket(ioct);
        connection_ = std::make_shared<TcpConnection>(std::move(socket), max_buffer_size);
    }

    bool PbClient::Connect(const std::string &ip, unsigned short port)
    {
        return connection_->Connect(ip, port);
    }

    void PbClient::SendRequest(const RequestPtr &req_ptr)
    {
        std::string req_str = GetPbCoder().EncodeRequest(req_ptr);
        connection_->Write(req_str);
    }

    void PbClient::SendHeartBeat()
    {
        RequestPtr req_ptr = std::make_shared<HeartBeatRequest>();
        SendRequest(req_ptr);
    }

    void PbClient::Read()
    {
        connection_->ReadLen(kTotalLenSize);
    }

    void PbClient::Close()
    {
        connection_->Close();
    }

    void PbClient::OnResponse(const ConnectionPtr &conn, asio::streambuf &buffer, size_t bytes_transfered)
    {
        if (state_ == PbClientState::kReadTotalLen)
        {
            if (bytes_transfered != kTotalLenSize)
            {
                // log
                assert(false);
                // error
                return;
            }
            std::size_t req_len = 0;
            std::istream is(&buffer);
            is.read(reinterpret_cast<char *>(&req_len), kTotalLenSize);
            connection_->ReadLen(req_len - kTotalLenSize);
            state_ = PbClientState::kReadRequest;
        }
        else if (state_ == PbClientState::kReadRequest)
        {
            ResponsePtr resp_ptr = GetPbCoder().DecodeResponse(buffer, bytes_transfered);
            read_callback_(shared_from_this(), resp_ptr);
            state_ = PbClientState::kReadTotalLen;
        }
        else
        {
            // log
            assert(false);
            // error
            return;
        }
    }

    void PbClient::OnRequest(const ConnectionPtr &conn, std::size_t bytes_transferred)
    {
        write_callback_(shared_from_this(), bytes_transferred);
    }
}
