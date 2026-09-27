#pragma once
#include <exception>
#include <type_traits>
#include <functional>
#include <concepts>
#include <utility>
#include <memory>

template<typename To>
struct ErrorTraits;

//示例：
//template<>
//struct ErrorTraits<NewErrorType>
//{
//	static NewErrorType from(OldErrorType err)
//	{
//		//convert...
//		//return NewErrorType
//	}
//};

template<class T>
concept BareType = std::same_as<T, std::remove_cvref_t<T>>;

template<typename To, typename From>
concept Error_Convertible =
BareType<To> &&
(std::same_as<std::remove_cvref_t<To>, std::decay_t<From>> ||
requires(From &&e)
{
	{
		ErrorTraits<To>::from(std::forward<From>(e))
	} -> std::same_as<To>;
});

template<class R>
struct ResultTraits;

template<typename V, typename E>
requires
(
	!std::same_as<std::remove_cvref_t<E>, void> &&
	(BareType<V> && BareType<E>)
)
class [[nodiscard]] Result;

template<class V, class E>
struct ResultTraits<Result<V, E>>
{
	using ValType = V;
	using ErrType = E;
};

template<class R>
concept Is_Result =
requires
{
	typename ResultTraits<std::remove_cvref_t<R>>::ValType;
	typename ResultTraits<std::remove_cvref_t<R>>::ErrType;
};

template<typename E>
requires(BareType<E>)
struct ErrorWrapper
{
	using ErrType = std::remove_cvref_t<E>;
	ErrType e;
};

template<class To, class From>
requires(Error_Convertible<To, From>)
static To ConvertError(From &&e)
noexcept
(
	[]() -> bool
	{
		if constexpr (std::same_as<std::remove_cvref_t<To>, std::decay_t<From>>)
		{
			return noexcept(To{ std::declval<From>() });
		}
		else
		{
			return noexcept(ErrorTraits<To>::from(std::declval<From>()));
		}

	}()
)
{
	if constexpr (std::same_as<std::remove_cvref_t<To>, std::decay_t<From>>)
	{
		return To{ std::forward<From>(e) };
	}
	else
	{
		return ErrorTraits<To>::from(std::forward<From>(e));
	}
}

template<typename F, typename V>
struct ResultInvokeTraits
{
	using Type = std::invoke_result_t<F, V &&>;
};

template<typename F>
struct ResultInvokeTraits<F, void>
{
	using Type = std::invoke_result_t<F>;
};

template<typename F, typename V>
using ResultInvokeTraits_T = typename ResultInvokeTraits<F, V>::Type;

struct DummyResult
{};

template<typename V, typename E>
requires
(
	!std::same_as<std::remove_cvref_t<E>, void> &&
	(BareType<V> && BareType<E>)
)
class [[nodiscard]] Result
{
	template<typename V, typename E>
	requires
	(
		!std::same_as<std::remove_cvref_t<E>, void> &&
		(BareType<V> &&BareType<E>)
	)
	friend class Result;
public:
	static inline constexpr bool IsVoidValue = std::same_as<std::remove_cvref_t<V>, void>;

	using ValType = std::remove_cvref_t<V>;
	using StorageType = std::conditional_t<IsVoidValue, DummyResult, ValType>;
	using ErrType = std::remove_cvref_t<E>;

protected:
	union
	{
		StorageType v;
		ErrType e;
	};
	bool bIsOk;

	void Destroy(void) noexcept
	{
		if (bIsOk)
		{
			std::destroy_at(&v);
		}
		else
		{
			std::destroy_at(&e);
		}
	}

public:
	void Replace(const StorageType &_val) noexcept
	requires(!IsVoidValue)
	{
		if (bIsOk)
		{
			v = _val;
			return;
		}

		std::destroy_at(&e);
		std::construct_at(&v, _val);
		bIsOk = true;
	}

	void Replace(StorageType &&_val) noexcept
	requires(!IsVoidValue)
	{
		if (bIsOk)
		{
			v = std::move(_val);
			return;
		}

		std::destroy_at(&e);
		std::construct_at(&v, std::move(_val));
		bIsOk = true;
	}

	void Replace(void) noexcept
	requires(IsVoidValue)
	{
		if (bIsOk)
		{
			return;
		}

		std::destroy_at(&e);
		std::construct_at(&v, DummyResult{});
		bIsOk = true;
	}

	void Replace(DummyResult) noexcept
	requires(IsVoidValue)
	{
		Replace();
	}

	template<typename E2>
	requires
	(
		BareType<E2> &&
		(std::same_as<ErrType, std::remove_cvref_t<E2>> ||
		std::constructible_from<ErrType, const E2&> && std::assignable_from<ErrType&, const E2&>)
	)
	void Replace(const ErrorWrapper<E2> &_err) noexcept
	{
		if (!bIsOk)
		{
			e = _err.e;
			return;
		}

		std::destroy_at(&v);
		std::construct_at(&e, _err.e);
		bIsOk = false;
	}

	template<typename E2>
	requires
	(
		BareType<E2> &&
		(std::same_as<ErrType, std::remove_cvref_t<E2>> ||
		std::constructible_from<ErrType, E2&&> && std::assignable_from<ErrType&, E2&&>)
	)
	void Replace(ErrorWrapper<E2> &&_err) noexcept
	{
		if (!bIsOk)
		{
			e = std::move(_err.e);
			return;
		}

		std::destroy_at(&v);
		std::construct_at(&e, std::move(_err.e));
		bIsOk = false;
	}

	Result(const StorageType &_val) noexcept(std::is_nothrow_copy_constructible_v<ValType>)
	requires(!IsVoidValue)
		: v(_val), bIsOk(true)
	{}

	Result(StorageType &&_val) noexcept(std::is_nothrow_move_constructible_v<ValType>)
	requires(!IsVoidValue)
		: v(std::move(_val)), bIsOk(true)
	{}

	Result(void) noexcept
	requires(IsVoidValue)
		: v(DummyResult{}), bIsOk(true)
	{}

	Result(DummyResult) noexcept//ResultOk used
	requires(IsVoidValue)
		: Result()
	{}

	template<typename E2>
	requires(std::constructible_from<ErrType, const E2&>)
	Result(const ErrorWrapper<E2> &_err) noexcept(std::is_nothrow_constructible_v<ErrType, const E2&>)
		: e(_err.e), bIsOk(false)
	{}

	template<typename E2>
	requires(std::constructible_from<ErrType, E2&&>)
	Result(ErrorWrapper<E2> &&_err) noexcept(std::is_nothrow_constructible_v<ErrType, E2 &&>)
		: e(std::move(_err.e)), bIsOk(false)
	{}

	Result(const Result &_other) noexcept(noexcept(std::construct_at(&v, _other.v)) && noexcept(std::construct_at(&e, _other.e)))
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(&v, _other.v);
		}
		else
		{
			std::construct_at(&e, _other.e);
		}
	}

	Result(Result &&_other) noexcept(noexcept(std::construct_at(&v, std::move(_other.v))) && noexcept(std::construct_at(&e, std::move(_other.e))))
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(&v, std::move(_other.v));
		}
		else
		{
			std::construct_at(&e, std::move(_other.e));
		}
	}

	template<typename E2>
	requires(Error_Convertible<ErrType, const E2&>)
	Result(const Result<ValType, E2> &_other) noexcept(noexcept(std::construct_at(&v, _other.v)) && noexcept(std::construct_at(&e, ConvertError<ErrType>(_other.e))))
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(&v, _other.v);
		}
		else
		{
			std::construct_at(&e, ConvertError<ErrType>(_other.e));
		}
	}

	template<typename E2>
	requires(Error_Convertible<ErrType, E2&&>)
	Result(Result<ValType, E2> &&_other) noexcept(noexcept(std::construct_at(&v, std::move(_other.v))) && noexcept(std::construct_at(&e, ConvertError<ErrType>(std::move(_other.e)))))
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(&v, std::move(_other.v));
		}
		else
		{
			std::construct_at(&e, ConvertError<ErrType>(std::move(_other.e)));
		}
	}

	~Result(void) noexcept
	{
		Destroy();
	}

	Result &operator=(const StorageType &_val) noexcept
	requires(!IsVoidValue)
	{
		Replace(_val);
		return *this;
	}

	Result &operator=(StorageType &&_val) noexcept
	requires(!IsVoidValue)
	{
		Replace(std::move(_val));
		return *this;
	}

	Result &operator=(StorageType) noexcept
	requires(IsVoidValue)
	{
		Replace();
		return *this;
	}

	template<typename E2>
	requires(std::constructible_from<ErrType, const E2&> && std::assignable_from<ErrType&, const E2&>)
	Result &operator=(const ErrorWrapper<E2> &_err) noexcept
	{
		Replace(_err);
		return *this;
	}

	template<typename E2>
	requires(std::constructible_from<ErrType, E2&&> && std::assignable_from<ErrType&, E2&&>)
	Result &operator=(ErrorWrapper<E2> &&_err) noexcept
	{
		Replace(std::move(_err));
		return *this;
	}

	Result &operator=(const Result &_other) noexcept
	{
		if (this == std::addressof(_other))
		{
			return *this;
		}

		if (_other.bIsOk)
		{
			Replace(_other.v);
		}
		else
		{
			Replace(ErrorWrapper<ErrType>{ _other.e });
		}

		return *this;
	}

	Result &operator=(Result &&_other) noexcept
	{
		if (this == std::addressof(_other))
		{
			return *this;
		}

		if (_other.bIsOk)
		{
			Replace(std::move(_other.v));
		}
		else
		{
			Replace(ErrorWrapper<ErrType>{ std::move(_other.e) });
		}

		return *this;
	}

	template<typename E2>
	requires(Error_Convertible<ErrType, const E2&>)
	Result &operator=(const Result<ValType, E2> &_other) noexcept
	{
		if constexpr (std::same_as<std::remove_cvref_t<ErrType>, std::decay_t<const E2 &>>)
		{
			if (this == std::addressof(_other))
			{
				return *this;
			}
		}

		if (_other.bIsOk)
		{
			Replace(_other.v);
		}
		else
		{
			Replace(ErrorWrapper<ErrType>{ ConvertError<ErrType>(_other.e) });
		}

		return *this;
	}

	template<typename E2>
	requires(Error_Convertible<ErrType, E2&&>)
	Result &operator=(Result<ValType, E2> &&_other) noexcept
	{
		if constexpr (std::same_as<std::remove_cvref_t<ErrType>, std::decay_t<E2&&>>)
		{
			if (this == std::addressof(_other))
			{
				return *this;
			}
		}

		if (_other.bIsOk)
		{
			Replace(std::move(_other.v));
		}
		else
		{
			Replace(ErrorWrapper<ErrType>{ ConvertError<ErrType>(std::move(_other.e)) });
		}

		return *this;
	}

	//--功能函数--//
	explicit operator bool(void) const noexcept
	{
		return bIsOk;
	}

	bool IsOk(void) const noexcept
	{
		return bIsOk;
	}

	bool IsErr(void) const noexcept
	{
		return !bIsOk;
	}

	StorageType &UnwrapValue(void) & noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			std::terminate();
		}

		return v;
	}

	const StorageType &UnwrapValue(void) const & noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			std::terminate();
		}

		return v;
	}

	StorageType &&UnwrapValue(void) && noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			std::terminate();
		}

		return std::move(v);
	}

	ErrType &UnwrapError(void) & noexcept
	{
		if (bIsOk)
		{
			std::terminate();
		}

		return e;
	}

	const ErrType &UnwrapError(void) const & noexcept
	{
		if (bIsOk)
		{
			std::terminate();
		}

		return e;
	}

	ErrType &&UnwrapError(void) && noexcept
	{
		if (bIsOk)
		{
			std::terminate();
		}

		return std::move(e);
	}

	StorageType *TryValue(void) noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			return nullptr;
		}

		return &v;
	}
	
	const StorageType *TryValue(void) const noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			return nullptr;
		}

		return &v;
	}

	ErrType *TryError(void) noexcept
	{
		if (bIsOk)
		{
			return nullptr;
		}

		return &e;
	}

	const ErrType *TryError(void) const noexcept
	{
		if (bIsOk)
		{
			return nullptr;
		}

		return &e;
	}

	template<typename F>
	requires(BareType<ResultInvokeTraits_T<F, ValType>>)
	auto MapValue(F &&_func) &&
	noexcept
	(
		[]() -> bool
		{
			using V2 = ResultInvokeTraits_T<F, ValType>;

			bool bRet = noexcept(Result<V2, ErrType>{ ErrorWrapper<ErrType>{ std::declval<ErrType &&>() } });
			if constexpr (std::same_as<std::remove_cvref_t<V2>, void>)
			{
				if constexpr (IsVoidValue)
				{
					bRet = bRet && noexcept(std::invoke(std::declval<F>()));
				}
				else
				{
					bRet = bRet && noexcept(std::invoke(std::declval<F>(), std::declval<ValType &&>()));
				}

				return bRet && noexcept(Result<V2, ErrType>{});
			}
			else
			{
				if constexpr (IsVoidValue)
				{
					return bRet && noexcept(Result<V2, ErrType>{ std::invoke(std::declval<F>()) });
				}
				else
				{
					return bRet && noexcept(Result<V2, ErrType>{ std::invoke(std::declval<F>(), std::declval<ValType &&>()) });
				}
			}
		}()
	)
	{
		using V2 = ResultInvokeTraits_T<F, ValType>;

		if (bIsOk)
		{
			if constexpr (std::same_as<std::remove_cvref_t<V2>, void>)
			{
				if constexpr (IsVoidValue)
				{
					std::invoke(std::forward<F>(_func));
				}
				else
				{
					std::invoke(std::forward<F>(_func), std::move(v));
				}

				return Result<V2, ErrType>{};
			}
			else
			{
				if constexpr (IsVoidValue)
				{
					return Result<V2, ErrType>{ std::invoke(std::forward<F>(_func)) };
				}
				else
				{
					return Result<V2, ErrType>{ std::invoke(std::forward<F>(_func), std::move(v)) };
				}
			}
		}
		else
		{
			return Result<V2, ErrType>{ ErrorWrapper<ErrType>{ std::move(e) } };
		}
	}

	template<typename F>
	requires(BareType<std::invoke_result_t<F, ErrType &&>>)
	auto MapError(F &&_func) &&
	noexcept
	(
		noexcept(Result<ValType, std::invoke_result_t<F, ErrType &&>>{ std::move(v) }) &&
		noexcept(Result<ValType, std::invoke_result_t<F, ErrType &&>>{ ErrorWrapper<std::invoke_result_t<F, ErrType &&>>{ std::invoke(std::declval<F>(), std::move(e)) } })
	)
	{
		using E2 = std::invoke_result_t<F, ErrType &&>;

		if (bIsOk)
		{
			return Result<ValType, E2>{ std::move(v) };
		}
		else
		{
			return Result<ValType, E2>{ ErrorWrapper<E2>{ std::invoke(std::forward<F>(_func), std::move(e)) } };
		}
	}

	template<typename F>
	requires(BareType<ResultInvokeTraits_T<F, ValType>> && Is_Result<ResultInvokeTraits_T<F, ValType>>)
	auto AndThen(F &&_func) &&
	noexcept
	(
		[]() -> bool
		{
			using R = ResultInvokeTraits_T<F, ValType>;

			if constexpr (IsVoidValue)
			{
				return noexcept(std::invoke(std::declval<F>())) &&
					noexcept(R{ ErrorWrapper<typename R::ErrType>{ ConvertError<typename R::ErrType>(std::declval<ErrType &&>()) } });
			}
			else
			{
				return noexcept(std::invoke(std::declval<F>(), std::declval<ValType &&>())) &&
					noexcept(R{ ErrorWrapper<typename R::ErrType>{ ConvertError<typename R::ErrType>(std::declval<ErrType &&>()) } });
			}
		}()
	)
	{
		using R = ResultInvokeTraits_T<F, ValType>;

		if (bIsOk)
		{
			if constexpr (IsVoidValue)
			{
				return std::invoke(std::forward<F>(_func));
			}
			else
			{
				return std::invoke(std::forward<F>(_func), std::move(v));
			}
		}
		else
		{
			return R{ ErrorWrapper<typename R::ErrType>{ ConvertError<typename R::ErrType>(std::move(e)) } };
		}
	}

	template<typename F>
	requires
	(
		BareType<std::invoke_result_t<F, ErrType &&>> &&
		Is_Result<std::invoke_result_t<F, ErrType &&>> &&
		std::same_as<ValType, std::remove_cvref_t<typename std::invoke_result_t<F, ErrType &&>::ValType>>
	)
	auto OrElse(F &&_func) &&
	noexcept
	(
		noexcept(std::invoke_result_t<F, ErrType &&>{ std::move(v) }) &&
		noexcept(std::invoke(std::declval<F>(), std::move(e)))
	)
	{
		using R = std::invoke_result_t<F, ErrType &&>;

		if (bIsOk)
		{
			return R{ std::move(v) };
		}
		else
		{
			return std::invoke(std::forward<F>(_func), std::move(e));
		}
	}
};

template<class V>
std::decay_t<V> ResultOk(V &&_value)
{
	return _value;//use implicit move
}

DummyResult ResultOk(void)
{
	return DummyResult{};//use dummy
}

template<class E>
ErrorWrapper<std::decay_t<E>> ResultErr(E &&_error)
{
	return ErrorWrapper<std::decay_t<E>>{ std::forward<E>(_error) };//use wrapper
}

#ifndef RESULT_TRY_ASSIGN
#define RESULT_TRY_ASSIGN(name, expr)\
auto name##_result = (expr);\
if (name##_result.IsErr())\
{\
	return std::move(name##_result);\
}\
auto name = std::move(name##_result).UnwrapValue();
#endif // !RESULT_TRY_ASSIGN

#ifndef RESULT_TRY
#define RESULT_TRY(expr)\
do\
{\
	if (auto _result = (expr); _result.IsErr())\
	{\
		return std::move(_result);\
	}\
} while(false)
#endif // !RESULT_TRY
