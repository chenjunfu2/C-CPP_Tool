#pragma once

#include <concepts>
#include <type_traits>

template<typename T>
struct My_Point
{
public:
	T x;
	T y;
public:
	//构造、析构、拷贝构造、移动构造、拷贝赋值、移动赋值、
	//全部让其自动生成

#define OPERATOR_XSELF_OBJ(op)\
My_Point &operator##op(const My_Point &_Right) noexcept\
	requires requires(T a, T b) {{ a op b } -> std::convertible_to<T>;}\
{\
	x op _Right.x;\
	y op _Right.y;\
	return *this;\
}

#define OPERATOR_XSELF_NUM(op)\
My_Point &operator##op(const T & tNum) noexcept\
	requires requires(T a, T b) {{ a op b } -> std::convertible_to<T>;}\
{\
	x op tNum;\
	y op tNum;\
	return *this;\
}

#define OPERATOR_XCOPY_OBJ(op)\
My_Point operator##op(const My_Point &_Right) const noexcept\
	requires requires(T a, T b) {{ a op b } -> std::convertible_to<T>;}\
{\
	return My_Point\
	{\
		x op _Right.x,\
		y op _Right.y,\
	};\
}

#define OPERATOR_XCOPY_NUM(op)\
My_Point operator##op(const T & tNum) const noexcept\
	requires requires(T a, T b) {{ a op b } -> std::convertible_to<T>;}\
{\
	return My_Point\
	{\
		x op tNum,\
		y op tNum,\
	};\
}

#define OPERATOR_GENERATOR(op)\
OPERATOR_XSELF_OBJ(op##=)\
OPERATOR_XSELF_NUM(op##=)\
OPERATOR_XCOPY_OBJ(op)\
OPERATOR_XCOPY_NUM(op)

	//批量生成
	OPERATOR_GENERATOR(+);
	OPERATOR_GENERATOR(-);
	OPERATOR_GENERATOR(*);
	OPERATOR_GENERATOR(/);
	OPERATOR_GENERATOR(%);
	OPERATOR_GENERATOR(<<);
	OPERATOR_GENERATOR(>>);

#undef OPERATOR_GENERATOR
#undef OPERATOR_XCOPY_NUM
#undef OPERATOR_XCOPY_OBJ
#undef OPERATOR_XSELF_NUM
#undef OPERATOR_XSELF_OBJ

	//布尔运算
	//注意必须std::convertible_to<bool>，并不多余，因为存在自定义重载返回非bool情况
	bool operator==(const My_Point &_Right)
		requires requires(T a, T b) {{ a == b } -> std::convertible_to<bool>;}
	{
		return x == _Right.x && y == _Right.y;
	}

	bool operator==(const T &tNum)
		requires requires(T a, T b) {{ a == b } -> std::convertible_to<bool>;}
	{
		return x == tNum && y == tNum;
	}

	bool operator!=(const My_Point &_Right)
		requires requires(T a, T b) {{ a == b } -> std::convertible_to<bool>;}
	{
		return x != _Right.x || y != _Right.y;
	}

	bool operator!=(const T &tNum)
		requires requires(T a, T b) {{ a == b } -> std::convertible_to<bool>;}
	{
		return x != tNum || y != tNum;
	}

	//不重载大于、小于判断，因为无法知道如何确认大小
};

//修改testType查看是否正确控制运算符生成
#define TEST
#ifdef TEST
void ______Test______(void)
{
	using testType = int;

	My_Point<testType> testPoint{};
	My_Point<testType> testPoint2{};
	My_Point<testType> testPoint3{};

	testPoint == testType{};
	testPoint != testType{};

	testPoint == testPoint2;
	testPoint != testPoint2;

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