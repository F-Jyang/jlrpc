#pragma once
#include <google/protobuf/service.h>

namespace jl
{
    // Abstract interface for a callback.  When calling an RPC, you must provide
    // a Closure to call when the procedure completes.
    // callback的抽象接口，当调用一个RPC时，需要提供一个Closure用于在RPC完成时调用。
    class PbRpcClosure : public google::protobuf::Closure
    {
    public:
        void SetCallback(const std::function<void()> &callabck)
        {
            callback_ = callabck;
        }

        void Run() override
        {
            if (callback_)
            {
                callback_();
            }
        }

    private:
        std::function<void()> callback_;
    };
}
