#pragma once

#define FAST_CGI_ENABLE_LOGGING

#if defined(FAST_CGI_ENABLE_LOGGING)
#	define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#	define FAST_CGI_LOGGER_NAME "fastcgi-default"
#	define FAST_CGI_LOGGER_PATTERN "[%T:%e | %=5t | %-20s at %-3# (%-20!)] [%^%=8l%$]\n\t%v"
#	include <iostream>
#	include <mutex>
#	include <spdlog/spdlog.h>
#	include <spdlog/sinks/stdout_color_sinks.h>

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

namespace fast_cgi {

inline std::shared_ptr<spdlog::logger> get_default_logger()
{
	static std::shared_ptr<spdlog::logger> logger([] {
		auto logger = spdlog::stdout_color_mt(FAST_CGI_LOGGER_NAME);

		logger->set_pattern(FAST_CGI_LOGGER_PATTERN);

		return logger;
	}());

	return logger;
}

} // namespace fast_cgi

#	define FAST_CGI_LOG(level, ...) SPDLOG_LOGGER_##level(fast_cgi::get_default_logger(), __VA_ARGS__)
#else
#	define FAST_CGI_LOG(...) static_cast<void>(0)
#endif
