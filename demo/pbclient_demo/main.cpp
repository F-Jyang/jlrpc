#include <iostream>
#include <net/pb/pb_client.h>
#include <utils/easy_log.hpp>

int main()
{
    asio::io_context ioct;
    jl::PbClientPtr client_ptr = std::make_shared<jl::PbClient>(ioct);
    client_ptr->Connect("192.168.5.100", 12345);
    client_ptr->SetResponseCallback(
        [](const jl::PbClientPtr &client, const jl::ResponsePtr &response)
        {
            if (response->GetMsgId() == "HEARTBEAT")
            {
                std::string result(response->GetResult());
                LOG_DEBUG << result;
            }
            else
            {
                response->GetResult();
            }
        });
    client_ptr->SetReqeustCallback(
        [](const jl::PbClientPtr &client, std::size_t bytes_transefered)
        {
            LOG_DEBUG << "Client send " << std::to_string(bytes_transefered) << " bytes.";
            client->Read();
        });
    client_ptr->SendHeartBeat();
    ioct.run();
}