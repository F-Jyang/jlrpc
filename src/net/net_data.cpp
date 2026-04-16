#include "net_data.h"

#include <cstring>

namespace jl
{
    Request::Request() noexcept
        : msg_id_(""),
          service_full_name_(""),
          param_("")
    {
    }

    Request::Request(std::string_view msg_id, std::string_view service_full_name, std::string_view param) noexcept
        : msg_id_(msg_id),
          service_full_name_(service_full_name),
          param_(param)
    {
    }

    Request::Request(std::string_view service_full_name, std::string_view param) noexcept
        : service_full_name_(service_full_name),
          param_(param)

    {
    }

    Request::Request(Request &&other) noexcept
        : msg_id_(std::move(other.msg_id_)),
          param_(std::move(other.param_)),
          service_full_name_(std::move(other.service_full_name_))
    {
    }

    void Request::SetMsgId(std::string_view msg_id)
    {
        msg_id_ = msg_id;
    }

    std::string_view Request::GetMsgId() const
    {
        return msg_id_;
    }

    void Request::SetServiceFullName(std::string_view service_full_name)
    {
        service_full_name_ = service_full_name;
    }

    std::string_view Request::GetServiceFullName() const
    {
        return service_full_name_;
    }

    void Request::SetParam(const std::string_view &param)
    {
        param_ = param;
    }

    std::string_view Request::GetParam() const
    {
        return param_;
    }

    int32_t Request::GetSize() const
    {
        return msg_id_.size() + service_full_name_.size() + param_.size();
    }

    // bool Request::ParseFromString(const std::string &req_str)
    // {
    //     // // min size is 16Bytes
    //     // if (req_str.size() < 16)
    //     // {
    //     //     // log
    //     //     return false;
    //     // }
    //     // int32_t total_len = req_str.size();
    //     // int idx = 0;
    //     // memcpy(&msg_id_len_, req_str.c_str() + idx, sizeof(msg_id_len_));
    //     // idx += sizeof(msg_id_len_);
    //     // memcpy(&service_full_name_len_, req_str.c_str() + idx, sizeof(service_full_name_len_));
    //     // idx += sizeof(service_full_name_len_);
    //     // memcpy(&pb_data_len_, req_str.c_str() + idx, sizeof(pb_data_len_));
    //     // idx += sizeof(pb_data_len_);
    //     // if (total_len != sizeof(msg_id_len_) + msg_id_len_ +
    //     //                      sizeof(service_full_name_len_) + service_full_name_len_ +
    //     //                      sizeof(pb_data_len_) + pb_data_len_)
    //     // {
    //     //     // log
    //     //     return false;
    //     // }
    //     // msg_id_ = std::string(req_str.begin() + idx, req_str.begin() + idx + msg_id_len_);
    //     // idx += msg_id_len_;
    //     // service_full_name_ = std::string(req_str.begin() + idx, req_str.begin() + idx + service_full_name_len_);
    //     // idx += service_full_name_len_;
    //     // pb_data_ = std::string(req_str.begin() + idx, req_str.begin() + idx + pb_data_len_);
    //     return true;
    // }

    // std::string Request::SerializeToString()
    // {
    //     std::string result;
    //     // int32_t total_len = 3 * sizeof(int32_t) + msg_id_.size() + service_full_name_.size() + pb_data_.size();
    //     // msg_id_len_ = msg_id_.size();
    //     // service_full_name_len_ = service_full_name_.size();
    //     // pb_data_len_ = pb_data_.size();
    //     // std::string result(total_len, 0);
    //     // int idx = 0;
    //     // memcpy(result.data() + idx, &msg_id_len_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, &service_full_name_len_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, &pb_data_len_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, msg_id_.data(), msg_id_len_);
    //     // idx += msg_id_len_;
    //     // memcpy(result.data() + idx, service_full_name_.data(), service_full_name_len_);
    //     // idx += service_full_name_len_;
    //     // memcpy(result.data() + idx, pb_data_.data(), pb_data_len_);
    //     return result;
    // }

    Response::Response() noexcept
        : msg_id_(""),
          result_(""),
          error_code_(NetErrorCode::kNoError)
    {
    }

    Response::Response(Response &&other) noexcept
        : error_code_(other.error_code_),
          msg_id_(std::move(other.msg_id_)),
          result_(std::move(other.result_))
    {
    }

    Response::Response(std::string_view msg_id, std::string_view result, NetErrorCode error_code) noexcept
        : msg_id_(msg_id),
          result_(result),
          error_code_(error_code)
    {
    }

    void Response::SetMsgId(std::string_view msg_id)
    {
        msg_id_ = msg_id;
    }

    std::string_view Response::GetMsgId() const
    {
        return msg_id_;
    }

    void Response::SetResult(std::string_view result)
    {
        result_ = result;
    }

    std::string_view Response::GetResult() const
    {
        return result_;
    }

    int32_t Response::GetSize() const
    {
        return msg_id_.size() +
               result_.size() +
               sizeof(error_code_);
    }

    void Response::SetErrorCode(NetErrorCode ec)
    {
        error_code_ = ec;
    }

    NetErrorCode Response::GetErrorCode() const
    {
        return error_code_;
    }

    // bool Response::ParseFromString(const std::string &req_str)
    // {
    //     // min size is 16Bytes
    //     // if (req_str.size() < 16)
    //     // {
    //     //     // log
    //     //     return false;
    //     // }
    //     // int32_t total_len = req_str.size();
    //     // int idx = 0;
    //     // memcpy(&msg_id_len_, req_str.c_str() + idx, sizeof(msg_id_len_));
    //     // idx += sizeof(msg_id_len_);
    //     // memcpy(&pb_data_len_, req_str.c_str() + idx, sizeof(pb_data_len_));
    //     // idx += sizeof(pb_data_len_);
    //     // memcpy(&error_code_, req_str.c_str() + idx, sizeof(error_code_));
    //     // idx += sizeof(error_code_);
    //     // if (total_len != sizeof(msg_id_len_) + msg_id_len_ +
    //     //                      sizeof(pb_data_len_) + pb_data_len_ +
    //     //                      sizeof(error_code_))
    //     // {
    //     //     // log
    //     //     return false;
    //     // }
    //     // msg_id_ = std::string(req_str.begin() + idx, req_str.begin() + idx + msg_id_len_);
    //     // idx += msg_id_len_;
    //     // pb_data_ = std::string(req_str.begin() + idx, req_str.begin() + idx + pb_data_len_);
    //     return true;
    // }

    // std::string Response::SerializeToString()
    // {
    //     std::string result;
    //     // int32_t total_len = 3 * sizeof(int32_t) + msg_id_.size() + pb_data_.size();
    //     // msg_id_len_ = msg_id_.size();
    //     // pb_data_len_ = pb_data_.size();
    //     // std::string result(total_len, 0);
    //     // int idx = 0;
    //     // memcpy(result.data() + idx, &msg_id_len_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, &pb_data_len_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, &error_code_, sizeof(int32_t));
    //     // idx += sizeof(int32_t);
    //     // memcpy(result.data() + idx, msg_id_.data(), msg_id_len_);
    //     // idx += msg_id_len_;
    //     // memcpy(result.data() + idx, pb_data_.data(), pb_data_len_);
    //     return result;
    // }

    HeartBeatRequest::HeartBeatRequest() noexcept : Request("HEARTBEAT", "PING", "")
    {
    }

    HeartBeatResponse::HeartBeatResponse() noexcept : Response("HEARTBEAT", "PONG")
    {
    }
}
