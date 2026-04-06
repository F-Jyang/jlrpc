#pragma once

#include <net/tcp_connection.h>
#include <net/net_data.h>
#include <net/pb/pb_coder.h>
// #include <interface/i_session.h>

namespace jl
{

    class PbSession;
    using PbSessionPtr = std::shared_ptr<PbSession>;

    using PbRequestCallback = std::function<void(const PbSessionPtr&, const RequestPtr &)>;
    using PbResponseCallback = std::function<void(const PbSessionPtr&, std::size_t bytes_transferred)>;
    using PbSessionCloseCallback = std::function<void(const PbSessionPtr&)>;

    enum class PbSessionState
    {
        kReadTotalLen = 0,
        kReadRequest,
    };

    class PbSession : public std::enable_shared_from_this<PbSession>
    {
    public:
        PbSession(net::tcp::socket &&socket);

        void Start();

        std::size_t GetId() const;

        void WriteResponse(const ResponsePtr& response);
        
        void SetRequestCallback(const PbRequestCallback &callback);

        void SetResponseCallback(const PbResponseCallback &callback);

        void SetCloseCallback(const PbSessionCloseCallback& callback);

        std::size_t GetTimeout() const;

        void SetTimeout(std::size_t timeout);

        /// @brief 关闭连接
        void Close();

        ~PbSession();
        
    private:

        void OnRequest(const ConnectionPtr &conn, const std::string &req_str);

        void OnResponse(const ConnectionPtr &conn, std::size_t bytes_transferred);

    private:
        std::size_t timeout_;
        std::size_t session_id_;
        ConnectionPtr connection_;
        std::atomic<PbSessionState> state_;
        PbRequestCallback request_callback_;
        PbResponseCallback response_callback_;
        PbSessionCloseCallback close_callback_;
    };
}