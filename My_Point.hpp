#pragma once
template<typename T>
struct My_Point
{
public:
	T x;
	T y;
public:
#define OPERATOR_XSELF_OBJ(sym)\
My_Point &operator##sym(const My_Point &_Right) noexcept\
{\
	x sym _Right.x;\
	y sym _Right.y;\
	return *this;\
}

#define OPERATOR_XSELF_NUM(sym)\
My_Point &operator##sym(const T & tNum) noexcept\
{\
	x sym tNum;\
	y sym tNum;\
	return *this;\
}

#define OPERATOR_XCOPY_OBJ(sym)\
My_Point operator##sym(const My_Point &_Right) const noexcept\
{\
	return My_Point\
	{\
		x sym _Right.x,\
		y sym _Right.y,\
	};\
}

#define OPERATOR_XCOPY_NUM(sym)\
My_Point operator##sym(const T & tNum) const noexcept\
{\
	return My_Point\
	{\
		x sym tNum,\
		y sym tNum,\
	};\
}

#define OPERATOR_GENERATOR(sym)\
OPERATOR_XSELF_OBJ(sym##=)\
OPERATOR_XSELF_NUM(sym##=)\
OPERATOR_XCOPY_OBJ(sym)\
OPERATOR_XCOPY_NUM(sym)

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
	bool operator==(const My_Point &_Right)
	{
		return x == _Right.x && y == _Right.y;
	}

	bool operator==(const T &tNum)
	{
		return x == tNum && y == tNum;
	}

	bool operator!=(const My_Point &_Right)
	{
		return x != _Right.x || y != _Right.y;
	}

	bool operator!=(const T &tNum)
	{
		return x != tNum || y != tNum;
	}

};
