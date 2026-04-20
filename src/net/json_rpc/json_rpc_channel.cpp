#include "json_rpc_channel.h"
#include <net/net_data.h>
#include <sstream>

namespace jl
{

JsonRpcChannel::JsonRpcChannel(asio::io_context& ioct, const std::string& ip, uint16_t port)
    : ioct_(ioct),
      ip_(ip),
      port_(port)
{
    client_ = std::make_shared<JsonClient>(ioct_, kDefaultBufferSize);
}

std::string JsonRpcChannel::CallMethod(const std::string& method, const std::string& params, int timeout)
{
    if (!client_->IsConnected() && !client_->Connect(ip_, port_))
    {
        return "";
    }

    // 生成唯一的 msg_id
    static int64_t g_id = 0;
    std::stringstream ss;
    ss << method << "_" << (++g_id);
    std::string msg_id = ss.str();

    Request req(msg_id, method, params);
    if (!client_->SendRequest(&req))
    {
        return "";
    }

    Response* resp = client_->ReadResponse();
    if (!resp)
    {
        return "";
    }

    if (resp->GetErrorCode() != NetErrorCode::kNoError)
    {
        delete resp;
        return "";
    }

    std::string result(resp->GetResult());
    delete resp;
    return result;
}

bool JsonRpcChannel::CallMethod(const std::string& method, const json& params, json& result, NetErrorCode& ec)
{
    std::string params_str = params.is_null() ? "" : params.dump();
    std::string result_str = CallMethod(method, params_str);
    if (result_str.empty())
    {
        ec = NetErrorCode::kUnknown;
        return false;
    }

    try
    {
        result = json::parse(result_str);
        ec = NetErrorCode::kNoError;
        return true;
    }
    catch (const std::exception&)
    {
        ec = NetErrorCode::kInvaliadRequest;
        return false;
    }
}

void JsonRpcChannel::AsyncCallMethod(const std::string& method, const std::string& params, JsonRpcResponseCallback callback)
{
    // TODO: 实现异步版本
    std::string result = CallMethod(method, params);
    if (callback)
    {
        NetErrorCode ec = result.empty() ? NetErrorCode::kUnknown : NetErrorCode::kNoError;
        callback(result, ec);
    }
}

void JsonRpcChannel::Close()
{
    if (client_)
    {
        client_->Close();
    }
}

} // namespace jl
