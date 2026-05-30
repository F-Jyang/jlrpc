#include "pb_client.h"

namespace jl
{

    jl::PbClient::PbClient(asio::io_context &ioct, std::size_t max_buffer_size, std::size_t timeout )
        : ioct_(ioct),
          timeout_(timeout)
    {
        connection_ = std::make_shared<SyncConnection>(ioct, max_buffer_size);
        // connection_->SetTimeoutCallback(
        //     [&](const ConnectionPtr &conn, const std::error_code &ec)
        //     {
        //         LOG_DEBUG << "client timeout";
        //         conn->Cancel();
        //     });
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
        std::size_t n = connection_->SyncWrite(req_str, timeout_, ec);
        assert(!ec);
        return n == req_str.size();
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
        std::error_code ec;
        std::string resp_str = connection_->SyncReadLen(kTotalLenSize, timeout_, ec);
        LOG_DEBUG << "Read response len finish";
        if (ec) // 远程连接断开后会触发
        {
            LOG_DEBUG << "ec: " << ec.message();
            return nullptr;
        }
        std::size_t resp_len = 0;
        memcpy(&resp_len, resp_str.c_str(), kTotalLenSize);
        resp_str = connection_->SyncReadLen(resp_len - kTotalLenSize, timeout_, ec);
        if (ec)
        {
            LOG_DEBUG << ec.message();
            return nullptr;
        }
        Response *resp_ptr = GetPbCoder().DecodeResponse(resp_str);
        return resp_ptr;
    }

    void PbClient::Close()
    {
        connection_->Close();
    }

    void PbClient::OnTimeout(const std::error_code &ec)
    {
        if (!ec)
        {
            connection_->Cancel();
        }
        else
        {
            LOG_DEBUG << __FUNCTION__ << ":" << ec.message();
        }
    }

    void PbClient::SetTimeout(int secs)
    {
        timeout_ = secs;
    }
}
