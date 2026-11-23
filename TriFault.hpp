#pragma once

#include "MyAssert.hpp"
#include "Result.hpp"

#include <source_location>
#include <type_traits>
#include <exception>
#include <concepts>
#include <stdint.h>
#include <stddef.h>
#include <utility>
#include <string>
#include <memory>
#include <format>
#include <list>

namespace tf
{
	template<typename RecoverableContext>
	requires(std::same_as<std::remove_cvref_t<RecoverableContext>, RecoverableContext> && std::is_nothrow_destructible_v<RecoverableContext>)
	struct Error//只存储Recoverable和回溯Fatal，Logic走Assert宏
	{
	public:
		struct PathSegment
		{
			std::string message;
			std::string detail;
			std::source_location location;
		};

		using RecoverableContext_T = RecoverableContext;
		using PathList_T = std::list<PathSegment>;

		union
		{
			RecoverableContext_T ctx;
			PathList_T path;
		};
		const bool isRecoverable;

	public:
		Error(RecoverableContext_T &&ctx_) noexcept(std::is_nothrow_move_constructible_v<RecoverableContext_T>)
			: ctx(std::move(ctx_)), isRecoverable(true)
		{}

		explicit Error(const RecoverableContext_T &ctx_) noexcept(std::is_nothrow_copy_constructible_v<RecoverableContext_T>)
			: ctx(ctx_), isRecoverable(true)
		{}

		Error(PathList_T &&path_) noexcept(std::is_nothrow_move_constructible_v<PathList_T>)
			: path(std::move(path_)), isRecoverable(false)
		{}

		explicit Error(const PathList_T &path_) noexcept(std::is_nothrow_copy_constructible_v<PathList_T>)
			: path(path_), isRecoverable(false)
		{}

		explicit Error(void) noexcept(std::is_nothrow_default_constructible_v<PathList_T>)
			: path({}), isRecoverable(false)
		{}

		Error(Error &&move)
			noexcept
			(
				std::is_nothrow_move_constructible_v<RecoverableContext_T> &&
				std::is_nothrow_move_constructible_v<PathList_T>
			)
			: isRecoverable(move.isRecoverable)
		{
			if (isRecoverable)
			{
				std::construct_at(&ctx, std::move(move.ctx));
			}
			else
			{
				std::construct_at(&path, std::move(move.path));
			}
		}

		explicit Error(const Error &copy)
			noexcept
			(
				std::is_nothrow_copy_constructible_v<RecoverableContext_T> &&
				std::is_nothrow_copy_constructible_v<PathList_T>
			)
			: isRecoverable(copy.isRecoverable)
		{
			if (isRecoverable)
			{
				std::construct_at(&ctx, copy.ctx);
			}
			else
			{
				std::construct_at(&path, copy.path);
			}
		}
		
		~Error(void) noexcept
		{
			if (isRecoverable)
			{
				std::destroy_at(&ctx);
			}
			else
			{
				std::destroy_at(&path);
			}
		}

		Error &operator=(const Error &) = delete;
		Error &operator=(Error &&) = delete;

	public:
		template<typename PathSegT>
		requires(std::same_as<std::remove_cvref_t<PathSegT>, PathSegment>)
		Error &WrapFatalTrace(PathSegT &&pathSeg) & noexcept
		{
			MyAssert(!isRecoverable, "Cannot wrap trace on a Recoverable error");
			try
			{
				path.push_back(std::forward<PathSegT>(pathSeg));
			}
			catch (const std::exception &e)
			{
				constexpr bool catch_exception = false;
				MyAssert(catch_exception, "%s", e.what());
			}

			return *this;
		}

		template<typename PathSegT>
		requires(std::same_as<std::remove_cvref_t<PathSegT>, PathSegment>)
		Error &&WrapFatalTrace(PathSegT &&pathSeg) && noexcept
		{
			MyAssert(!isRecoverable, "Cannot wrap trace on a Recoverable error");
			try
			{
				path.push_back(std::forward<PathSegT>(pathSeg));
			}
			catch (const std::exception &e)
			{
				constexpr bool catch_exception = false;
				MyAssert(catch_exception, "%s", e.what());
			}

			return std::move(*this);
		}

		template<typename STR0, typename STR1>
		requires(std::is_constructible_v<std::string, STR0> && std::is_constructible_v<std::string, STR1>)
		Error &WrapFatalTrace(STR0 &&message, STR1 &&detail, std::source_location location = std::source_location::current()) & noexcept
		{
			MyAssert(!isRecoverable, "Cannot wrap trace on a Recoverable error");
			try
			{
				path.emplace_back(std::forward<STR0>(message), std::forward<STR1>(detail), std::move(location));
			}
			catch (const std::exception &e)
			{
				constexpr bool catch_exception = false;
				MyAssert(catch_exception, "%s", e.what());
			}

			return *this;
		}

		template<typename STR0, typename STR1>
		requires(std::is_constructible_v<std::string, STR0> && std::is_constructible_v<std::string, STR1>)
		Error &&WrapFatalTrace(STR0 &&message, STR1 &&detail, std::source_location location = std::source_location::current()) && noexcept
		{
			MyAssert(!isRecoverable, "Cannot wrap trace on a Recoverable error");
			try
			{
				path.emplace_back(std::forward<STR0>(message), std::forward<STR1>(detail), std::move(location));
			}
			catch (const std::exception &e)
			{
				constexpr bool catch_exception = false;
				MyAssert(catch_exception, "%s", e.what());
			}

			return std::move(*this);
		}

		[[nodiscard]] PathList_T &GetFatalTrace(void) noexcept
		{
			MyAssert(!isRecoverable, "Cannot get Fatal trace for a Recoverable error");
			return path;
		}

		[[nodiscard]] const PathList_T &GetFatalTrace(void) const noexcept
		{
			MyAssert(!isRecoverable, "Cannot get Fatal trace for a Recoverable error");
			return path;
		}

		[[nodiscard]] RecoverableContext_T &GetRecoverableContext(void) noexcept
		{
			MyAssert(isRecoverable, "Cannot get context from a Fatal error");
			return ctx;
		}

		[[nodiscard]] const RecoverableContext_T &GetRecoverableContext(void) const noexcept
		{
			MyAssert(isRecoverable, "Cannot get context from a Fatal error");
			return ctx;
		}

		[[nodiscard]] bool IsRecoverable(void) const noexcept
		{
			return isRecoverable;
		}

		[[nodiscard]] std::string ToString(void) const
		{
			MyAssert(!isRecoverable, "Cannot format trace to string for a Recoverable error");
			std::string out = "Fatal Error!\n";

			auto it = path.crbegin(), end = path.crend();
			if (it == end)
			{
				out += "└─ <Empty Stack Trace>\n";
				return out;
			}

			while(true)//至少1个
			{
				const auto &seg = *it++;
				if(it != end)
				{
					out += std::format
					(
						"├─ [{}:{}] {}\n│   message: {}\n│   detail: {}\n",
						seg.location.file_name(), seg.location.line(), seg.location.function_name(),
						seg.message,
						seg.detail
					);
				}
				else
				{
					out += std::format
					(
						"└─ [{}:{}] {}\n    message: {}\n    detail: {}\n",
						seg.location.file_name(), seg.location.line(), seg.location.function_name(),
						seg.message,
						seg.detail
					);
					break;
				}
			}

			return out;
		}

		using HasResultFailInfo = void;//标签
		static void ResultFailInfo(const char *err) noexcept//Result错误信息反馈接口
		{
			constexpr bool ResultFail = false;
			MyAssert(ResultFail, err);
		}
	};

	template<typename Value, typename RecoverableContext>
	using Result = CppResult::Result<Value, Error<RecoverableContext>>;

	template<typename RecoverableContext, typename CTX>
	requires(std::same_as<RecoverableContext, std::remove_cvref_t<CTX>>)
	auto MakeRecoverableError(CTX &&ctx)
	{
		return CppResult::ResultErr(Error<RecoverableContext>{ std::forward<CTX>(ctx) });
	}

	template<typename RecoverableContext>
	auto MakeFatalError(void)
	{
		return CppResult::ResultErr(Error<RecoverableContext>{});
	}

	template<typename RecoverableContext, typename STR0, typename STR1>
	auto MakeFatalError(STR0 &&message, STR1 &&detail, std::source_location location = std::source_location::current())
	{
		return CppResult::ResultErr(Error<RecoverableContext>{}.WrapFatalTrace(std::forward<STR0>(message), std::forward<STR1>(detail), std::move(location)));
	}

	template<typename RecoverableContext>
	auto MakeError(Error<RecoverableContext> &&err)
	{
		return CppResult::ResultErr(std::move(err));
	}

	template<typename RecoverableContext>
	auto MakeError(Error<RecoverableContext> &err)
	{
		return CppResult::ResultErr(err);
	}

#ifndef MakeLogicError
#define MakeLogicError(format_str, ...)\
do\
{\
	constexpr bool LogicError = false;\
	MyAssert(LogicError, format_str __VA_OPT__(, ) __VA_ARGS__);\
} while (false)
#endif // !MakeLogicError
}