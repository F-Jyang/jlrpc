#pragma once

#include <net/pb/pb_client.h>
#include <google/protobuf/service.h>
#include <future>

namespace jl
{
    class PbRpcChannel : public google::protobuf::RpcChannel
    {
    public:
        
        PbRpcChannel(asio::io_context& ioct, const std::string& ip, uint16_t port);
        
        // Call the given method of the remote service.  The signature of this
        // procedure looks the same as Service::CallMethod(), but the requirements
        // are less strict in one important way:  the request and response objects
        // need not be of any specific class as long as their descriptors are
        // method->input_type() and method->output_type().

        // 调用远程服务的函数
        // 该函数类似Service::CallMethod()，但是要求更宽松，request和response不必是特定的类，
        // 只要它们的描述符是method->input_type()和method->output_type()即可。
        void CallMethod(const google::protobuf::MethodDescriptor *method,
                        google::protobuf::RpcController *controller, const google::protobuf::Message *request,
                        google::protobuf::Message *response, google::protobuf::Closure *done) override;

    private:
        asio::io_context& ioct_;
        std::string ip_;
        uint16_t port_;
        PbClientPtr client_;
    };
}