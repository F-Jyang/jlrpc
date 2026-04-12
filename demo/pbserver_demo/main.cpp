#include <iostream>
#include <net/pb/pb_server.h>
#include <query_service_impl.h>

#ifdef PROTOBUF_REFLECTION_TEST
void test()
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
	closure_ptr->SetCallback([resp]()
							 {
		QueryRsp query_resp;
		assert(query_resp.ParseFromString(resp->SerializeAsString()));
		std::cout << query_resp.errorcode() << " " << query_resp.msg() << std::endl; });
	// 反射调用函数
	query_service_ptr->CallMethod(method_desc, controller_ptr, req, resp, closure_ptr);
}
#endif

int main(int, char **)
{
	// asio::io_context io;
	// asio::signal_set signals(io, SIGINT, SIGTERM);

	// signals.async_wait([&](const std::error_code& ec, int sig) {
	//     std::cout << "Signal received: " << sig << std::endl;
	//     io.stop();
	// });

	// std::cout << "Waiting for SIGINT (Ctrl+C)..." << std::endl;
	// io.run();
	// std::cout << "Exited" << std::endl;
	// return 0;

	asio::io_context ioct;
	jl::PbServer server(ioct, "127.0.0.1", 9999);
	std::shared_ptr<google::protobuf::Service> query_service_ptr(new QueryServiceImpl());
	server.RegisterSerivce("query_service", query_service_ptr);
	server.Start();
}
