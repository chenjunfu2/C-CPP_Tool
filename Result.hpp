#pragma once
#include <initializer_list>
#include <type_traits>
#include <functional>
#include <exception>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <utility>
#include <memory>


template<typename To>
struct ErrorTraits
{};

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

template<typename T>
using Bare_T = std::remove_cvref_t<T>;

template<typename T>
concept Is_Bare = std::same_as<T, Bare_T<T>>;

template<typename To, typename From>
concept Error_Traits_Define =
Is_Bare<To> &&
requires(From && _e)//要么自定义转换
{
	{
		ErrorTraits<To>::from(std::forward<From>(_e))
	} -> std::same_as<To>;
} &&
std::constructible_from<To, To &&>;//同时转换后的结果必须能构造自己，赋值是可选的，所以不做判断

template<typename To, typename From>
concept Error_Convertible =
Is_Bare<To> &&//目标必须是裸类型
(
	std::assignable_from<To &, From> ||//要么可以被指定类型赋值
	std::constructible_from<To, From> ||//要么可以被指定类型构造
	Error_Traits_Define<To, From>//要么用户定义了合法转换
);

template<typename Err, typename Err2>
concept Error_Constructible =
std::constructible_from<Err, Err2> || Error_Traits_Define<Err, Err2>;

template<typename R>
struct ResultTraits;

template<typename V, typename E>
requires
(
	!std::same_as<Bare_T<E>, void> &&
	(Is_Bare<V> && Is_Bare<E>) &&
	(std::is_void_v<V> || (std::is_nothrow_destructible_v<V> && !std::is_array_v<V>)) &&
	(std::is_nothrow_destructible_v<E> && !std::is_array_v<E>)
)
class [[nodiscard]] Result;

template<typename V, typename E>
struct ResultTraits<Result<V, E>>
{
	using ValType = V;
	using ErrType = E;
};

template<typename R>
concept Is_Result =
requires
{
	typename ResultTraits<Bare_T<R>>::ValType;
	typename ResultTraits<Bare_T<R>>::ErrType;
};

template<typename E>
requires(Is_Bare<E> && !std::is_array_v<E> && !std::is_void_v<E>)
struct ErrorWrapper
{
	using ErrType = Bare_T<E>;
	ErrType e;
};

//提示性标签
struct ErrorPlace_T
{};
constexpr static inline ErrorPlace_T ErrorPlace{};

template<typename To, typename From>
requires(Error_Traits_Define<To, From>)
constexpr inline To ConvertTraitsError(From &&_e) noexcept(noexcept(ErrorTraits<To>::from(std::forward<From>(_e))))
{
	return ErrorTraits<To>::from(std::forward<From>(_e));
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

template <class T, class U>
concept EqComparable = requires(T a, U b)
{
	{
		a == b
	} -> std::convertible_to<bool>;
};

template <class T, class U>
concept NeqComparable = requires(T a, U b)
{
	{
		a != b
	} -> std::convertible_to<bool>;
};

template<typename TD, typename TC, typename ...Args>
concept Is_ExceptionSafetyReplace =
std::is_nothrow_destructible_v<Bare_T<TD>> &&
(
	std::is_nothrow_constructible_v<Bare_T<TC>, Args...> ||
	std::is_nothrow_move_constructible_v<Bare_T<TC>> ||
	std::is_nothrow_copy_constructible_v<Bare_T<TD>> ||
	std::is_nothrow_move_constructible_v<Bare_T<TD>>
);

template<typename TSafe, typename TOther>
concept Is_ExceptionSafetySwap =
(
	std::is_nothrow_move_constructible_v<TSafe> ||
	std::is_nothrow_copy_constructible_v<TSafe>
) &&
(
	std::is_nothrow_move_constructible_v<TOther> ||
	std::is_nothrow_copy_constructible_v<TOther> ||
	std::is_copy_constructible_v<TOther>
);

struct DummyResult
{
	bool operator==(DummyResult) const noexcept
	{
		return true;
	}

	bool operator!=(DummyResult) const noexcept
	{
		return false;
	}
};

template<typename V, typename E>
requires
(
	!std::same_as<Bare_T<E>, void> &&
	(Is_Bare<V> &&Is_Bare<E>) &&
	(std::is_void_v<V> || (std::is_nothrow_destructible_v<V> && !std::is_array_v<V>)) &&
	(std::is_nothrow_destructible_v<E> && !std::is_array_v<E>)
)
class [[nodiscard]] Result
{
	template<typename V2, typename E2>
	requires
	(
		!std::same_as<Bare_T<E2>, void> &&
		(Is_Bare<V2> &&Is_Bare<E2>) &&
		(std::is_void_v<V2> || (std::is_nothrow_destructible_v<V2> && !std::is_array_v<V2>)) &&
		(std::is_nothrow_destructible_v<E2> && !std::is_array_v<E2>)
	)
	friend class Result;
public:
	static inline constexpr bool IsVoidValue = std::is_void_v<V>;

	using ValType = Bare_T<V>;
	using StorageType = std::conditional_t<IsVoidValue, DummyResult, ValType>;
	using ErrType = Bare_T<E>;

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
			std::destroy_at(std::addressof(v));
		}
		else
		{
			std::destroy_at(std::addressof(e));
		}
	}

	//异常安全
	template<typename TD, typename TC, typename ...Args>
	requires(Is_ExceptionSafetyReplace<TD, TC, Args...>)
	static void ExceptionSafetyReplace(TD &_destroy, TC &_construct, Args&&... _args) noexcept(std::is_nothrow_constructible_v<TC, Args...>)
	{
		if constexpr (std::is_nothrow_constructible_v<TC, Args...>)//noexcept
		{
			std::destroy_at(std::addressof(_destroy));
			std::construct_at(std::addressof(_construct), std::forward<Args>(_args)...);
		}
		else if constexpr (std::is_nothrow_move_constructible_v<TC>)//excepted
		{
			TC tmp(std::forward<Args>(_args)...);//构造失败直接抛异常，无需回滚
			std::destroy_at(std::addressof(_destroy));
			std::construct_at(std::addressof(_construct), std::move(tmp));
		}
		else if constexpr (std::is_nothrow_copy_constructible_v<TD>)//excepted
		{
			TD tmp(_destroy);
			std::destroy_at(std::addressof(_destroy));
			try
			{
				std::construct_at(std::addressof(_construct), std::forward<Args>(_args)...);
			}
			catch (...)
			{
				std::construct_at(std::addressof(_destroy), tmp);//构造失败回滚
				throw;
			}
		}
		else if constexpr (std::is_nothrow_move_constructible_v<TD>)//excepted
		{
			TD tmp(std::move(_destroy));
			std::destroy_at(std::addressof(_destroy));
			try
			{
				std::construct_at(std::addressof(_construct), std::forward<Args>(_args)...);
			}
			catch (...)
			{
				std::construct_at(std::addressof(_destroy), std::move(tmp));//构造失败回滚
				throw;
			}
		}
		else//fail
		{
			static_assert(false, "WTF?");
		}
	}


	template<typename TSafe, typename TOther>
	requires(Is_ExceptionSafetySwap<TSafe, TOther>)
	static void ExceptionSafetySwap(TSafe &_s_old, TSafe &_s_new, TOther &_o_old, TOther &_o_new)
	noexcept
	(
		(
			std::is_nothrow_move_constructible_v<TSafe> ||
			std::is_nothrow_copy_constructible_v<TSafe>
		) &&
		(
			std::is_nothrow_move_constructible_v<TOther> ||
			std::is_nothrow_copy_constructible_v<TOther>
		)
	)
	{
		/*
		s保证至少有nothrow的move或copy其一，
		总是优先使用s的nothrow方法进行rollback，
		先弄出tmp，接着处理o，如果o有noexcept的move那就move，否则使用copy，
		这样不会因为move失败导致非法状态。一旦成功，
		那么用s同样的noexcept的方法弄到对面对象里，这样完成
		如果失败，那么o不会破坏，于是用noexcept方法回滚s
		
		对于参数，_s_old与_o_old是需要进行swap的原始对象，_s_new与_o_new是目标新对象，
		要求_s_old与_o_new内存地址相同，_s_new与_o_old内存地址相同。
		也就是把_s_old构造到_s_new，_o_old构造到_o_new
		*/

		if constexpr (std::is_nothrow_move_constructible_v<TSafe>)
		{
			TSafe _s_tmp(std::move(_s_old));
			std::destroy_at(std::addressof(_s_old));

			if constexpr (std::is_nothrow_move_constructible_v<TOther>)//优先移动构造
			{
				std::construct_at(std::addressof(_o_new), std::move(_o_old));//noexcept

				std::destroy_at(std::addressof(_o_old));//noexcept
				std::construct_at(std::addressof(_s_new), std::move(_s_tmp));//noexcept
			}
			else if constexpr (std::is_nothrow_copy_constructible_v<TOther>)//回退拷贝构造（无异常）
			{
				std::construct_at(std::addressof(_o_new), _o_old);//noexcept

				std::destroy_at(std::addressof(_o_old));//noexcept
				std::construct_at(std::addressof(_s_new), std::move(_s_tmp));//noexcept
			}
			else if constexpr (std::is_copy_constructible_v<TOther>)//回退拷贝构造（有异常）
			{
				try
				{
					std::construct_at(std::addressof(_o_new), _o_old);//excepted!

					std::destroy_at(std::addressof(_o_old));//noexcept
					std::construct_at(std::addressof(_s_new), std::move(_s_tmp));//noexcept
				}
				catch (...)//回滚
				{
					std::construct_at(std::addressof(_s_old), std::move(_s_tmp));//撤回了一个析构（重新构造）
					throw;
				}
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
		else if constexpr (std::is_nothrow_copy_constructible_v<TSafe>)
		{
			TSafe _s_tmp(_s_old);
			std::destroy_at(std::addressof(_s_old));

			if constexpr (std::is_nothrow_move_constructible_v<TOther>)//优先移动构造
			{
				std::construct_at(std::addressof(_o_new), std::move(_o_old));//noexcept
				std::destroy_at(std::addressof(_o_old));//noexcept

				std::construct_at(std::addressof(_s_new), _s_tmp);//noexcept
			}
			else if constexpr (std::is_nothrow_copy_constructible_v<TOther>)//回退拷贝构造（无异常）
			{
				std::construct_at(std::addressof(_o_new), _o_old);//noexcept
				std::destroy_at(std::addressof(_o_old));//noexcept

				std::construct_at(std::addressof(_s_new), _s_tmp);//noexcept
			}
			else if constexpr (std::is_copy_constructible_v<TOther>)//回退拷贝构造（有异常）
			{
				try
				{
					std::construct_at(std::addressof(_o_new), _o_old);//excepted!
					std::destroy_at(std::addressof(_o_old));//noexcept

					std::construct_at(std::addressof(_s_new), _s_tmp);//noexcept
				}
				catch (...)//回滚
				{
					std::construct_at(std::addressof(_s_old), _s_tmp);//撤回了一个析构（重新构造）
					throw;
				}
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
		else
		{
			static_assert(false, "WTF?");
		}
	}

public:
	template<typename ...Args>
	requires(std::is_nothrow_constructible_v<StorageType, Args...>)
	StorageType &Emplace(Args&&... _args) noexcept//原位构造要求不能出异常
	requires(!IsVoidValue)
	{
		Destroy();
		std::construct_at(std::addressof(v), std::forward<Args>(_args)...);
		bIsOk = true;

		return v;
	}

	template<typename U, typename ...Args>
	requires(std::is_nothrow_constructible_v<StorageType, std::initializer_list<U> &, Args...>)
	StorageType &Emplace(std::initializer_list<U> _il, Args&&... _args) noexcept//原位构造要求不能出异常
	requires(!IsVoidValue)
	{
		Destroy();
		std::construct_at(std::addressof(v), _il, std::forward<Args>(_args)...);
		bIsOk = true;

		return v;
	}

	void Emplace(void) noexcept//构造DummyResult必不抛异常
	requires(IsVoidValue)
	{
		Destroy();
		std::construct_at(std::addressof(v));
		bIsOk = true;
	}

	void Emplace(DummyResult) noexcept//构造DummyResult必不抛异常
	requires(IsVoidValue)
	{
		Emplace();
	}
	
	template<typename ...Args>
	requires(std::is_nothrow_constructible_v<ErrType, Args...>)
	ErrType &Emplace(ErrorPlace_T, Args&&... _args) noexcept//原位构造要求不能出异常
	{
		Destroy();
		std::construct_at(std::addressof(e), std::forward<Args>(_args)...);
		bIsOk = false;

		return e;
	}

	template<typename U, typename ...Args>
	requires(std::is_nothrow_constructible_v<ErrType, std::initializer_list<U> &, Args...>)
	ErrType &Emplace(ErrorPlace_T, std::initializer_list<U> _il, Args&&... _args) noexcept//原位构造要求不能出异常
	{
		Destroy();
		std::construct_at(std::addressof(e), _il, std::forward<Args>(_args)...);
		bIsOk = false;

		return e;
	}

	StorageType &Replace(const StorageType &_val)
	noexcept
	(
		std::is_nothrow_constructible_v<StorageType, const StorageType &> &&
		(
			!std::assignable_from<StorageType &, const StorageType &> ||
			std::is_nothrow_assignable_v<StorageType &, const StorageType &>
		)
	)
	requires
	(
		!IsVoidValue &&
		(
			std::assignable_from<StorageType &, const StorageType &> ||
			(std::constructible_from<StorageType, const StorageType &> && Is_ExceptionSafetyReplace<StorageType &, StorageType &, const StorageType &>)
		) &&
		Is_ExceptionSafetyReplace<ErrType &, StorageType &, const StorageType &>
	)
	{
		using To = StorageType;
		using From = const StorageType &;

		if (bIsOk)
		{
			if constexpr (std::assignable_from<To &, From>)//优先赋值
			{
				v = _val;
			}
			else if constexpr (std::constructible_from<To, From>)//回退重新构造
			{
				ExceptionSafetyReplace(v, v, _val);
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
		else
		{
			ExceptionSafetyReplace(e, v, _val);
			bIsOk = true;
		}

		return v;
	}

	StorageType &Replace(StorageType &&_val)
	noexcept
	(
		std::is_nothrow_constructible_v<StorageType, StorageType &&> &&
		(
			!std::assignable_from<StorageType &, StorageType &&> ||
			std::is_nothrow_assignable_v<StorageType &, StorageType &&>
		)
	)
	requires
	(
		!IsVoidValue &&
		(
			std::assignable_from<StorageType &, StorageType &&> ||
			(std::constructible_from<StorageType, StorageType &&> && Is_ExceptionSafetyReplace<StorageType &, StorageType &, StorageType &&>)
		) &&
		Is_ExceptionSafetyReplace<ErrType &, StorageType &, StorageType &&>
	)
	{
		using To = StorageType;
		using From = StorageType &&;

		if (bIsOk)
		{
			if constexpr (std::assignable_from<To &, From>)//优先赋值
			{
				v = std::move(_val);
			}
			else if constexpr (std::constructible_from<To, From>)//回退重新构造
			{
				ExceptionSafetyReplace(v, v, std::move(_val));
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
		else
		{
			ExceptionSafetyReplace(e, v, std::move(_val));
			bIsOk = true;
		}

		return v;
	}

	void Replace(void) noexcept//构造DummyResult必不抛异常
	requires(IsVoidValue)
	{
		if (bIsOk)
		{
			return;
		}
		else
		{
			std::destroy_at(std::addressof(e));
			std::construct_at(std::addressof(v));//DummyResult{}
			bIsOk = true;
		}
	}

	void Replace(DummyResult) noexcept(noexcept(Replace()))
	requires(IsVoidValue)
	{
		Replace();//空参数代理
	}

	template<typename E2>
	requires
	(
		Error_Convertible<ErrType, E2&&> &&
		(
			std::assignable_from<ErrType &, E2 &&> ||//赋值
			(
				std::constructible_from<ErrType, E2 &&> &&//重构造
				Is_ExceptionSafetyReplace<ErrType &, ErrType &, E2 &&>
			) ||
			Error_Traits_Define<ErrType, E2 &&> &&//可转换
			(
				std::assignable_from<ErrType &, ErrType &&> ||//赋值
				(
					std::constructible_from<ErrType, ErrType &&> &&//重构造
					Is_ExceptionSafetyReplace<ErrType &, ErrType &, ErrType &&>
				)
			)
		) &&
		(
			(
				std::constructible_from<ErrType, E2 &&> &&//赋值
				Is_ExceptionSafetyReplace<StorageType &, ErrType &, E2 &&>
			) ||
			(
				Error_Traits_Define<ErrType, E2 &&> &&//转换
				Is_ExceptionSafetyReplace<StorageType &, ErrType &, ErrType &&>
			)
		)
	)
	ErrType &Replace(ErrorPlace_T, E2 &&_e)
	noexcept
	(
		[]() constexpr -> bool
		{
			bool bNoexcept = true;

			if constexpr (std::assignable_from<ErrType &, E2 &&>)//优先赋值
			{
				bNoexcept = bNoexcept && std::is_nothrow_assignable_v<ErrType &, E2 &&>;
			}
			else if constexpr (std::constructible_from<ErrType, E2 &&>)//回退重新构造
			{
				bNoexcept = bNoexcept && std::is_nothrow_constructible_v<ErrType, E2 &&>;
			}
			else if constexpr (Error_Traits_Define<ErrType, E2 &&>)//需要转换
			{
				if constexpr (std::assignable_from<ErrType &, ErrType &&>)//优先赋值
				{
					bNoexcept = bNoexcept && noexcept(ConvertTraitsError<ErrType>(std::declval<E2 &&>())) && std::is_nothrow_assignable_v<ErrType &, ErrType &&>;
				}
				else if constexpr (std::constructible_from<ErrType, ErrType &&>)//回退重新构造
				{
					bNoexcept = bNoexcept && noexcept(ConvertTraitsError<ErrType>(std::declval<E2 &&>())) && std::is_nothrow_constructible_v<ErrType, ErrType &&>;
				}
			}

			if constexpr (std::constructible_from<ErrType, E2 &&>)
			{
				bNoexcept = bNoexcept && std::is_nothrow_constructible_v<ErrType, E2 &&>;
			}
			else if constexpr (Error_Traits_Define<ErrType, E2 &&>)//需要转换
			{
				bNoexcept = bNoexcept && noexcept(ConvertTraitsError<ErrType>(std::declval<E2 &&>())) && std::is_nothrow_constructible_v<ErrType, ErrType &&>;
			}

			return bNoexcept;
		}()
	)
	{
		if (!bIsOk)
		{
			if constexpr (std::assignable_from<ErrType &, E2 &&>)//优先赋值
			{
				e = std::forward<E2>(_e);
			}
			else if constexpr (std::constructible_from<ErrType, E2 &&>)//回退重新构造
			{
				ExceptionSafetyReplace(e, e, std::forward<E2>(_e));
			}
			else if constexpr (Error_Traits_Define<ErrType, E2 &&>)//需要转换
			{
				if constexpr (std::assignable_from<ErrType &, ErrType &&>)//优先赋值
				{
					e = ConvertTraitsError<ErrType>(std::forward<E2>(_e));
				}
				else if constexpr (std::constructible_from<ErrType, ErrType &&>)//回退重新构造
				{
					ExceptionSafetyReplace(e, e, ConvertTraitsError<ErrType>(std::forward<E2>(_e)));
				}
				else
				{
					static_assert(false, "WTF?");
				}
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
		else
		{
			if constexpr (std::constructible_from<ErrType, E2 &&>)
			{
				ExceptionSafetyReplace(v, e, std::forward<E2>(_e));
			}
			else if constexpr (Error_Traits_Define<ErrType, E2 &&>)//需要转换
			{
				//安全替换，Error_Convertible保证不存在不能使用To构造To的情况。
				ExceptionSafetyReplace(v, e, ConvertTraitsError<ErrType>(std::forward<E2>(_e)));
			}
			else
			{
				static_assert(false, "WTF?");
			}

			bIsOk = false;
		}

		return e;
	}

	template<typename E2>
	requires(Is_Bare<E2> && Error_Convertible<ErrType, const E2 &>)
	ErrType &Replace(const ErrorWrapper<E2> &_err) noexcept(noexcept(Replace(ErrorPlace_T{}, _err.e)))
	{
		return Replace(ErrorPlace_T{}, _err.e);
	}

	template<typename E2>
	requires(Is_Bare<E2> && Error_Convertible<ErrType, E2 &&>)
	ErrType &Replace(ErrorWrapper<E2> &&_err) noexcept(noexcept(Replace(ErrorPlace_T{}, std::move(_err.e))))
	{
		return Replace(ErrorPlace_T{}, std::move(_err.e));
	}

	Result(const StorageType &_val) noexcept(std::is_nothrow_copy_constructible_v<StorageType>)
	requires(!IsVoidValue && std::constructible_from<StorageType, const StorageType &>)
		: v(_val), bIsOk(true)
	{}

	Result(StorageType &&_val) noexcept(std::is_nothrow_move_constructible_v<StorageType>)
	requires(!IsVoidValue && std::constructible_from<StorageType, StorageType &&>)
		: v(std::move(_val)), bIsOk(true)
	{}

	template<typename ...Args>
	requires(std::is_constructible_v<StorageType, Args...>)
	Result(Args&&... _args) noexcept(std::is_nothrow_constructible_v<StorageType, Args...>)
	requires(!IsVoidValue)
		: v(std::forward<Args>(_args)...), bIsOk(true)
	{}

	template<typename U, typename ...Args>
	requires(std::is_constructible_v<StorageType, std::initializer_list<U> &, Args...>)
	Result(std::initializer_list<U> _il, Args&&... _args) noexcept(std::is_nothrow_constructible_v<StorageType, std::initializer_list<U> &, Args...>)
	requires(!IsVoidValue)
		: v(_il, std::forward<Args>(_args)...), bIsOk(true)
	{}

	Result(void) noexcept//保证不抛出
	requires(IsVoidValue)
		: v(), bIsOk(true)
	{}

	Result(DummyResult) noexcept(noexcept(Result()))
	requires(IsVoidValue)
		: Result()//代理转发
	{}

	template<typename E2>
	requires(Error_Constructible<ErrType, const E2 &>)
	Result(const ErrorWrapper<E2> &_err)
	noexcept
	(
		[]() -> bool
		{
			using To = ErrType;
			using From = const E2 &;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				return std::is_nothrow_constructible_v<To, From>;
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				return noexcept(ConvertTraitsError<ErrType>(std::declval<const E2 &>())) && std::is_nothrow_constructible_v<To, ErrType&&>;
			}
		}()
	)
		: bIsOk(false)
	{
		using To = ErrType;
		using From = const E2 &;

		if constexpr (std::constructible_from<To, From>)//可以构造
		{
			std::construct_at(std::addressof(e), _err.e);
		}
		else if constexpr (Error_Traits_Define<To, From>)//需要转换
		{
			std::construct_at(std::addressof(e), ConvertTraitsError<ErrType>(_err.e));
		}
		else
		{
			static_assert(false, "WTF?");
		}
	}

	template<typename E2>
	requires(Error_Constructible<ErrType, E2 &&>)
	Result(ErrorWrapper<E2> &&_err)
	noexcept
	(
		[]() -> bool
		{
			using To = ErrType;
			using From = E2 &&;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				return std::is_nothrow_constructible_v<To, From>;
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				return noexcept(ConvertTraitsError<ErrType>(std::declval<E2 &&>())) && std::is_nothrow_constructible_v<To, ErrType&&>;
			}
		}()
	)
		: bIsOk(false)
	{
		using To = ErrType;
		using From = E2 &&;

		if constexpr (std::constructible_from<To, From>)//可以构造
		{
			std::construct_at(std::addressof(e), std::move(_err.e));
		}
		else if constexpr (Error_Traits_Define<To, From>)//需要转换
		{
			std::construct_at(std::addressof(e), ConvertTraitsError<ErrType>(std::move(_err.e)));
		}
		else
		{
			static_assert(false, "WTF?");
		}
	}

	template<typename ...Args>
	requires(std::is_constructible_v<ErrType, Args...>)
	Result(ErrorPlace_T, Args&&... _args) noexcept(std::is_nothrow_constructible_v<ErrType, Args...>)
		: e(std::forward<Args>(_args)...), bIsOk(false)
	{}

	template<typename U, typename ...Args>
	requires(std::is_constructible_v<ErrType, std::initializer_list<U> &, Args...>)
	Result(ErrorPlace_T, std::initializer_list<U> _il, Args&&... _args) noexcept(std::is_nothrow_constructible_v<ErrType, std::initializer_list<U> &, Args...>)
		: e(_il, std::forward<Args>(_args)...), bIsOk(false)
	{}

	Result(const Result &_other) noexcept(noexcept(std::construct_at(std::addressof(v), _other.v)) && noexcept(std::construct_at(std::addressof(e), _other.e)))
	requires(std::constructible_from<StorageType, const StorageType &> && std::constructible_from<ErrType, const ErrType &>)
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(std::addressof(v), _other.v);
		}
		else
		{
			std::construct_at(std::addressof(e), _other.e);
		}
	}

	Result(Result &&_other) noexcept(noexcept(std::construct_at(std::addressof(v), std::move(_other.v))) && noexcept(std::construct_at(std::addressof(e), std::move(_other.e))))
	requires(std::constructible_from<StorageType, StorageType &&> && std::constructible_from<ErrType, ErrType &&>)
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(std::addressof(v), std::move(_other.v));
		}
		else
		{
			std::construct_at(std::addressof(e), std::move(_other.e));
		}
	}

	template<typename E2>
	requires(!std::same_as<ErrType, Bare_T<E2>> && std::constructible_from<StorageType, const StorageType &> && Error_Constructible<ErrType, const E2&>)
	Result(const Result<ValType, E2> &_other)
	noexcept
	(
		[]() -> bool
		{
			using To = ErrType;
			using From = const E2 &;
			bool bNothrowStorage = std::is_nothrow_constructible_v<StorageType, const StorageType &>;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				return bNothrowStorage && std::is_nothrow_constructible_v<To, From>;
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				return bNothrowStorage && noexcept(ConvertTraitsError<ErrType>(std::declval<const E2 &>())) && std::is_nothrow_constructible_v<To, ErrType&&>;
			}
		}()
	)
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(std::addressof(v), _other.v);
		}
		else
		{
			using To = ErrType;
			using From = const E2 &;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				std::construct_at(std::addressof(e), _other.e);
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				std::construct_at(std::addressof(e), ConvertTraitsError<ErrType>(_other.e));
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
	}

	template<typename E2>
	requires(!std::same_as<ErrType, Bare_T<E2>> && std::constructible_from<StorageType, StorageType &&> && Error_Constructible<ErrType, E2&&>)
	Result(Result<ValType, E2> &&_other)
	noexcept
	(
		[]() -> bool
		{
			using To = ErrType;
			using From = E2 &&;
			bool bNothrowStorage = std::is_nothrow_constructible_v<StorageType, StorageType &&>;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				return bNothrowStorage && std::is_nothrow_constructible_v<To, From>;
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				return bNothrowStorage && noexcept(ConvertTraitsError<ErrType>(std::declval<E2 &&>())) && std::is_nothrow_constructible_v<To, ErrType&&>;
			}
		}()
	)
		: bIsOk(_other.bIsOk)
	{
		if (bIsOk)
		{
			std::construct_at(std::addressof(v), std::move(_other.v));
		}
		else
		{
			using To = ErrType;
			using From = E2 &&;

			if constexpr (std::constructible_from<To, From>)//可以构造
			{
				std::construct_at(std::addressof(e), std::move(_other.e));
			}
			else if constexpr (Error_Traits_Define<To, From>)//需要转换
			{
				std::construct_at(std::addressof(e), ConvertTraitsError<ErrType>(std::move(_other.e)));
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
	}

	~Result(void) noexcept
	{
		Destroy();
#if defined(_MSC_VER) && defined(RESULT_DESTROY_DEBUG)//仅msvc可以这样检测
		memset(&bIsOk, (uint8_t)0xCC, sizeof(bIsOk));//bool无法通过语言层面赋值得到0xCC，必须使用魔法写入，以便触发非法使用插装检查
#endif // RESULT_DESTROY_DEBUG
	}

	Result &operator=(const StorageType &_val) noexcept(noexcept(Replace(_val)))
	requires
	(
		!IsVoidValue &&
		std::assignable_from<StorageType &, const StorageType &> &&
		requires(const StorageType &_val)
		{
			Replace(_val);
		}
	)
	{
		Replace(_val);
		return *this;
	}

	Result &operator=(StorageType &&_val) noexcept(noexcept(Replace(std::move(_val))))
	requires
	(
		!IsVoidValue &&
		std::assignable_from<StorageType &, StorageType &&> &&
		requires(StorageType &&_val)
		{
			Replace(std::move(_val));
		}
	)
	{
		Replace(std::move(_val));
		return *this;
	}

	Result &operator=(StorageType) noexcept(noexcept(Replace()))
	requires(IsVoidValue)
	{
		Replace();
		return *this;
	}

	template<typename E2>
	requires
	(
		Error_Convertible<ErrType, const E2&> &&
		std::assignable_from<ErrType &, const E2 &> &&
		requires(const ErrorWrapper<E2> &_err)
		{
			std::declval<Result &>().Replace(ErrorPlace_T{}, _err.e);
		}
	)
	Result &operator=(const ErrorWrapper<E2> &_err) noexcept(noexcept(Replace(ErrorPlace_T{}, _err.e)))
	{
		Replace(ErrorPlace_T{}, _err.e);
		return *this;
	}

	template<typename E2>
	requires
	(
		Error_Convertible<ErrType, E2&&> &&
		std::assignable_from<ErrType &, E2 &&> &&
		requires(ErrorWrapper<E2> &&_err)
		{
			std::declval<Result &>().Replace(ErrorPlace_T{}, std::move(_err.e));
		}
	)
	Result &operator=(ErrorWrapper<E2> &&_err) noexcept(noexcept(Replace(ErrorPlace_T{}, std::move(_err.e))))
	{
		Replace(ErrorPlace_T{}, std::move(_err.e));
		return *this;
	}

	Result &operator=(const Result &_other) noexcept(noexcept(Replace(_other.v)) && noexcept(Replace(ErrorPlace_T{}, _other.e)))
	requires
	(
		std::assignable_from<StorageType &, const StorageType &> &&
		std::assignable_from<ErrType &, const ErrType &> &&
		requires(const Result &_other)
		{
			std::declval<Result &>().Replace(_other.v);
			std::declval<Result &>().Replace(ErrorPlace_T{}, _other.e);
		}
	)
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
			Replace(ErrorPlace_T{}, _other.e);
		}

		return *this;
	}

	Result &operator=(Result &&_other) noexcept(noexcept(Replace(std::move(_other.v))) && noexcept(Replace(ErrorPlace_T{}, std::move(_other.e))))
	requires
	(
		std::assignable_from<StorageType &, StorageType &&> &&
		std::assignable_from<ErrType &, ErrType &&> &&
		requires(Result &&_other)
		{
			std::declval<Result &>().Replace(std::move(_other.v));
			std::declval<Result &>().Replace(ErrorPlace_T{}, std::move(_other.e));
		}
	)
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
			Replace(ErrorPlace_T{}, std::move(_other.e));
		}

		return *this;
	}

	template<typename E2>
	requires
	(
		!std::same_as<ErrType, Bare_T<E2>> &&
		Error_Convertible<ErrType, const E2&> &&
		std::assignable_from<StorageType &, const StorageType &> &&
		requires(const Result<ValType, E2> &_other)
		{
			std::declval<Result &>().Replace(_other.v);
			std::declval<Result &>().Replace(ErrorPlace_T{}, _other.e);
		}
	)
	Result &operator=(const Result<ValType, E2> &_other) noexcept(noexcept(Replace(_other.v)) && noexcept(Replace(ErrorPlace_T{}, _other.e)))
	{
		if constexpr (std::same_as<Bare_T<ErrType>, Bare_T<E2>>)
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
			Replace(ErrorPlace_T{}, _other.e);
		}

		return *this;
	}

	template<typename E2>
	requires
	(
		!std::same_as<ErrType, Bare_T<E2>> &&
		Error_Convertible<ErrType, E2&&> &&
		std::assignable_from<StorageType &, StorageType &&> &&
		requires(Result<ValType, E2> &&_other)
		{
			std::declval<Result &>().Replace(std::move(_other.v));
			std::declval<Result &>().Replace(ErrorPlace_T{}, std::move(_other.e));
		}
	)
	Result &operator=(Result<ValType, E2> &&_other) noexcept(noexcept(Replace(std::move(_other.v))) && noexcept(Replace(ErrorPlace_T{}, std::move(_other.e))))
	{
		if constexpr (std::same_as<Bare_T<ErrType>, Bare_T<E2>>)
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
			Replace(ErrorPlace_T{}, std::move(_other.e));
		}

		return *this;
	}

	//--功能函数--//
	explicit operator bool(void) const noexcept
	{
		return bIsOk;
	}

	bool operator==(const Result &_other) const noexcept(noexcept(v == _other.v) && noexcept(e == _other.e))
	requires(EqComparable<const StorageType &, const StorageType &> && EqComparable<const ErrType &, const ErrType &>)
	{
		if (bIsOk == _other.bIsOk)
		{
			if (bIsOk)
			{
				return v == _other.v;
			}
			else
			{
				return e == _other.e;
			}
		}
		else
		{
			return false;
		}
	}

	bool operator!=(const Result &_other) const noexcept(noexcept(v != _other.v) && noexcept(e != _other.e))
	requires(NeqComparable<const StorageType &, const StorageType &> && NeqComparable<const ErrType &, const ErrType &>)
	{
		if (bIsOk != _other.bIsOk)
		{
			return true;
		}
		else
		{
			if (bIsOk)
			{
				return v != _other.v;
			}
			else
			{
				return e != _other.e;
			}
		}
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

	const StorageType &&UnwrapValue(void) const && noexcept
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

	const ErrType &&UnwrapError(void) const && noexcept
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

		return std::addressof(v);
	}
	
	const StorageType *TryValue(void) const noexcept
	requires(!IsVoidValue)
	{
		if (!bIsOk)
		{
			return nullptr;
		}

		return std::addressof(v);
	}

	ErrType *TryError(void) noexcept
	{
		if (bIsOk)
		{
			return nullptr;
		}

		return std::addressof(e);
	}

	const ErrType *TryError(void) const noexcept
	{
		if (bIsOk)
		{
			return nullptr;
		}

		return std::addressof(e);
	}

	template<typename F>
	requires
	(
		Is_Bare<ResultInvokeTraits_T<F, ValType>> &&
		requires
		{
			typename Result<ResultInvokeTraits_T<F, ValType>, ErrType>;
		} &&
		(
			(
				std::is_void_v<ResultInvokeTraits_T<F, ValType>> &&
				(
					(
						IsVoidValue &&
						requires(F &&f)
						{
							std::invoke(std::forward<F>(f));
							Result<ResultInvokeTraits_T<F, ValType>, ErrType>{};
						}
					) ||
					(
						!IsVoidValue &&
						requires(F &&f, StorageType &&v)
						{
							std::invoke(std::forward<F>(f), std::move(v));
							Result<ResultInvokeTraits_T<F, ValType>, ErrType>{};
						}
					)
				)
			) ||
			(
				!std::is_void_v<ResultInvokeTraits_T<F, ValType>> &&
				(
					(
						IsVoidValue &&
						requires(F &&f)
						{
							Result<ResultInvokeTraits_T<F, ValType>, ErrType>{ std::invoke(std::forward<F>(f)) };
						}
					) ||
					(
						!IsVoidValue &&
						requires(F &&f, StorageType &&v)
						{
							Result<ResultInvokeTraits_T<F, ValType>, ErrType>{ std::invoke(std::forward<F>(f), std::move(v)) };
						}
					)
				)
			)
		) &&
		requires(ErrType &&e)
		{
			Result<ResultInvokeTraits_T<F, ValType>, ErrType>{ ErrorPlace_T{}, std::move(e) };
		}
	)
	auto MapValue(F &&_func) &&
	noexcept
	(
		[]() -> bool
		{
			using V2 = ResultInvokeTraits_T<F, ValType>;

			bool bRet = noexcept(Result<V2, ErrType>{ ErrorWrapper<ErrType>{ std::declval<ErrType &&>() } });
			if constexpr (std::same_as<Bare_T<V2>, void>)
			{
				if constexpr (IsVoidValue)
				{
					bRet = bRet && noexcept(std::invoke(std::declval<F &&>()));
				}
				else
				{
					bRet = bRet && noexcept(std::invoke(std::declval<F &&>(), std::declval<StorageType &&>()));
				}

				return bRet && noexcept(Result<V2, ErrType>{});
			}
			else
			{
				if constexpr (IsVoidValue)
				{
					return bRet && noexcept(Result<V2, ErrType>{ std::invoke(std::declval<F &&>()) });
				}
				else
				{
					return bRet && noexcept(Result<V2, ErrType>{ std::invoke(std::declval<F &&>(), std::declval<StorageType &&>()) });
				}
			}
		}()
	)
	{
		using V2 = ResultInvokeTraits_T<F, ValType>;

		if (bIsOk)
		{
			if constexpr (std::is_void_v<V2>)
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
			return Result<V2, ErrType>{ ErrorPlace_T{}, std::move(e) };
		}
	}

	template<typename F>
	requires
	(
		Is_Bare<std::invoke_result_t<F, ErrType &&>> &&
		!std::is_void_v<std::invoke_result_t<F, ErrType &&>> &&
		requires
		{
			typename Result<ValType, std::invoke_result_t<F, ErrType &&>>;
		} &&
		requires(F &&f, StorageType &&v, ErrType &&e)
		{
			Result<ValType, std::invoke_result_t<F, ErrType &&>>{ std::move(v) };
			Result<ValType, std::invoke_result_t<F, ErrType &&>>{ ErrorPlace_T{}, std::invoke(std::forward<F>(f), std::move(e)) };
		}
	)
	auto MapError(F &&_func) &&
	noexcept
	(
		noexcept(Result<ValType, std::invoke_result_t<F, ErrType &&>>{ std::move(v) }) &&
		noexcept(Result<ValType, std::invoke_result_t<F, ErrType &&>>{ ErrorPlace_T{}, std::invoke(std::forward<F>(_func), std::move(e)) })
	)
	{
		using E2 = std::invoke_result_t<F, ErrType &&>;

		if (bIsOk)
		{
			return Result<ValType, E2>{ std::move(v) };
		}
		else
		{
			return Result<ValType, E2>{ ErrorPlace_T{}, std::invoke(std::forward<F>(_func), std::move(e)) };
		}
	}

	template<typename F>
	requires
	(
		Is_Bare<ResultInvokeTraits_T<F, ValType>> &&
		Is_Result<ResultInvokeTraits_T<F, ValType>> &&
		Error_Constructible<typename ResultInvokeTraits_T<F, ValType>::ErrType, ErrType &&>
	)
	auto AndThen(F &&_func) &&
	noexcept
	(
		[]() -> bool
		{
			using R = ResultInvokeTraits_T<F, ValType>;

			bool bNoExcept = true;
			if constexpr (IsVoidValue)
			{
				bNoExcept = bNoExcept && noexcept(std::invoke(std::declval<F &&>()));
			}
			else
			{
				bNoExcept = bNoExcept && noexcept(std::invoke(std::declval<F &&>(), std::declval<StorageType &&>()));
			}

			if constexpr (std::constructible_from<typename R::ErrType, ErrType &&>)
			{
				bNoExcept = bNoExcept && noexcept(R{ ErrorPlace_T{}, std::declval<ErrType &&>() });
			}
			else if constexpr (Error_Traits_Define<typename R::ErrType, ErrType &&>)//需要转换
			{
				bNoExcept = bNoExcept && noexcept(R{ ErrorPlace_T{}, ConvertTraitsError<typename R::ErrType>(std::declval<ErrType &&>()) });
			}

			return bNoExcept;
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
			if constexpr (std::constructible_from<typename R::ErrType, ErrType&&>)
			{
				return R{ ErrorPlace_T{}, std::move(e) };
			}
			else if constexpr (Error_Traits_Define<typename R::ErrType, ErrType&&>)//需要转换
			{
				return R{ ErrorPlace_T{}, ConvertTraitsError<typename R::ErrType>(std::move(e)) };
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
	}

	template<typename F>
	requires
	(
		Is_Bare<std::invoke_result_t<F, ErrType &&>> &&
		Is_Result<std::invoke_result_t<F, ErrType &&>> &&
		(
			(IsVoidValue && std::is_constructible_v<Bare_T<std::invoke_result_t<F, ErrType &&>>>) ||
			std::is_constructible_v<Bare_T<std::invoke_result_t<F, ErrType &&>>, StorageType &&>
		)
	)
	auto OrElse(F &&_func) &&
	noexcept
	(
		noexcept(std::invoke_result_t<F, ErrType &&>{ std::move(v) }) &&
		noexcept(std::invoke(std::forward<F>(_func), std::move(e)))
	)
	{
		using R = std::invoke_result_t<F, ErrType &&>;

		if (bIsOk)
		{
			if constexpr (IsVoidValue)
			{
				return R{};
			}
			else
			{
				return R{ std::move(v) };
			}
		}
		else
		{
			return std::invoke(std::forward<F>(_func), std::move(e));
		}
	}

	template<typename T = StorageType>
	requires(std::constructible_from<StorageType, const StorageType &> && std::constructible_from<StorageType, T>)
	StorageType ValueOr(T &&_val) const &
	noexcept
	(
		std::is_nothrow_constructible_v<StorageType, const StorageType &> &&
		std::is_nothrow_constructible_v<StorageType, T>
	)
	requires(!IsVoidValue)
	{
		if (bIsOk)
		{
			return v;
		}

		return std::forward<T>(_val);
	}

	template<typename T = StorageType>
	requires(std::constructible_from<StorageType, StorageType &&> && std::constructible_from<StorageType, T>)
	StorageType ValueOr(T &&_val) &&
	noexcept
	(
		std::is_nothrow_constructible_v<StorageType, StorageType &&> &&
		std::is_nothrow_constructible_v<StorageType, T>
	)
	requires(!IsVoidValue)
	{
		if (bIsOk)
		{
			return std::move(v);
		}

		return std::forward<T>(_val);
	}

	template<typename T = ErrType>
	requires(std::constructible_from<ErrType, const ErrType &> && std::constructible_from<ErrType, T>)
	ErrType ErrorOr(T &&_err) const &
	noexcept
	(
		std::is_nothrow_constructible_v<ErrType, const ErrType &> &&
		std::is_nothrow_constructible_v<ErrType, T>
	)
	{
		if (!bIsOk)
		{
			return e;
		}

		return std::forward<T>(_err);
	}

	template<typename T = ErrType>
	requires(std::constructible_from<ErrType, ErrType &&> && std::constructible_from<ErrType, T>)
	ErrType ErrorOr(T &&_err) &&
	noexcept
	(
		std::is_nothrow_constructible_v<ErrType, ErrType &&> &&
		std::is_nothrow_constructible_v<ErrType, T>
	)
	{
		if (!bIsOk)
		{
			return std::move(e);
		}

		return std::forward<T>(_err);
	}


	void Swap(Result &_other)
	noexcept
	(
		[]() -> bool
		{
			using std::swap;
			bool bNoexcept = true;

			bNoexcept = bNoexcept && noexcept(swap(std::declval<StorageType &>(), std::declval<StorageType &>()));
			bNoexcept = bNoexcept && noexcept(swap(std::declval<ErrType &>(), std::declval<ErrType &>()));

			bNoexcept = bNoexcept && (
				std::is_nothrow_move_constructible_v<StorageType> ||
				std::is_nothrow_copy_constructible_v<StorageType>
			);

			bNoexcept = bNoexcept &&
			(
				std::is_nothrow_move_constructible_v<ErrType> ||
				std::is_nothrow_copy_constructible_v<ErrType>
			);

			return bNoexcept;
		}()
	)
	requires
	(
		(
			requires(Result &_this, Result &_other)
			{
				swap(_this.v, _other.v);
			} ||
			requires(Result &_this, Result &_other)
			{
				std::swap(_this.v, _other.v);
			}
		) &&
		(
			requires(Result &_this, Result &_other)
			{
				swap(_this.e, _other.e);
			} ||
			requires(Result &_this, Result &_other)
			{
				std::swap(_this.e, _other.e);
			}
		) &&
		(
			Is_ExceptionSafetySwap<StorageType, ErrType> ||
			Is_ExceptionSafetySwap<ErrType, StorageType>
		)
	)
	{
		using std::swap;//引入std，首先ADL，否则回退std

		if (bIsOk == _other.bIsOk)//同状态
		{
			if (bIsOk)
			{
				swap(v, _other.v);
			}
			else
			{
				swap(e, _other.e);
			}
		}
		else//异状态
		{
			if constexpr (
				std::is_nothrow_move_constructible_v<StorageType> ||
				std::is_nothrow_copy_constructible_v<StorageType>
			)
			{
				if (bIsOk)//换当前的v
				{
					ExceptionSafetySwap(v, _other.v, _other.e, e);
					bIsOk = false;
					_other.bIsOk = true;
				}
				else//换other的v
				{
					ExceptionSafetySwap(_other.v, v, e, _other.e);
					bIsOk = true;
					_other.bIsOk = false;
				}
			}
			else if constexpr (
				std::is_nothrow_move_constructible_v<ErrType> ||
				std::is_nothrow_copy_constructible_v<ErrType>
			)
			{
				if (bIsOk)//换other的e
				{
					ExceptionSafetySwap(_other.e, e, v, _other.v);
					bIsOk = false;
					_other.bIsOk = true;
				}
				else//换当前的e
				{
					ExceptionSafetySwap(e, _other.e, _other.v, v);
					bIsOk = true;
					_other.bIsOk = false;
				}
			}
			else
			{
				static_assert(false, "WTF?");
			}
		}
	}

	// ADL used
	friend void swap(Result &_l, Result &_r) noexcept(noexcept(_l.Swap(_r)))
	{
		return _l.Swap(_r);
	}
};

template<typename V>
requires(!std::is_array_v<V> && !std::is_void_v<V>)
constexpr inline V ResultOk(V &&_value)
{
	return std::forward<V>(_value);//NRVO对参数不生效，必须主动转发
}

constexpr inline DummyResult ResultOk(void)
{
	return DummyResult{};//use dummy
}

template<typename E>
requires(!std::is_array_v<E> && !std::is_void_v<E>)
constexpr inline ErrorWrapper<std::decay_t<E>> ResultErr(E &&_error)
{
	return ErrorWrapper<std::decay_t<E>>{ std::forward<E>(_error) };//use wrapper
}

#ifndef RESULT_TRY_ASSIGN
#define RESULT_TRY_ASSIGN(name, expr)\
auto name##_result = (expr);\
if (name##_result.IsErr())\
{\
	return ResultErr(std::move(name##_result).UnwrapError());\
}\
auto name = std::move(name##_result).UnwrapValue();
#endif // !RESULT_TRY_ASSIGN

#ifndef RESULT_TRY
#define RESULT_TRY(expr)\
do\
{\
	if (auto _result = (expr); _result.IsErr())\
	{\
		return ResultErr(std::move(_result).UnwrapError());\
	}\
} while(false)
#endif // !RESULT_TRY
