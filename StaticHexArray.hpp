#pragma once

#include <bit>

#include <array>
#include <string>
#include <vector>

#include <stdint.h>
#include <stddef.h>

class StaticHexArray
{
	StaticHexArray(void) = delete;
	~StaticHexArray(void) = delete;

public:
	template<typename T, size_t N>
	requires(sizeof(T) == 1)
		struct StringLiteral : public std::array<T, N>
	{
	public:
		using Super = std::array<T, N>;

	public:
		constexpr StringLiteral(const T(&_tStr)[N]) noexcept :Super(std::to_array(_tStr))
		{}
		constexpr ~StringLiteral(void) noexcept = default;
	};

private:
	class Counter
	{
	private:
		size_t szCount = 0;

	public:
		constexpr Counter(void) noexcept = default;
		constexpr ~Counter(void) noexcept = default;

		constexpr void Push(uint8_t u8Byte) noexcept
		{
			++szCount;
		}

		constexpr const auto &Raw(void) const noexcept
		{
			return szCount;
		}
	};

	template<typename T, size_t N>
	requires(sizeof(T) == 1)//大小必须为1
	class Array
	{
	private:
		std::array<T, N> arrData{ 0 };
		size_t szIndex = 0;

	public:
		constexpr Array(void) noexcept = default;
		constexpr ~Array(void) noexcept = default;

		constexpr void Push(uint8_t u8Byte) noexcept
		{
			arrData[szIndex] = std::bit_cast<T>(u8Byte);
			++szIndex;
		}

		constexpr const auto &Raw(void) const noexcept
		{
			return arrData;
		}
	};

	template<typename T = std::basic_string<uint8_t>>
	requires(sizeof(typename T::value_type) == 1)//大小必须为1
	class DynamicArray
	{
	private:
		T tData{};

	public:
		DynamicArray(size_t szReserve = 0)
		{
			if (szReserve != 0)
			{
				tData.reserve(szReserve);
			}
		}

		~DynamicArray(void) = default;

		void Push(uint8_t u8Byte)
		{
			tData.push_back(std::bit_cast<typename T::value_type>(u8Byte));
		}

		const auto &Raw(void) const noexcept
		{
			return tData;
		}
	};

private:
	static constexpr uint8_t CharToHex(char c) noexcept
	{
		switch (c)
		{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return (uint8_t)(c - '0') + (uint8_t)0x0;

		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			return (uint8_t)(c - 'A') + (uint8_t)0xA;

		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			return (uint8_t)(c - 'a') + (uint8_t)0xA;

		default:
			return (uint8_t)0x0;
		}
	}

	static constexpr uint8_t CharsToHex(char h, char l) noexcept
	{
		return (CharToHex(h) & 0x0F) << 4 | (CharToHex(l) & 0x0F) << 0;
	}

	static constexpr bool IsSpace(char c) noexcept
	{
		switch (c)
		{
		case ' ': //space
		case '\t'://horizontal tab
		case '\n'://newline
		case '\v'://vertical tab
		case '\f'://form feed
		case '\r'://carriage return
		case '\0'://end str
			return true;
		default:
			return false;
		}
	}

	static constexpr bool IsDelimiter(char c) noexcept
	{
		switch (c)
		{
		case ',': case '.': case ';': case ':':
		case '`': case '|': case '\\': case '/':
		case '-': case '_': case '\'': case '\"':
			return true;
		default:
			return false;
		}
	}

	template<typename RetT, typename T>
	static constexpr auto Parse(const T *tpData, size_t szDataSize, RetT tRet = {}) noexcept
	{
		size_t i = 0;

		//获取一个非空白字符
		//但是为什么这里不用lambda而要使用宏定义呢？因为三大家编译器中，
		//只有一家不支持带捕获的编译期lambda，是谁呢，好难猜呀~
#define GetNoSpaceChar(c,...)\
/*跳过空白*/\
{\
	/*注意这里的IsSpace对于字符串结尾\0也视作空白并跳过*/\
	while (i < szDataSize && (IsSpace(tpData[i]) || IsDelimiter(tpData[i])))\
	{\
		++i;\
	}\
	\
	/*如果是因为i < sStr.Size()不满足，而不是遇到了一个非空白字符离开的while，则失败*/\
	if (i >= szDataSize)\
	{\
		__VA_ARGS__;\
		break;\
	}\
	\
	c = tpData[i++];\
}

		while (true)
		{
			char h, l;

			GetNoSpaceChar(h);//高位不存在则退出
			GetNoSpaceChar(l,//低位不存在，仅有高位
				tRet.Push(CharsToHex(h, '0')));//低位补0，然后退出

			//高低位合并
			tRet.Push(CharsToHex(h, l));
		}

		return tRet.Raw();
	}

public:
	template<StringLiteral sStr, typename T = uint8_t>
	static consteval auto ToHexArr(void) noexcept
	{
		constexpr auto szArrSize = Parse<Counter>(sStr.data(), sStr.size());
		return Parse<Array<T, szArrSize>>(sStr.data(), sStr.size());
	}

	static size_t ToHexArrLength(const std::string_view &viewStr)
	{
		return Parse<Counter>(viewStr.data(), viewStr.size());
	}

	template<typename RetT = std::vector<uint8_t>>
	static auto ToHexArr(const std::string_view &viewStr, size_t szReserve = 0)
	{
		 return Parse<DynamicArray<RetT>>(viewStr.data(), viewStr.size(), { szReserve });
	}

	template<typename RetT = std::vector<uint8_t>, typename T = char>
	static auto ToHexArr(const T *tpData, size_t szDataSize, size_t szReserve = 0)
	{
		return Parse<DynamicArray<RetT>>(tpData, szDataSize, { szReserve });
	}
};