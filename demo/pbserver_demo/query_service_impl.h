#pragma once
#include <query.pb.h>

class QueryServiceImpl : public QueryService
{
public:
    void QueryName(::google::protobuf::RpcController *controller,
                   const ::QueryReq *request,
                   ::QueryRsp *response,
                   ::google::protobuf::Closure *done);
};