#include "pb_rpc_channel.h"

namespace jl
{
    void PbRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
    {

        if (done)
        {
            done->Run();
        }
    }
}