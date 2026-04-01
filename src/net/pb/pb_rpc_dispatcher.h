#pragma once
#include <net/pb/pb_data.h>
#include <interface/i_session.h>
#include <map>
#include <string>
#include <google/protobuf/service.h>

namespace jl
{
    using PbServicePtr = std::shared_ptr<google::protobuf::Service>;

    class PbRpcDispatcher
    {
    public:
        void RegisterService(const std::string &name, const PbServicePtr &service_ptr);

        void Dispatch(const SessionPtr &session_ptr, const RequestPtr &req_ptr);

        void UnRegisterService(const std::string &name);

    private:
        bool GetServiceAndMethod(const std::string& service_full_name, std::string& service_name, std::string& method_name);    

    private:
        std::map<std::string, PbServicePtr> service_map_;
    };
}