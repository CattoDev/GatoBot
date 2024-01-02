#ifdef GB_DEBUG

#include <Geode/loader/Log.hpp>

#define GB_LOG(fmt, ...) geode::log::debug(fmt, __VA_ARGS__);

#else

#define GB_LOG(fmt, ...) 

#endif