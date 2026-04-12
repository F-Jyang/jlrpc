#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#define LOG_DEBUG jl::LogStream(jl::LogLevel::kDebug, __FUNCTION__)

namespace jl
{
    enum class LogLevel
    {
        kDebug = 0,
        kInfo,
        kWarn,
        kError
    };

    class Logger
    {
    public:
        Logger() : 
            buffer_(4096)
        {
        }

        static Logger &GetInstance()
        {
            static Logger logger;
            return logger;
        }

        void log(LogLevel level, const std::string &func_name, const std::string &msg)
        {
            std::string level_str;
            if (level == LogLevel::kDebug)
            {
                level_str = "DEBUG";
            }
            else if (level == LogLevel::kInfo)
            {
                level_str = "INFO";
            }
            else if (level == LogLevel::kWarn)
            {
                level_str = "WARN";
            }
            else if (level == LogLevel::kError)
            {
                level_str = "ERROR";
            }
            std::stringstream oss;
            oss << "[" << level_str << "] " << func_name << ": " << msg << "\n";
            printf("%s", oss.str().c_str());
        }

    private:
        std::vector<char> buffer_;
    };

    class LogStream
    {
    public:
        LogStream(LogLevel level, const std::string &func_name) : level_(level),
                                                                  func_name_(func_name)
        {
        }
        ~LogStream()
        {
            Logger::GetInstance().log(level_, func_name_, msg_);
        }

        LogStream &operator<<(const std::string msg)
        {
            msg_ += msg;
            return *this;
        }

    private:
        LogLevel level_;
        std::string func_name_;
        std::string msg_;
    };

}