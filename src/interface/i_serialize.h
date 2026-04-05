/// @brief 序列化接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <string>
#include <optional>
#include <memory>

namespace jl
{
    class Request;
    class Response;

    class ISerialize
    {
    public:
        /// @brief 序列化
        /// @return 序列化后的字符串
		virtual std::string SerializeToString(const Response&) = 0;

		virtual std::string SerializeToString(const Request&) = 0;

        /// @brief 反序列化
        /// @param  序列化后的字符串
        /// @return 成功返回true，失败返回false
		virtual bool ParseFromString(const Response&) = 0;

		virtual bool ParseFromString(const Request&) = 0;
    };

    using SerializePtr = std::shared_ptr<ISerialize>;
}