#include <iostream>
#include <net/json_rpc/json_rpc_channel.h>
#include <nlohmann/json.hpp>
#include <utils/easy_log.hpp>

using json = nlohmann::json;

int main()
{
    asio::io_context ioct;

    jl::JsonRpcChannel channel(ioct, "127.0.0.1", 9999);

    // Synchronous call example
    for (int i = 0; i < 3; ++i)
    {
        json params;
        params["id"] = 1;
        params["req_no"] = 100 + i;

        jl::NetErrorCode ec;
        json result;
        bool ok = channel.CallMethod("QueryService.QueryName", params, result, ec);

        if (!ok)
        {
            LOG_DEBUG << "RPC failed: error=" << std::to_string(static_cast<int>(ec));
        }
        else
        {
            LOG_DEBUG << "Response: errorCode=" << std::to_string(result.value("errorCode", -1))
                      << " msg=" << result.value("msg", "");
        }
    }

    LOG_DEBUG << "Client finished";
    ioct.stop();
}
