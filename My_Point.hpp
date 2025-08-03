#pragma once

#include <concepts>
#include <type_traits>

#define OPERATOR_TEST(op,name)\
template<typename T>\
concept HasOperator##name = requires(T x, T y)\
{\
	{ x op y } -> std::convertible_to<T>;\
};

OPERATOR_TEST(+, Add);
OPERATOR_TEST(-, Sub);
OPERATOR_TEST(*, Mul);
OPERATOR_TEST(/, Div);
OPERATOR_TEST(%, Mod);
OPERATOR_TEST(<<,Shl);
OPERATOR_TEST(>>,Shr);
OPERATOR_TEST(==,Equ);
OPERATOR_TEST(!=,Neq);

template<typename T>
struct My_Point
{
public:
	T x;
	T y;
public:
	//构造、析构、拷贝构造、移动构造、拷贝赋值、移动赋值、
	//全部让其自动生成

#define OPERATOR_XSELF_OBJ(op,name)\
template<typename = typename std::enable_if_t<HasOperator##name<T>>>\
My_Point &operator##op(const My_Point &_Right) noexcept\
{\
	x op _Right.x;\
	y op _Right.y;\
	return *this;\
}

#define OPERATOR_XSELF_NUM(op,name)\
template<typename = typename std::enable_if_t<HasOperator##name<T>>>\
My_Point &operator##op(const T & tNum) noexcept\
{\
	x op tNum;\
	y op tNum;\
	return *this;\
}

#define OPERATOR_XCOPY_OBJ(op,name)\
template<typename = typename std::enable_if_t<HasOperator##name<T>>>\
My_Point operator##op(const My_Point &_Right) const noexcept\
{\
	return My_Point\
	{\
		x op _Right.x,\
		y op _Right.y,\
	};\
}

#define OPERATOR_XCOPY_NUM(op,name)\
template<typename = typename std::enable_if_t<HasOperator##name<T>>>\
My_Point operator##op(const T & tNum) const noexcept\
{\
	return My_Point\
	{\
		x op tNum,\
		y op tNum,\
	};\
}

#define OPERATOR_GENERATOR(op,name)\
OPERATOR_XSELF_OBJ(op##=,name)\
OPERATOR_XSELF_NUM(op##=,name)\
OPERATOR_XCOPY_OBJ(op,name)\
OPERATOR_XCOPY_NUM(op,name)

	//批量生成
	OPERATOR_GENERATOR(+, Add);
	OPERATOR_GENERATOR(-, Sub);
	OPERATOR_GENERATOR(*, Mul);
	OPERATOR_GENERATOR(/, Div);
	OPERATOR_GENERATOR(%, Mod);
	OPERATOR_GENERATOR(<<,Shl);
	OPERATOR_GENERATOR(>>,Shr);

#undef OPERATOR_GENERATOR
#undef OPERATOR_XCOPY_NUM
#undef OPERATOR_XCOPY_OBJ
#undef OPERATOR_XSELF_NUM
#undef OPERATOR_XSELF_OBJ

	//布尔运算
	template<typename = typename std::enable_if_t<HasOperatorEqu<T>>>
	bool operator==(const My_Point &_Right)
	{
		return x == _Right.x && y == _Right.y;
	}

	template<typename = typename std::enable_if_t<HasOperatorEqu<T>>>
	bool operator==(const T &tNum)
	{
		return x == tNum && y == tNum;
	}

	template<typename = typename std::enable_if_t<HasOperatorNeq<T>>>
	bool operator!=(const My_Point &_Right)
	{
		return x != _Right.x || y != _Right.y;
	}

	template<typename = typename std::enable_if_t<HasOperatorNeq<T>>>
	bool operator!=(const T &tNum)
	{
		return x != tNum || y != tNum;
	}
};

#undef OPERATOR_TEST

//修改testType查看是否正确控制运算符生成
#define TEST
#ifdef TEST
void ______Test______(void)
{
	using testType = int;

	My_Point<testType> testPoint{};
	My_Point<testType> testPoint2{};
	My_Point<testType> testPoint3{};

	testPoint += testType{};
	testPoint -= testType{};
	testPoint *= testType{};
	testPoint /= testType{};
	testPoint %= testType{};
	testPoint <<= testType{};
	testPoint >>= testType{};

	testPoint3 = testPoint + testType{};
	testPoint3 = testPoint - testType{};
	testPoint3 = testPoint * testType{};
	testPoint3 = testPoint / testType{};
	testPoint3 = testPoint % testType{};
	testPoint3 = testPoint << testType{};
	testPoint3 = testPoint >> testType{};

	testPoint += testPoint2;
	testPoint -= testPoint2;
	testPoint *= testPoint2;
	testPoint /= testPoint2;
	testPoint %= testPoint2;
	testPoint <<= testPoint2;
	testPoint >>= testPoint2;

	testPoint3 = testPoint + testPoint2;
	testPoint3 = testPoint - testPoint2;
	testPoint3 = testPoint * testPoint2;
	testPoint3 = testPoint / testPoint2;
	testPoint3 = testPoint % testPoint2;
	testPoint3 = testPoint << testPoint2;
	testPoint3 = testPoint >> testPoint2;
}
#endif