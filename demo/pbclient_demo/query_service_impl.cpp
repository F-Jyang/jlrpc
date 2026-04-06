#include "query_service_impl.h"
#include <iostream>

void QueryServiceImpl::QueryName(::google::protobuf::RpcController *controller, const ::QueryReq *request, ::QueryRsp *response, ::google::protobuf::Closure *done)
{
    std::cout << request->req_no() << " " << request->id() << std::endl;
    response->set_errorcode(0);
    response->set_msg("no error");
    if(done)
    {
        done->Run();
    }
}