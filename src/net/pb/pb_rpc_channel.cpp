#include "pb_rpc_channel.h"

#include <net/net_data.h>
#include <net/pb/pb_coder.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <system_error>

namespace jl
{

    PbRpcChannel::PbRpcChannel(asio::io_context& ioct, const std::string& ip, uint16_t port):
        ioct_(ioct),
        ip_(ip),
        port_(port)
    {
        client_ = std::make_shared<PbClient>(ioct_, kDefaultBufferSize);
    }

    void PbRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
    {
        controller->Reset();
        do 
        {
            if (!client_->IsConnected() && !client_->Connect(ip_, port_))
            {
                controller->SetFailed("Connect Error");
		        break;
            }
           
            std::string param_str = request->SerializeAsString();
            const std::string& full_name = method->full_name();
            jl::Request* req_ptr = new jl::Request("111",full_name, param_str); // todo: set msg_id
            Response* resp_ptr = nullptr;
            try
            {
                client_->SetTimeout(32);
                bool res = client_->SendRequest(req_ptr);
                if (!res)
                {
				    controller->SetFailed("Send request Error");
                }
                resp_ptr = client_->ReadResponse();
                if(!resp_ptr)
                {
                    throw std::system_error(std::make_error_code(std::errc::timed_out), "read response");
                }
                std::string_view result_view = resp_ptr->GetResult();
                if (!response->ParseFromString(absl::string_view(result_view.data(),result_view.size())))
                {
                    controller->SetFailed("Parse response Error");
                }
            }
            catch (const std::exception& e)
            {
                // LOG
                controller->SetFailed(e.what());
            }

            if (resp_ptr)delete resp_ptr;
            delete req_ptr;
        } while (0);
        if (done)
        {
            done->Run();
        }
    }
}