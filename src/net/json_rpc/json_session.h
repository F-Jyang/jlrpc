/// @brief JSON-RPC Session
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <interface/i_session.h>
#include <net/tcp_connection.h>
#include <net/net_data.h>
#include <net/json_rpc/http_coder.h>
#include <net/json_rpc/json_coder.h>
#include <string>

namespace jl
{

    class JsonSession;
    using JsonSessionPtr = std::shared_ptr<JsonSession>;

    using JsonRequestCallback = std::function<void(const JsonSessionPtr &, const Request *, const std::error_code &ec)>;
    using JsonResponseCallback = std::function<void(const JsonSessionPtr &, std::size_t bytes_transferred, const std::error_code &ec)>;
    using JsonSessionCloseCallback = std::function<void(const JsonSessionPtr &, const std::error_code &ec)>;

    enum class JsonSessionState
    {
        kReadHttpHeader = 0,
        kReadHttpBody,
        kProcessRequest,
    };

    class JsonSession : public ISession
    {
    public:
        JsonSession(const ConnectionPtr &conn);

        void Start();

        std::size_t GetId() const;

        void WriteResponse(const Response *response);

        void SetRequestCallback(const JsonRequestCallback &callback);

        void SetResponseCallback(const JsonResponseCallback &callback);

        void SetCloseCallback(const JsonSessionCloseCallback &callback);

        const asio::any_io_executor &GetIoExecutor() override;

        void Close();

        ~JsonSession();

    private:
        void OnRequest(const SessionPtr &session, std::string_view req_str, const std::error_code &ec) override;

        void OnResponse(const SessionPtr &session, std::size_t bytes_transferred, const std::error_code &ec) override;

        // void OnResponse(const JsonSessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec) override;

        void ReadHttpHeader();
        void ReadHttpBody(size_t body_size);
        void ProcessRequest(const std::string &body);

    private:
        std::size_t session_id_;
        ConnectionPtr connection_;
        std::atomic<JsonSessionState> state_;
        JsonRequestCallback request_callback_;
        JsonResponseCallbaReck response_callback_;
        JsonSessionCloseCallback close_callback_;
        std::string http_req_data_;

        std::chrono::steady_clock::time_point start_;
    };

} // namespace jl
