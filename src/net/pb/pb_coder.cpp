#include "pb_coder.h"
#include <net/pb/pb_data.h>
#include <iostream>
#include <assert.h>

namespace jl
{
    std::string PbCoder::EncodeRequest(const RequestPtr &req_ptr)
    {
        if(!req_ptr)
        {
            // nullptr error
            return "xxx";
        }        
        int32_t total_len = sizeof(int32_t) + req_ptr->GetSize();
        if(total_len > kMaxRequestSize)
        {
            // length error
            return "xxx";
        }

        std::string req_with_len(total_len, '\0');
        std::string req_str = req_ptr->SerializeToString();
        memcpy(req_with_len.data(), &total_len, sizeof(int32_t));
        memcpy(req_with_len.data() + sizeof(int32_t), req_str.c_str(), req_str.size());
        return req_with_len;
    }

    RequestPtr PbCoder::DecodeRequest(asio::streambuf &buffer)
    {
        std::string req_str(buffer.size(), '\0');
        std::istream is(&buffer);
        is.read(req_str.data(), buffer.size());
        if (req_str.size() == 0 || req_str.size() > kMaxRequestSize)
        {
            // log
            return nullptr;
        }
        auto req = std::make_shared<PbRequest>();
        if (!req->ParseFromString(req_str))
        {
            // log
            return nullptr;
        }
        return req;
    }

    std::string PbCoder::EncodeResponse(const ResponsePtr &resp_ptr)
    {
        if(!resp_ptr)
        {
            // nullptr error
            return "xxx";
        }        
        int32_t total_len = sizeof(int32_t) + resp_ptr->GetSize();
        if(total_len > kMaxResponseSize)
        {
            // length error
            return "xxx";
        }
        std::string resp_with_len(total_len, '\0');
        std::string resp_str = resp_ptr->SerializeToString();
        memcpy(resp_with_len.data(), &total_len, sizeof(int32_t));
        memcpy(resp_with_len.data() + sizeof(int32_t), resp_str.c_str(), resp_str.size());
        return resp_with_len;
    }

    ResponsePtr PbCoder::DecodeResponse(asio::streambuf &buffer)
    {
        std::string resp_str(buffer.size(), '\0');
        std::istream is(&buffer);
        is.read(resp_str.data(), buffer.size());
        if (resp_str.size() == 0 || resp_str.size() > kMaxRequestSize)
        {
            // log
            return nullptr;
        }
        auto resp = std::make_shared<PbResponse>();
        if (!resp->ParseFromString(resp_str))
        {
            // log
            return nullptr;
        }
        return resp;
    }
}
