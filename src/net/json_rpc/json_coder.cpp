#include "json_coder.h"
#include <net/net_data.h>
#include <sstream>
#include <iomanip>

// 使用 nlohmann/json
#include <nlohmann/json.hpp>

namespace jl
{
using json = nlohmann::json;

std::string JsonCoder::EncodeRequest(const Request* req_ptr)
{
    JsonRpcRequest req;
    req.method = std::string(req_ptr->GetServiceFullName());
    req.params = std::string(req_ptr->GetParam());
    req.id = std::string(req_ptr->GetMsgId());

    json j;
    j["jsonrpc"] = "2.0";
    j["method"] = req.method;
    if (!req.params.empty())
    {
        j["params"] = json::parse(req.params);
    }
    else
    {
        j["params"] = json::object();
    }
    j["id"] = req.id;

    return j.dump();
}

Request* JsonCoder::DecodeRequest(std::string_view req_str)
{
    try
    {
        auto j = json::parse(req_str);

        if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0")
            return nullptr;
        if (!j.contains("method"))
            return nullptr;

        std::string id;
        if (j.contains("id"))
            id = j["id"].get<std::string>();

        std::string params;
        if (j.contains("params"))
            params = j["params"].dump();

        auto req = new Request(id, j["method"].get<std::string>(), params);
        return req;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

std::string JsonCoder::EncodeResponse(const Response* resp_ptr)
{
    if (resp_ptr->GetErrorCode() != NetErrorCode::kNoError)
    {
        return BuildJsonRpcErrorResponse(
            std::string(resp_ptr->GetMsgId()),
            static_cast<int32_t>(resp_ptr->GetErrorCode()),
            GetPbErrorMsg(resp_ptr->GetErrorCode()));
    }
    else
    {
        return BuildJsonRpcResponse(std::string(resp_ptr->GetMsgId()), std::string(resp_ptr->GetResult()));
    }
}

Response* JsonCoder::DecodeResponse(std::string_view resp_str)
{
    try
    {
        auto j = json::parse(resp_str);

        if (!j.contains("jsonrpc") || j["jsonrpc"] != "2.0")
            return nullptr;

        std::string id;
        if (j.contains("id"))
            id = j["id"].get<std::string>();

        if (j.contains("error"))
        {
            // 错误响应
            int32_t code = j["error"].value("code", -32000);
            std::string msg = j["error"].value("message", "Unknown error");
            auto resp = new Response(id, "", static_cast<NetErrorCode>(code));
            return resp;
        }
        else if (j.contains("result"))
        {
            // 成功响应
            std::string result = j["result"].dump();
            auto resp = new Response(id, result, NetErrorCode::kNoError);
            return resp;
        }

        return nullptr;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

JsonRpcRequest* JsonCoder::ParseJsonRpcRequest(std::string_view req_str)
{
    try
    {
        auto j = json::parse(req_str);

        auto req = new JsonRpcRequest();
        req->jsonrpc = j.value("jsonrpc", "");
        req->method = j.value("method", "");
        req->id = j.value("id", "");

        if (j.contains("params"))
            req->params = j["params"].dump();

        return req;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

std::string JsonCoder::BuildJsonRpcResponse(const std::string& id, const std::string& result)
{
    json j;
    j["jsonrpc"] = "2.0";
    if (result.empty())
    {
        j["result"] = nullptr;
    }
    else
    {
        j["result"] = json::parse(result);
    }
    j["id"] = id;
    return j.dump();
}

std::string JsonCoder::BuildJsonRpcErrorResponse(const std::string& id, int32_t error_code, const std::string& error_message)
{
    json j;
    j["jsonrpc"] = "2.0";
    j["error"]["code"] = error_code;
    j["error"]["message"] = error_message;
    j["id"] = id;
    return j.dump();
}

Coder<JsonCoder>& GetJsonCoder()
{
    static Coder<JsonCoder> coder;
    return coder;
}

} // namespace jl
