#include "pb_client.h"

namespace jl
{
    jl::PbClient::PbClient(asio::io_context &ioct, std::size_t max_buffer_size)
        : state_(PbClientState::kReadTotalLen),
          ioct_(ioct),
          work_guard_(asio::make_work_guard(ioct)),
          timer_(ioct)
    {
        connection_ = std::make_shared<TcpConnection>(ioct, max_buffer_size);
    }

    bool PbClient::Connect(const std::string &ip, unsigned short port)
    {
        return connection_->Connect(ip, port);
    }

    bool PbClient::IsConnected()
    {
        return connection_->IsConnected();
    }

    bool PbClient::SendRequest(const Request *req_ptr)
    {
        std::string req_str = GetPbCoder().EncodeRequest(req_ptr);
        std::error_code ec;
        std::size_t n = connection_->SyncWrite(req_str, ec);
        assert(!ec);
        return n == req_str.size();
        // std::error_code flag = asio::error::would_block;
        // std::size_t n;
        // connection_->SetWriteCallback(
        //     [&](const ConnectionPtr &, std::size_t bytes_transferred, const std::error_code &ec)
        //     {
        //         n = bytes_transferred;
        //         // ec.clear();
        //         flag = ec;
        //     });
        // connection_->Write(req_str);
        // do
        //     ioct_.run_one();
        // while (flag == asio::error::would_block);
        // return n == req_str.size();
    }

    bool PbClient::SendHeartBeat()
    {
        Request *req_ptr = new HeartBeatRequest();
        bool res = SendRequest(req_ptr);
        delete req_ptr;
        return res;
    }

    Response *PbClient::ReadResponse()
    {
        // std::string resp_len_str = connection_->SyncReadLen(kTotalLenSize);
        std::error_code ec;
        std::string resp_str = connection_->SyncReadLen(kTotalLenSize, ec);
        assert(!ec);
        std::size_t resp_len = 0;
        memcpy(&resp_len, resp_str.c_str(), kTotalLenSize);
        resp_str = connection_->SyncReadLen(resp_len - kTotalLenSize + 1, ec);
        if (ec)
        {
            LOG_DEBUG << ec.message();
            return nullptr;
        }
        Response *resp_ptr = GetPbCoder().DecodeResponse(resp_str);
        return resp_ptr;
        // connection_->ReadLen(kTotalLenSize);
        // std::string resp_str;
        // connection_->SetReadCallback(
        //     [&](const ConnectionPtr &conn, std::string_view data, const std::error_code &ec)
        //     {
        //         resp_str = data;
        //         flag = ec;
        //         // ec.clear();
        //     });
        // do
        //     ioct_.run_one();
        // while (flag == asio::error::would_block);
        // std::size_t resp_len = 0;
        // memcpy(&resp_len, resp_str.c_str(), kTotalLenSize);
        // // std::string resp_str = connection_->SyncReadLen(resp_len - kTotalLenSize);
        // connection_->ReadLen(resp_len - kTotalLenSize);
        // flag = asio::error::would_block;
        // do
        //     ioct_.run_one();
        // while (flag == asio::error::would_block);
        // Response *resp_ptr = GetPbCoder().DecodeResponse(resp_str);
        // return resp_ptr;
    }

    void PbClient::Close()
    {
        connection_->Close();
    }

    void PbClient::OnTimeout(const std::error_code &ec)
    {
        if (!ec)
        {
            LOG_DEBUG << __FUNCTION__ << ":" << ec.message();
            // connection_->Close();
            connection_->Cancel();
        }
        else
        {
            LOG_DEBUG << __FUNCTION__ << ":" << ec.message();
        }
    }

    void PbClient::SetTimeout(int secs)
    {
        LOG_DEBUG << "SetTimeout called, timer expiry before: "
                  << std::to_string(timer_.expiry().time_since_epoch().count());
        auto now = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        // timer_ = asio::steady_timer(ioct_);
        // timer_.expires_at(asio::steady_timer::time_point::max()); // 先设到无限远
        timer_.expires_after(std::chrono::seconds(secs));
        // timer_.expires_at(now);

        LOG_DEBUG << "SetTimeout new expiry, now is: "
                  << std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(timer_.expiry() - now).count())
                  << "ms from now";
        // timer_.expires_after(std::chrono::seconds(secs));
        std::weak_ptr<PbClient> weak(shared_from_this());
        timer_.async_wait(
            [weak](const std::error_code &ec)
            {
                auto self = weak.lock();
                if (self)
                    self->OnTimeout(ec);
            });
    }

    // void PbClient::OnResponse(const ConnectionPtr &conn, const std::string& resp_str)
    //{
    //     if (state_ == PbClientState::kReadTotalLen)
    //     {
    //         if (resp_str.size() != kTotalLenSize)
    //         {
    //             // log
    //             assert(false);
    //             // error
    //             return;
    //         }
    //         std::size_t req_len = 0;
    //         memcpy(&req_len, resp_str.c_str(),kTotalLenSize);
    //         connection_->ReadLen(req_len - kTotalLenSize);
    //         state_ = PbClientState::kReadRequest;
    //     }
    //     else if (state_ == PbClientState::kReadRequest)
    //     {
    //         Response* resp_ptr = GetPbCoder().DecodeResponse(resp_str);
    //         read_callback_(shared_from_this(), resp_ptr);
    //         state_ = PbClientState::kReadTotalLen;
    //         ReadResponse();
    //         delete resp_ptr;
    //     }
    //     else
    //     {
    //         // log
    //         //assert(false);
    //         // error
    //         return;
    //     }
    // }

    // void PbClient::OnRequest(const ConnectionPtr &conn, std::size_t bytes_transferred)
    //{
    //     write_callback_(shared_from_this(), bytes_transferred);
    //     // std::lock_guard<std::mutex> lock(write_mutex_);
    //     // write_queue_.pop();
    //     // if(!write_queue_.empty())
    //     // {
    //     //     connection_->Write()
    //     // }
    // }
}
