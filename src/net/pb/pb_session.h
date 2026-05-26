#pragma once

#include <interface/i_session.h>
#include <net/tcp_connection.h>
#include <net/net_data.h>
#include <net/pb/pb_coder.h>
#include <string>
// #include <interface/i_session.h>

namespace jl
{

    class PbSession;
    using PbSessionPtr = std::shared_ptr<PbSession>;

    using PbRequestCallback = std::function<void(const PbSessionPtr&, const Request*, const std::error_code& ec)>;
    using PbResponseCallback = std::function<void(const PbSessionPtr&, std::size_t bytes_transferred, const std::error_code& ec)>;
    using PbSessionCloseCallback = std::function<void(const PbSessionPtr&, const std::error_code& ec)>;

    enum class PbSessionState
    {
        kReadTotalLen = 0,
        kReadRequest,
    };

    class PbSession : public ISession
    {
    public:
		// PbSession(asio::io_context& ioct, net::tcp::socket&& socket);
		
        PbSession(const ConnectionPtr& conn);

        void Start();

        std::size_t GetId() const;

        void WriteResponse(const Response* response);
        
        void SetRequestCallback(const RequestCallback &callback);

        void SetResponseCallback(const ResponseCallback &callback);

        void SetCloseCallback(const SessionCloseCallback& callback);

        const asio::any_io_executor& GetIoExecutor() override;

        /// @brief 关闭连接
        void Close();

        ~PbSession();
        
    private:

        void OnRequest(const SessionPtr& session, std::string_view req_str, const std::error_code& ec);

        void OnResponse(const SessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec);

    private:
        std::size_t session_id_;
        ConnectionPtr connection_;
        std::atomic<PbSessionState> state_;

        // for test
        std::chrono::steady_clock::time_point start_;
    };
}