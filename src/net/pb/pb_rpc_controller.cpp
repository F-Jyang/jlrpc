#include "pb_rpc_controller.h"

namespace jl
{
    PbRpcController::PbRpcController() : cancel_state_(CancelState::kNoCancel),
                                         is_failed_(false),
                                         error_msg_("")
    {
    }

    void PbRpcController::Reset()
    {
        cancel_state_ = CancelState::kNoCancel;
        is_failed_ = false;
        error_msg_ = "";
    }

    bool PbRpcController::Failed() const
    {
        return is_failed_;
    }

    std::string PbRpcController::ErrorText() const
    {
        return error_msg_;
    }

    void PbRpcController::StartCancel()
    {
        cancel_state_ = CancelState::kCancelBegin;
    }

    void PbRpcController::SetFailed(const std::string &reason)
    {
        is_failed_ = true;
        error_msg_ = reason;
    }

    bool PbRpcController::IsCanceled() const
    {
        return cancel_state_ != CancelState::kNoCancel;
    }

    void PbRpcController::NotifyOnCancel(google::protobuf::Closure *callback)
    {
        if(is_finished_)
        {
            callback->Run();
            return;
        }

        if(cancel_state_!=CancelState::kNoCancel && callback)
        {
            callback->Run();
            return;
        }

    }
    
    bool PbRpcController::IsFinished()
    {
        return is_finished_;
    }

    void PbRpcController::Finished()
    {
        is_finished_ = true;
    }
}