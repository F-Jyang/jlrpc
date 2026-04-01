#include <iostream>
#include <query_service_impl.h>
#include <net/pb/pb_rpc_channel.h>
#include <net/pb/pb_rpc_closure.h>
#include <net/pb/pb_rpc_controller.h>
#include <google/protobuf/message.h>

int main(int, char **)
{
    google::protobuf::RpcChannel *channel = new jl::PbRpcChannel();
    std::shared_ptr<google::protobuf::Service> query_service_ptr(new QueryServiceImpl());
    QueryReq query_req;
    query_req.set_id(111);
    query_req.set_req_no(222);
    // 获取method descriptor
    auto method_desc = query_service_ptr->GetDescriptor()->FindMethodByName("QueryName");
    // 创建Message
    google::protobuf::Message *req = query_service_ptr->GetRequestPrototype(method_desc).New();
    google::protobuf::Message *resp = query_service_ptr->GetResponsePrototype(method_desc).New();
    // 创建 closure、controller
    jl::PbRpcClosure *closure_ptr = new jl::PbRpcClosure();
    jl::PbRpcController *controller_ptr = new jl::PbRpcController();
    // 设置req、设置closure
    assert(req->ParseFromString(query_req.SerializeAsString()));
    closure_ptr->SetCallback([resp]() {
        QueryRsp query_resp;
        assert(query_resp.ParseFromString(resp->SerializeAsString()));
        std::cout << query_resp.errorcode() << " " << query_resp.msg() << std::endl;
    });
    // 反射调用函数
    query_service_ptr->CallMethod(method_desc, controller_ptr, req, resp, closure_ptr);
}
