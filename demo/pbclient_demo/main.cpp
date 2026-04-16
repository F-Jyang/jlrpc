#include <iostream>
#include <net/pb/pb_client.h>
#include <net/pb/pb_rpc_channel.h>
#include <net/pb/pb_rpc_controller.h>
#include <utils/easy_log.hpp>
#include <query_service_impl.h>
#include <thread>

#define CONNECTION_COUNT 1

int main()
{
	std::vector<std::thread> threads(CONNECTION_COUNT);
	for (int i = 0; i < threads.size(); ++i)
	{
        threads[i] = std::move(std::thread(
            [&]()
            {
                asio::io_context ioct;
                //jl::PbClientPtr client_ptr = std::make_shared<jl::PbClient>(ioct);
                //client_ptr->Connect("127.0.0.1", 9999);
      //          client_ptr->SetResponseCallback(
      //              [](const jl::PbClientPtr& client, const jl::Response* response)
      //              {
      //                  if (response->GetMsgId() == "HEARTBEAT")
      //                  {
      //                      std::string result(response->GetResult());
      //                      LOG_DEBUG << result;
      //                  }
      //                  else
      //                  {
      //                      QueryRsp resp;
      //                      std::string result(response->GetResult());
      //                      if (resp.ParseFromString(result))
      //                      {
      //                          LOG_DEBUG << resp.msg() << " " << std::to_string(resp.errorcode());
      //                          if (response->GetErrorCode() != jl::NetErrorCode::kNoError)
      //                          {
      //                              LOG_DEBUG << "Recive error: " << jl::GetPbErrorMsg(response->GetErrorCode());
      //                          }
      //                      }
      //                      else
      //                      {
      //                          LOG_DEBUG << "parser faild!";
      //                      }
      //                  }
      //                  std::this_thread::sleep_for(std::chrono::milliseconds(20));
						////client->SendHeartBeat();
      //                  QueryReq req;
      //                  req.set_id(123);
      //                  req.set_req_no(234);
      //                  std::string req_str = req.SerializeAsString();
      //                  jl::Request* req_ptr = new jl::Request("query_service.QueryName", req_str);
      //                  client->SendRequest(req_ptr);
      //                  delete req_ptr;
      //                  client->ReadResponse();
      //              });
      //          client_ptr->SetReqeustCallback(
      //              [](const jl::PbClientPtr& client, std::size_t bytes_transefered)
      //              {
      //                  LOG_DEBUG << "Client send " << std::to_string(bytes_transefered) << " bytes.";
      //              });

    //            client_ptr->SendHeartBeat();
				//client_ptr->ReadResponse();

                jl::PbRpcChannel channel(ioct,"127.0.0.1", 9999);
                QueryService_Stub stub(&channel);
                jl::PbRpcController controller;
                QueryReq req;
                req.set_id(1);
                req.set_req_no(2);
                QueryRsp resp;
				stub.QueryName(&controller, &req, &resp, nullptr);
				if (controller.Failed())
				{
					LOG_DEBUG << controller.ErrorText();
				}
				LOG_DEBUG << std::to_string(resp.errorcode()) << ":" << resp.msg();
				stub.QueryName(&controller, &req, &resp, nullptr);
				if (controller.Failed())
				{
					LOG_DEBUG << controller.ErrorText();
				}
				LOG_DEBUG << std::to_string(resp.errorcode()) << ":" << resp.msg();
                //ioct.run();
                ioct.stop();
            }
        )
        );
    }
    for (int i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
}