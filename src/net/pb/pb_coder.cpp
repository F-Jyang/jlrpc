#include "pb_coder.h"
#include <net/net_data.h>
#include <iostream>
#include <assert.h>
#include <string_view>

namespace jl
{
    std::string PbCoder::EncodeRequest(const Request* req_ptr)
    {
        int32_t total_len = 4 * sizeof(int32_t) + req_ptr->GetSize();
        std::string_view msg_id = req_ptr->GetMsgId(), param = req_ptr->GetParam(), service_full_name = req_ptr->GetServiceFullName();
        int32_t msg_id_len = msg_id.size(), param_len = param.size(), service_full_name_len = service_full_name.size();
        std::string req_with_len(total_len, '\0');
        int idx = 0;
        memcpy(req_with_len.data(), &total_len, sizeof(int32_t)); // 固定4Byte的总长度，用于提示对方包的长度消息
        idx += sizeof(int32_t);
        memcpy(req_with_len.data() + idx, &msg_id_len, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(req_with_len.data() + idx, &service_full_name_len, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(req_with_len.data() + idx, &param_len, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(req_with_len.data() + idx, msg_id.data(), msg_id_len);
        idx += msg_id_len;
        memcpy(req_with_len.data() + idx, service_full_name.data(), service_full_name_len);
        idx += service_full_name_len;
        memcpy(req_with_len.data() + idx, param.data(), param_len);
        return req_with_len;
    }

    Request* PbCoder::DecodeRequest(std::string_view req_str)
    {
        if (req_str.size() < 3 * sizeof(int32_t) || req_str.size() > kMaxRequestSize)
        {
            // log
            return nullptr;
        }
        auto req = new Request();
        int32_t msg_id_len = 0, param_len = 0, service_full_name_len = 0;
        int idx = 0;
        memcpy(&msg_id_len, req_str.data(), sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(&service_full_name_len, req_str.data() + idx, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(&param_len, req_str.data() + idx, sizeof(int32_t));
        idx += sizeof(int32_t);
        if (msg_id_len + service_full_name_len + param_len != req_str.size() - idx)
        {
            // log
            return nullptr;
        }
        req->SetMsgId(req_str.substr(idx, msg_id_len));
        idx += msg_id_len;
        req->SetServiceFullName(req_str.substr(idx, service_full_name_len));
        idx += service_full_name_len;
        req->SetParam(req_str.substr(idx, param_len));
        return req;
    }

    std::string PbCoder::EncodeResponse(const Response* resp_ptr)
    {
        int32_t total_len = 3 * sizeof(int32_t) + resp_ptr->GetSize();
        std::string_view msg_id = resp_ptr->GetMsgId(), result = resp_ptr->GetResult();
        int32_t error_code = static_cast<int32_t>(resp_ptr->GetErrorCode());
        int32_t msg_id_len = msg_id.size(), result_len = result.size();
        std::string resp_with_len(total_len, '\0');
        int idx = 0;
        memcpy(resp_with_len.data(), &total_len, sizeof(int32_t)); // 固定4Byte的总长度，用于提示对方包的长度消息
        idx += sizeof(int32_t);
        memcpy(resp_with_len.data() + idx, &msg_id_len, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(resp_with_len.data() + idx, &result_len, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(resp_with_len.data() + idx, msg_id.data(), msg_id_len);
        idx += msg_id_len;
        memcpy(resp_with_len.data() + idx, result.data(), result_len);
        idx += result_len;
        memcpy(resp_with_len.data() + idx, &error_code, sizeof(int32_t));
        return resp_with_len;
    }

    Response* PbCoder::DecodeResponse(std::string_view resp_str)
    {
        if (resp_str.size() < 2 * sizeof(int32_t) || resp_str.size() > kMaxResponseSize)
        {
            // log
            return nullptr;
        }
        auto resp = new Response();
        int32_t msg_id_len = 0, result_len = 0;
        int idx = 0;
        memcpy(&msg_id_len, resp_str.data(), sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(&result_len, resp_str.data() + idx, sizeof(int32_t));
        idx += sizeof(int32_t);
        if (msg_id_len + result_len + sizeof(int32_t) /*error_code */ != resp_str.size() - idx)
        {
            // log
            return nullptr;
        }
        resp->SetMsgId(resp_str.substr(idx, msg_id_len));
        idx += msg_id_len;
        resp->SetResult(resp_str.substr(idx, result_len));
        idx += result_len;
        int32_t error_code;
        memcpy(&error_code, resp_str.data() + idx, sizeof(int32_t));
        resp->SetErrorCode(static_cast<NetErrorCode>(error_code));
        return resp;
    }

    Coder<PbCoder> &GetPbCoder()
    {
        static Coder<PbCoder> coder;
        return coder;
    }
}
