#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <fstream>
#include <spdlog/sinks/basic_file_sink.h>

#include "Plugin.h"

#define DLLEXPORT __declspec(dllexport)

#ifndef NDEBUG
#define LOG_DEBUG(msg, ...) logger::debug(msg, ##__VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

namespace logger = SKSE::log;

using namespace std::literals;
namespace util
{
    [[nodiscard]] constexpr int ascii_tolower(int ch) noexcept
    {
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        return ch;
    }

    struct iless
    {
        using is_transparent = int;

        template <std::ranges::contiguous_range S1, std::ranges::contiguous_range S2>
            requires(
        std::is_same_v<std::ranges::range_value_t<S1>, char>&&
            std::is_same_v<std::ranges::range_value_t<S2>, char>)
            constexpr bool operator()(S1&& a_str1, S2&& a_str2) const
        {
            std::size_t count = std::ranges::size(a_str2);
            const std::size_t len1 = std::ranges::size(a_str1);
            const bool shorter = len1 < count;
            if (shorter)
                count = len1;

            if (count) {
                const char* p1 = std::ranges::data(a_str1);
                const char* p2 = std::ranges::data(a_str2);

                do {
                    const int ch1 = ascii_tolower(*p1++);
                    const int ch2 = ascii_tolower(*p2++);
                    if (ch1 != ch2)
                        return ch1 < ch2;
                } while (--count);
            }

            return shorter;
        }
    };

    using SKSE::stl::report_and_fail;

    template <class T>
    using istring_map = std::map<std::string, T, iless>;
}

namespace stl {
    template <typename R, typename... Args>
    inline std::uintptr_t function_ptr(R(*fn)(Args...))
    {
        return reinterpret_cast<std::uintptr_t>(fn);
    }

    template <typename Class, typename R, typename... Args>
    inline std::uintptr_t function_ptr(R(Class::* fn)(Args...))
    {
        return reinterpret_cast<std::uintptr_t>((void*&)fn);
    }
}

// Used as a compile guard in certain templated function (see INISettings.h, if present)
template <class T>
inline constexpr bool always_false = false;

template <class T>
class Singleton
{
public:
    static T* GetSingleton()
    {
        static T singleton;
        return std::addressof(singleton);
    }

    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;

protected:
    Singleton() = default;
    ~Singleton() = default;
};

#define SECTION_SEPARATOR logger::info("=========================================================="sv)