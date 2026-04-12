#include <iostream>
#include <net/pb/pb_client.h>
#include <utils/easy_log.hpp>
#include <query_service_impl.h>
#include <thread>

#define CONNECTION_COUNT 30

int main()
{
	std::vector<std::thread> threads(CONNECTION_COUNT);
	for (int i = 0; i < threads.size(); ++i)
	{
        threads[i] = std::move(std::thread(
            [&]()
            {
                asio::io_context ioct;
                jl::PbClientPtr client_ptr = std::make_shared<jl::PbClient>(ioct);
                client_ptr->Connect("127.0.0.1", 9999);
                client_ptr->SetResponseCallback(
                    [](const jl::PbClientPtr& client, const jl::ResponsePtr& response)
                    {
                        if (response->GetMsgId() == "HEARTBEAT")
                        {
                            std::string result(response->GetResult());
                            LOG_DEBUG << result;
                        }
                        else
                        {
                            QueryRsp resp;
                            std::string result(response->GetResult());
                            if (resp.ParseFromString(result))
                            {
                                LOG_DEBUG << resp.msg() << " " << std::to_string(resp.errorcode());
                                if (response->GetErrorCode() != jl::NetErrorCode::kNoError)
                                {
                                    LOG_DEBUG << "Recive error: " << jl::GetPbErrorMsg(response->GetErrorCode());
                                }
                            }
                            else
                            {
                                LOG_DEBUG << "parser faild!";
                            }
                        }
                    });
                client_ptr->SetReqeustCallback(
                    [](const jl::PbClientPtr& client, std::size_t bytes_transefered)
                    {
                        LOG_DEBUG << "Client send " << std::to_string(bytes_transefered) << " bytes.";
                    });

                client_ptr->SendHeartBeat();
                QueryReq req;
                req.set_id(123);
                req.set_req_no(234);
                std::string req_str = req.SerializeAsString();
                jl::RequestPtr req_ptr = std::make_shared<jl::Request>("query_service.QueryName", req_str);
                client_ptr->SendRequest(req_ptr);
                client_ptr->ReadResponse();
                ioct.run();
            }
        )
        );
    }
    for (int i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
}