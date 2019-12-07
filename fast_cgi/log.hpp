#pragma once

/* #undef FAST_CGI_ENABLE_LOGGING */

#if defined(FAST_CGI_ENABLE_LOGGING)
#    define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#    include <iostream>
#    include <mutex>
#    include <spdlog/spdlog.h>

struct buff
{
    const void* buf;
    std::size_t s;
};

namespace fmt {
template<>
struct formatter<buff>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) const -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const buff& p, FormatContext& ctx) -> decltype(ctx.out())
    {
        for (auto i = (const unsigned char*) p.buf, c = i + p.s; i < c; ++i) {
            if (std::isprint(*i)) {
                format_to(ctx.out(), "{}", (char) *i);
            } else {
                format_to(ctx.out(), "\\x{:02x}", *i);
            }
        }

        return ctx.out();
    }
};
} // namespace fmt

template<typename... Ts>
inline void log(const char* func, const char* level, const char* msg, Ts&&... ts)
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    fmt::print("[{:<15} | {:>8}] {}\n", func, level, fmt::format(msg, std::forward<Ts>(ts)...));
}

#    define LOG(level, ...) SPDLOG_##level(__VA_ARGS__)
#else
#    define LOG(...) ((void)0)
#endif
