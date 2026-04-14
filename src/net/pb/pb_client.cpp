#include "pb_client.h"

namespace jl
{
    jl::PbClient::PbClient(asio::io_context &ioct, std::size_t max_buffer_size)
        : state_(PbClientState::kReadTotalLen),
         timer_(ioct)
    {
        net::tcp::socket socket(ioct);
        connection_ = std::make_shared<TcpConnection>(std::move(socket), max_buffer_size);
    }

    bool PbClient::Connect(const std::string &ip, unsigned short port)
    {
        return connection_->Connect(ip, port);
    }

    void PbClient::SendRequest(const Request* req_ptr)
    {
        std::string req_str = GetPbCoder().EncodeRequest(req_ptr);       
        connection_->Write(req_str);
        // std::lock_guard<std::mutex> lock(write_mutex_);
        // bool is_writing = !write_queue_.empty();
        // write_queue_.emplace(req_str);
        // if(!is_writing)
        // {
        // }
    }

    void PbClient::SendHeartBeat()
    {
        Request* req_ptr = new HeartBeatRequest();
        SendRequest(req_ptr);
        delete req_ptr;
    }

    void PbClient::ReadResponse()
    {
        connection_->ReadLen(kTotalLenSize);
    }

    void PbClient::Close()
    {
        connection_->Close();
    }

    void PbClient::OnResponse(const ConnectionPtr &conn, const std::string& resp_str)
    {
        if (state_ == PbClientState::kReadTotalLen)
        {
            if (resp_str.size() != kTotalLenSize)
            {
                // log
                assert(false);
                // error
                return;
            }
            std::size_t req_len = 0;
            memcpy(&req_len, resp_str.c_str(),kTotalLenSize);
            connection_->ReadLen(req_len - kTotalLenSize);
            state_ = PbClientState::kReadRequest;
        }
        else if (state_ == PbClientState::kReadRequest)
        {
            Response* resp_ptr = GetPbCoder().DecodeResponse(resp_str);
            read_callback_(shared_from_this(), resp_ptr);
            state_ = PbClientState::kReadTotalLen;
            ReadResponse();
            delete resp_ptr;
        }
        else
        {
            // log
            //assert(false);
            // error
            return;
        }
    }

    void PbClient::OnRequest(const ConnectionPtr &conn, std::size_t bytes_transferred)
    {
        write_callback_(shared_from_this(), bytes_transferred);
        // std::lock_guard<std::mutex> lock(write_mutex_);
        // write_queue_.pop();
        // if(!write_queue_.empty())
        // {
        //     connection_->Write()
        // }
    }
}
