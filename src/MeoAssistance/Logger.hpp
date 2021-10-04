﻿#pragma once

#include <fstream>
#include <mutex>
#include <iostream>
#include <vector>
#include <type_traits>
#include <filesystem>

#include "AsstAux.h"
#include "Version.h"

namespace asst {
    class Logger {
    public:
        ~Logger() = default;

        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;

        static Logger& get_instance()
        {
            static Logger _unique_instance;
            return _unique_instance;
        }

        template <typename... Args>
        inline void log_trace(Args &&... args)
        {
            constexpr static std::string_view level = "TRC";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void log_info(Args &&... args)
        {
            constexpr static std::string_view level = "INF";
            log(level, std::forward<Args>(args)...);
        }
        template <typename... Args>
        inline void log_error(Args &&... args)
        {
            constexpr static std::string_view level = "ERR";
            log(level, std::forward<Args>(args)...);
        }

        const std::string m_log_filename = asst::GetCurrentDir() + "asst.log";
        const std::string m_log_bak_filename = asst::GetCurrentDir() + "asst.bak.log";
    private:
        Logger()
        {
            check_filesize_and_remove();
            log_init_info();
        }

        void check_filesize_and_remove()
        {
            constexpr uintmax_t MaxLogSize = 4 * 1024 * 1024;
            try {
                if (std::filesystem::exists(m_log_filename)) {
                    uintmax_t log_size = std::filesystem::file_size(m_log_filename);
                    if (log_size >= MaxLogSize) {
                        std::filesystem::rename(m_log_filename, m_log_bak_filename);
                    }
                }
            }
            catch (...)
            {
                ;
            }
        }
        void log_init_info()
        {
            log_trace("-----------------------------");
            log_trace("MeoAssistance Process Start");
            log_trace("Version", asst::Version);
            log_trace("Build DataTime", __DATE__, __TIME__);
            log_trace("Working Path", GetCurrentDir());
            log_trace("Resource Path", GetResourceDir());
            log_trace("-----------------------------");
        }

        template <typename... Args>
        void log(const std::string_view& level, Args &&... args)
        {
            std::unique_lock<std::mutex> trace_lock(m_trace_mutex);

            char buff[128] = { 0 };
            sprintf_s(buff, "[%s][%s][Px%x][Tx%x]",
                asst::GetFormatTimeString().c_str(),
                level.data(), _getpid(), ::GetCurrentThreadId());

            stream_args<true>(std::cout, buff, std::forward<Args>(args)...);
            std::ofstream ofs(m_log_filename, std::ios::out | std::ios::app);
            stream_args(ofs, buff, std::forward<Args>(args)...);
            ofs.close();
        }

        template <bool ToGbk = false, typename T, typename... Args>
        inline void stream_args(std::ostream& os, T&& first, Args && ...rest)
        {
            stream<ToGbk, T>()(os, std::forward<T>(first));
            stream_args<ToGbk>(os, std::forward<Args>(rest)...);
        }
        template <bool>
        inline void stream_args(std::ostream& os)
        {
            os << std::endl;
        }

        template <bool ToGbk, typename T, typename = void>
        struct stream
        {
            inline void operator()(std::ostream& os, T&& first)
            {
                os << first << " ";
            }
        };
        template <typename T>
        struct stream<true, T, typename std::enable_if<std::is_constructible<std::string, T>::value>::type>
        {
            inline void operator()(std::ostream& os, T&& first)
            {
                os << Utf8ToGbk(first) << " ";
            }
        };

        std::mutex m_trace_mutex;
    };

    class LoggerAux {
    public:
        LoggerAux(const std::string& func_name)
            : m_func_name(func_name),
            m_start_time(std::chrono::system_clock::now())
        {
            Logger::get_instance().log_trace(m_func_name, " | enter");
        }
        ~LoggerAux()
        {
            auto duration = std::chrono::system_clock::now() - m_start_time;
            Logger::get_instance().log_trace(m_func_name, " | leave,",
                std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
        }
    private:
        std::string m_func_name;
        std::chrono::time_point<std::chrono::system_clock> m_start_time;
    };
}

#define DebugTrace			Logger::get_instance().log_trace
#define DebugTraceInfo		Logger::get_instance().log_info
#define DebugTraceError		Logger::get_instance().log_error
#define DebugTraceFunction	LoggerAux _func_aux(__FUNCTION__)
#define DebugTraceScope		LoggerAux _func_aux