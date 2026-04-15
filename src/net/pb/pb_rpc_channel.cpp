#include "pb_rpc_channel.h"

#include <net/net_data.h>
#include <net/pb/pb_coder.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <future>

namespace jl
{

    PbRpcChannel::PbRpcChannel(asio::io_context& ioct, const std::string& ip, uint16_t port):
        ioct_(ioct),
        ip_(ip),
        port_(port)
    {
        client_ = std::make_shared<PbClient>(ioct_, kDefaultBufferSize);
		client_->SetResponseCallback(
			[&](const jl::PbClientPtr& client, const jl::Response* response)
			{
                //this->resp_promise_.set_value(std::move(*response));
			});
		client_->SetReqeustCallback(
			[&](const jl::PbClientPtr& client, std::size_t bytes_transefered)
			{
				LOG_DEBUG << "Client send " << std::to_string(bytes_transefered) << " bytes.";
			});
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
            jl::Request* req_ptr = new jl::Request(full_name, param_str);
            req_ptr->SetMsgId("111"); // todo: get msg id
            client_->SendRequest(req_ptr);
            client_->ReadResponse();
            delete req_ptr;
        } while (0);
        if (done)
        {
            done->Run();
        }
    }
}