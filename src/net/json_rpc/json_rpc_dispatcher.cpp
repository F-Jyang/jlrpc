#include "json_rpc_dispatcher.h"
#include <utils/easy_log.hpp>

namespace jl
{

void JsonRpcDispatcher::RegisterMethod(const std::string& method_name, JsonRpcMethodHandler handler)
{
    method_handlers_[method_name] = std::move(handler);
}

void JsonRpcDispatcher::UnregisterMethod(const std::string& method_name)
{
    method_handlers_.erase(method_name);
}

Response* JsonRpcDispatcher::Dispatch(const Request* req)
{
    auto resp = new Response();

    if (!req)
    {
        resp->SetErrorCode(NetErrorCode::kInvaliadRequest);
        return resp;
    }

    std::string_view method = req->GetServiceFullName();
    std::string_view id = req->GetMsgId();
    std::string_view params_str = req->GetParam();

    resp->SetMsgId(id);

    if (method.empty())
    {
        resp->SetErrorCode(NetErrorCode::kInvaliadRequest);
        return resp;
    }

    auto it = method_handlers_.find(std::string(method));
    if (it == method_handlers_.end())
    {
        resp->SetErrorCode(NetErrorCode::kFunctionNotFound);
        return resp;
    }

    // 解析 params 为 JSON
    json params;
    if (!params_str.empty())
    {
        try
        {
            params = json::parse(params_str);
        }
        catch (const std::exception& e)
        {
            LOG_DEBUG << "JSON parse error: " << e.what();
            resp->SetErrorCode(NetErrorCode::kInvaliadRequest);
            return resp;
        }
    }

    // 调用处理函数
    NetErrorCode error = NetErrorCode::kNoError;
    json result = it->second(params, error);

    if (error != NetErrorCode::kNoError)
    {
        resp->SetErrorCode(error);
        return resp;
    }

    // 序列化结果
    try
    {
        std::string result_str = result.dump();
        resp->SetResult(result_str);
    }
    catch (const std::exception& e)
    {
        LOG_DEBUG << "JSON dump error: " << e.what();
        resp->SetErrorCode(NetErrorCode::kUnknown);
    }

    return resp;
}

bool JsonRpcDispatcher::HasMethod(const std::string& method_name) const
{
    return method_handlers_.find(method_name) != method_handlers_.end();
}

} // namespace jl
