#include <iostream>
#include <net/json_rpc/json_server.h>
#include <nlohmann/json.hpp>
#include <utils/easy_log.hpp>

using json = nlohmann::json;

int main(int, char**)
{
    asio::io_context ioct;
    jl::JsonServer server(ioct, "127.0.0.1", 9999);

    // Register RPC method using lambda
    server.RegisterMethod("QueryService.QueryName",
        [](const json& params, jl::NetErrorCode& error) -> json
        {
            if (!params.contains("id") || !params.contains("req_no"))
            {
                error = jl::NetErrorCode::kInvaliadRequest;
                return json{};
            }

            int id = params["id"];
            int req_no = params["req_no"];

            std::cout << "QueryName called: req_no=" << req_no << " id=" << id << std::endl;

            json result;
            result["errorCode"] = 0;
            result["msg"] = "no error - JSON-RPC Server";
            return result;
        });

    server.Start();
    ioct.run();
}
