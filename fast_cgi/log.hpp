#pragma once

/* #undef FAST_CGI_ENABLE_LOGGING */

#if defined(FAST_CGI_ENABLE_LOGGING)
#    include <spdlog/spdlog.h>
#    define LOG(s) spdlog::s
#else
#    define LOG(s)
#endif
