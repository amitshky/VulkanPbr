#pragma once

#include <utility>
#include <stdexcept>

#include "core/logger.h"

// non-static member functions cannot be used inside the glfw callbacks, so we bind them to another
// function
#define BIND_FN(fn)                                             \
	[this](auto&&... args) -> decltype(auto) {                  \
		return this->fn(std::forward<decltype(args)>(args)...); \
	}


template<typename... Args>
inline void LogAndThrow(Args&&... formatStr)
{
	Logger::Error(std::forward<Args>(formatStr)...);
	throw std::runtime_error(fmt::format(std::forward<Args>(formatStr)...));
}

template<typename... Args>
inline void ErrCheck(bool condition, Args&&... formatStr)
{
	if (condition)
	{
		Logger::Error(std::forward<Args>(formatStr)...);
		throw std::runtime_error(fmt::format(std::forward<Args>(formatStr)...));
	}
}