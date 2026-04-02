#include "pd_data.h"

#include <cstring>

namespace jl
{
    void PbRequest::SetMsgId(const std::string &msg_id)
    {
        msg_id_ = msg_id;
    }

    std::string PbRequest::GetMsgId() const
    {
        return msg_id_;
    }

    void PbRequest::SetServiceFullName(const std::string &service_full_name)
    {
        service_full_name_ = service_full_name;
    }

    std::string PbRequest::GetServiceFullName() const
    {
        return service_full_name_;
    }

    void PbRequest::SetParam(const std::string &param)
    {
        pb_data_ = param;
    }

    std::string PbRequest::GetParam() const
    {
        return pb_data_;
    }

    int32_t PbRequest::GetSize() const
    {
        return sizeof(msg_id_len_) + msg_id_len_ +
               sizeof(service_full_name_len_) + service_full_name_len_ +
               sizeof(pb_data_len_) + pb_data_len_;
    }

    bool PbRequest::ParseFromString(const std::string &req_str)
    {
        // min size is 16Bytes
        if (req_str.size() < 16)
        {
            // log
            return false;
        }
        int32_t total_len = req_str.size();
        int idx = 0;
        memcpy(&msg_id_len_, req_str.c_str() + idx, sizeof(msg_id_len_));
        idx += sizeof(msg_id_len_);
        memcpy(&service_full_name_len_, req_str.c_str() + idx, sizeof(service_full_name_len_));
        idx += sizeof(service_full_name_len_);
        memcpy(&pb_data_len_, req_str.c_str() + idx, sizeof(pb_data_len_));
        idx += sizeof(pb_data_len_);
        if (total_len != sizeof(msg_id_len_) + msg_id_len_ +
                             sizeof(service_full_name_len_) + service_full_name_len_ +
                             sizeof(pb_data_len_) + pb_data_len_)
        {
            // log
            return false;
        }
        msg_id_ = std::string(req_str.begin() + idx, req_str.begin() + idx + msg_id_len_);
        idx += msg_id_len_;
        service_full_name_ = std::string(req_str.begin() + idx, req_str.begin() + idx + service_full_name_len_);
        idx += service_full_name_len_;
        pb_data_ = std::string(req_str.begin() + idx, req_str.begin() + idx + pb_data_len_);
        return true;
    }

    std::string PbRequest::SerializeToString()
    {
        int32_t total_len = 3 * sizeof(int32_t) + msg_id_.size() + service_full_name_.size() + pb_data_.size();
        msg_id_len_ = msg_id_.size();
        service_full_name_len_ = service_full_name_.size();
        pb_data_len_ = pb_data_.size();
        std::string result(total_len, 0);
        int idx = 0;
        memcpy(result.data() + idx, &msg_id_len_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, &service_full_name_len_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, &pb_data_len_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, msg_id_.data(), msg_id_len_);
        idx += msg_id_len_;
        memcpy(result.data() + idx, service_full_name_.data(), service_full_name_len_);
        idx += service_full_name_len_;
        memcpy(result.data() + idx, pb_data_.data(), pb_data_len_);
        return result;
    }

    void PbResponse::SetMsgId(const std::string &msg_id)
    {
        msg_id_ = msg_id;
    }

    std::string PbResponse::GetMsgId() const
    {
        return msg_id_;
    }

    void PbResponse::SetResult(const std::string &result)
    {
        pb_data_ = result;
    }

    std::string PbResponse::GetResult() const
    {
        return pb_data_;
    }

    int32_t PbResponse::GetSize() const
    {
        return sizeof(msg_id_len_) + msg_id_len_ +
               sizeof(pb_data_len_) + pb_data_len_ +
               sizeof(error_code_);
    }

    void PbResponse::SetErrorCode(DataErrorCode ec)
    {
        error_code_ = static_cast<int32_t>(ec);
    }

    bool PbResponse::ParseFromString(const std::string &req_str)
    {
        // min size is 16Bytes
        if (req_str.size() < 16)
        {
            // log
            return false;
        }
        int32_t total_len = req_str.size();
        int idx = 0;
        memcpy(&msg_id_len_, req_str.c_str() + idx, sizeof(msg_id_len_));
        idx += sizeof(msg_id_len_);
        memcpy(&pb_data_len_, req_str.c_str() + idx, sizeof(pb_data_len_));
        idx += sizeof(pb_data_len_);
        memcpy(&error_code_, req_str.c_str() + idx, sizeof(error_code_));
        idx += sizeof(error_code_);
        if (total_len != sizeof(msg_id_len_) + msg_id_len_ +
                             sizeof(pb_data_len_) + pb_data_len_ +
                             sizeof(error_code_))
        {
            // log
            return false;
        }
        msg_id_ = std::string(req_str.begin() + idx, req_str.begin() + idx + msg_id_len_);
        idx += msg_id_len_;
        pb_data_ = std::string(req_str.begin() + idx, req_str.begin() + idx + pb_data_len_);
        return true;
    }

    std::string PbResponse::SerializeToString()
    {
        int32_t total_len = 3 * sizeof(int32_t) + msg_id_.size() + pb_data_.size();
        msg_id_len_ = msg_id_.size();
        pb_data_len_ = pb_data_.size();
        std::string result(total_len, 0);
        int idx = 0;
        memcpy(result.data() + idx, &msg_id_len_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, &pb_data_len_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, &error_code_, sizeof(int32_t));
        idx += sizeof(int32_t);
        memcpy(result.data() + idx, msg_id_.data(), msg_id_len_);
        idx += msg_id_len_;
        memcpy(result.data() + idx, pb_data_.data(), pb_data_len_);
        return result;
    }
}
