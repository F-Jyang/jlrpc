#include "pb_rpc_dispatcher.h"
#include <net/pb/pb_rpc_closure.h>
#include <net/pb/pb_rpc_controller.h>
#include <google/protobuf/message.h>

namespace jl
{
    void PbRpcDispatcher::RegisterService(const std::string &name, const PbServicePtr &service_ptr)
    {
        if (service_ptr)
        {
            service_map_[name] = service_ptr;
        }
    }

    ResponsePtr PbRpcDispatcher::Dispatch(const RequestPtr &req_ptr)
    {
        namespace Pb = google::protobuf;

		ResponsePtr resp_ptr = std::make_shared<PbResponse>(req_ptr->GetMsgId(), "", DataErrorCode::kNoError);
		std::string service_full_name = req_ptr->GetServiceFullName();
        std::string serivec_name, method_name;
        if (!GetServiceAndMethod(service_full_name, serivec_name, method_name))
        {
            // param error
            resp_ptr->SetErrorCode(DataErrorCode::kFunctionNotFound);
            return resp_ptr;
        }
        auto &&service_pair = service_map_.find(serivec_name);
        if (service_pair == service_map_.end())
        {
            // service unregister
            resp_ptr->SetErrorCode(DataErrorCode::kFunctionNotFound);
			return resp_ptr;
        }
        PbServicePtr service_ptr = service_pair->second;
        if (!service_ptr)
        {
            // service unregister
			resp_ptr->SetErrorCode(DataErrorCode::kFunctionNotFound);
			return resp_ptr;
        }
        const Pb::ServiceDescriptor *service_dp = service_ptr->GetDescriptor();
        if (!service_dp)
        {
            // service error
			resp_ptr->SetErrorCode(DataErrorCode::kFunctionNotFound);
			return resp_ptr;
        }
        const Pb::MethodDescriptor *method_dp = service_dp->FindMethodByName(method_name);
        if (!method_dp)
        {
            // method error
			resp_ptr->SetErrorCode(DataErrorCode::kFunctionNotFound);
			return resp_ptr;
        }
        Pb::Message *request_msg = service_ptr->GetRequestPrototype(method_dp).New();
        Pb::Message *response_msg = service_ptr->GetResponsePrototype(method_dp).New();
        if (!response_msg || !request_msg)
        {
            // unknown error?
            resp_ptr->SetErrorCode(DataErrorCode::kUnknown);
			return resp_ptr;
        }
        if (!request_msg->ParseFromString(req_ptr->GetParam()))
        {
            // requset parse error
            resp_ptr->SetErrorCode(DataErrorCode::kInvaliadRequest);
			return resp_ptr;
        }
        PbRpcClosure done;
        done.SetCallback([]() {});
        PbRpcController controller;
        service_ptr->CallMethod(method_dp, &controller, request_msg, response_msg, &done);
        std::string resp_str;
        if (!response_msg->SerializeToString(&resp_str))
        {
            // response error
			resp_ptr->SetErrorCode(DataErrorCode::kUnknown);
            return resp_ptr;
        }
        resp_ptr->SetResult(resp_str); // 设置rpc调用结果
        // need to Run done ???
        // if(done)
        // {
        //     done->Run();
        // }
        delete request_msg;
        delete response_msg;
        return resp_ptr;
    }

    void PbRpcDispatcher::UnRegisterService(const std::string &name)
    {
        if (service_map_.find(name) != service_map_.end())
        {
            service_map_.erase(name);
        }
    }

    bool PbRpcDispatcher::GetServiceAndMethod(const std::string &service_full_name, std::string &service_name, std::string &method_name)
    {

        return true;
    }

    // PbServicePtr PbRpcDispatcher::GetService(const std::string &name)
    // {
    //     auto it = service_map_.find(name);
    //     if (it != service_map_.end())
    //     {
    //         return service_map_[name];
    //     }
    //     return nullptr;
    // }
}