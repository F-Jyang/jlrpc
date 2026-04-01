#pragma once

#include <google/protobuf/service.h>

namespace jl
{

    // An RpcController mediates a single method call.  The primary purpose of
    // the controller is to provide a way to manipulate settings specific to the
    // RPC implementation and to find out about RPC-level errors.
    //
    // The methods provided by the RpcController interface are intended to be a
    // "least common denominator" set of features which we expect all
    // implementations to support.  Specific implementations may provide more
    // advanced features (e.g. deadline propagation).

    /*
    RpcController用于协调单个方法调用
    主要目的:
        1、提供一种方法来操作特定于RPC实现的设置
        2、获取RCP层面的错误信息

    RpcController接口提供的方法被被设计为一组 “最小公分母”功能，——即我们期望所有实现都能支持的基础功能集。
    具体实现的可以提供更高级的特性
    */
    class PbRpcController : public google::protobuf::RpcController
    {
        enum class CancelState
        {
            kNoCancel = 0,
            kCancelBegin,
            kCanceling,
            kCancelFinish,
        };

    public:
        PbRpcController();
        // Client-side methods ---------------------------------------------

        // Resets the RpcController to its initial state so that it may be reused in
        // a new call.  Must not be called while an RPC is in progress.
        // 重置RpcController，以使RpcController可以重复使用。禁止在正在Rpc时调用
        void Reset() override;

        // After a call has finished, returns true if the call failed.  The possible
        // reasons for failure depend on the RPC implementation.  Failed() must not
        // be called before a call has finished.  If Failed() returns true, the
        // contents of the response message are undefined.
        // 用于RPC结束时返回是否调用失败。失败的原因取决于RPC的实现，在RPC结束前禁止调用。
        //  如果调用失败，响应的内容是未定义的。
        bool Failed() const override;

        // If Failed() is true, returns a human-readable description of the error.
        // 如果调用失败，返回可读的错误信息
        std::string ErrorText() const override;

        // Advises the RPC system that the caller desires that the RPC call be
        // canceled.  The RPC system may cancel it immediately, may wait awhile and
        // then cancel it, or may not even cancel the call at all.  If the call is
        // canceled, the "done" callback will still be called and the RpcController
        // will indicate that the call failed at that time.
        // 通知RPC系统，调用者希望取消PRC调用。RPC系统可能很快取消它，可能等待一会然后取消，
        // 也可能根本不取消。如果调用被取消，“done” 回调也会被调用，并且RpcController将会
        // 指示调用失败
        void StartCancel() override;

        // Server-side methods ---------------------------------------------

        // Causes Failed() to return true on the client side.  "reason" will be
        // incorporated into the message returned by ErrorText().  If you find
        // you need to return machine-readable information about failures, you
        // should incorporate it into your response protocol buffer and should
        // NOT call SetFailed().
        // 令client端的Failed()返回true。ErrorText()将返回"reason"。如果你发现失败时需要返回
        // 机器可读的回复，你应该将机器可读的信息包含在你的响应协议缓冲区中，而不应该调用SetFailed()。
        void SetFailed(const std::string &reason) override;

        // If true, indicates that the client canceled the RPC, so the server may
        // as well give up on replying to it.  The server should still call the
        // final "done" callback.
        // 如果true，代表client取消了RPC，所以服务端将放弃回复。服务端接下来应该调用最后的 "done"
        // 回调。
        bool IsCanceled() const override;

        // Asks that the given callback be called when the RPC is canceled.  The
        // callback will always be called exactly once.  If the RPC completes without
        // being canceled, the callback will be called after completion.  If the RPC
        // has already been canceled when NotifyOnCancel() is called, the callback
        // will be called immediately.
        //
        // NotifyOnCancel() must be called no more than once per request.
        // 当RPC被取消时，询问给定的被调用的callback。
        // callback将总是被精确地调用一次。
        // 如果RPC完成而没有被取消，callback将在完成后被调用。
        // NotifyOnCancel()如果RPC被取消，callback将立即被调用
        void NotifyOnCancel(google::protobuf::Closure *callback) override;

        // Common methods
        bool IsFinished();

        void Finished();

    private:
        std::string error_msg_;
        CancelState cancel_state_;
        bool is_failed_;
        bool is_finished_;
    };
}