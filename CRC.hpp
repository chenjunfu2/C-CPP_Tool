#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <utility>
#include <type_traits>
#include <concepts>
#include <span>
#include <bit>

namespace CRC_TOOLS
{
	template<size_t>
	inline constexpr bool AlwaysFalse = false;//msvc bug

	template<size_t BitsOfType>
	struct TypeSelector
	{
		static_assert(AlwaysFalse<BitsOfType>, "Unsupported bit width");// msvc 19.40起才修复，目前只能套一层
	};

	template<>
	struct TypeSelector<64>
	{
		using Type = uint64_t;
	};

	template<>
	struct TypeSelector<32>
	{
		using Type = uint32_t;
	};

	template<>
	struct TypeSelector<16>
	{
		using Type = uint16_t;
	};

	template<>
	struct TypeSelector<8>
	{
		using Type = uint8_t;
	};

	template<size_t BitsOfType>
	struct TypeSwitch : public TypeSelector<BitsOfType>
	{
		static constexpr size_t szTypeBits = BitsOfType;
	};


	template <typename T, size_t Shift>
	requires (std::unsigned_integral<T> &&std::has_single_bit(sizeof(T) * 8) && std::has_single_bit(Shift))
	constexpr T MaskR = []() noexcept -> T
	{
		constexpr size_t szBits = sizeof(T) * 8;

		T tValue = 0;
		T tBitOn = 1;

		for (size_t i = 0; i < szBits; ++i)
		{
			size_t szGroup = i / Shift;
			if (szGroup % 2 == 0)//交替组
			{
				tValue |= tBitOn;
			}

			tBitOn <<= 1;
		}

		return tValue;
	}();

	template <typename T, size_t Shift>
	requires (std::unsigned_integral<T> &&std::has_single_bit(sizeof(T) * 8) && std::has_single_bit(Shift))
	constexpr T MaskL = []() noexcept -> T
	{
		constexpr size_t szBits = sizeof(T) * 8;

		T tValue = 0;
		T tBitOn = 1;

		for (size_t i = 0; i < szBits; ++i)
		{
			size_t szGroup = i / Shift;
			if (szGroup % 2 == 1)//交替组
			{
				tValue |= tBitOn;
			}

			tBitOn <<= 1;
		}

		return tValue;
	}();

	/*
	bits reverse
	12345678
	
	1|2 3|4 5|6 7|8
	2|1 4|3 6|5 8|7
	
	21|43 65|87
	43|21 87|65
	
	4321|8765
	8765|4321
	
	87654321
	*/
	template<typename T>
	requires (std::integral<T> &&std::has_single_bit(sizeof(T) * 8))
	inline constexpr T ReverseBits(T tBits) noexcept
	{
		using UT = typename std::make_unsigned_t<T>;
		UT uTmp = std::bit_cast<UT>(tBits);

		constexpr size_t szBits = sizeof(T) * 8;//确定有多少bits
		constexpr size_t szExponent = std::countr_zero(szBits);//确定数值由多少个2组成（2的指数）

		[&] <size_t... i>(std::index_sequence<i...>) -> void
		{
			((uTmp = (uTmp & MaskR<UT, (1ULL << i)>) << (1ULL << i) | (uTmp & MaskL<UT, (1ULL << i)>) >> (1ULL << i)), ...);
		}(std::make_index_sequence<szExponent>{});

		return std::bit_cast<T>(uTmp);
	}

	/*
	//已弃用，作为ReverseBits模板的直观解释
	uint64_t ReverseBits64(uint64_t u64Bits)
	{

		constexpr uint64_t u64MaskR1 = 0x55'55'55'55'55'55'55'55ULL;//0101
		constexpr uint64_t u64MaskL1 = u64MaskR1 << 1;//1010

		constexpr uint64_t u64MaskR2 = 0x33'33'33'33'33'33'33'33ULL;//0011
		constexpr uint64_t u64MaskL2 = u64MaskR2 << 2;//1100

		constexpr uint64_t u64MaskR4 = 0x0F'0F'0F'0F'0F'0F'0F'0FULL;
		constexpr uint64_t u64MaskL4 = u64MaskR4 << 4;

		constexpr uint64_t u64MaskR8 = 0x00'FF'00'FF'00'FF'00'FFULL;
		constexpr uint64_t u64MaskL8 = u64MaskR8 << 8;

		constexpr uint64_t u64MaskR16 = 0x00'00'FF'FF'00'00'FF'FFULL;
		constexpr uint64_t u64MaskL16 = u64MaskR16 << 16;

		constexpr uint64_t u64MaskR32 = 0x00'00'00'00'FF'FF'FF'FFULL;
		constexpr uint64_t u64MaskL32 = u64MaskR32 << 32;

		uint64_t u64Tmp = u64Bits;
		u64Tmp = (u64Tmp & u64MaskR1) << 1 | (u64Tmp & u64MaskL1) >> 1;
		u64Tmp = (u64Tmp & u64MaskR2) << 2 | (u64Tmp & u64MaskL2) >> 2;
		u64Tmp = (u64Tmp & u64MaskR4) << 4 | (u64Tmp & u64MaskL4) >> 4;
		u64Tmp = (u64Tmp & u64MaskR8) << 8 | (u64Tmp & u64MaskL8) >> 8;
		u64Tmp = (u64Tmp & u64MaskR16) << 16 | (u64Tmp & u64MaskL16) >> 16;
		u64Tmp = (u64Tmp & u64MaskR32) << 32 | (u64Tmp & u64MaskL32) >> 32;

		return u64Tmp;
	}
	*/
}

template<size_t CRCBits>
requires (std::has_single_bit(CRCBits) && CRCBits >= 8)//2的次方并且至少8
class CRC
{
protected:
	static_assert(CHAR_BIT == 8, "Unsupported platform");
	using CRCTypeSwitch = CRC_TOOLS::TypeSwitch<CRCBits>;

public:
	using CRCType = CRCTypeSwitch::Type;
	static constexpr size_t szTypeBits = CRCTypeSwitch::szTypeBits;

protected:
	bool bRefIn = false;
	CRCType tCrcState = 0;
	CRCType tCrcTable[UINT8_MAX + 1] = {};

protected:
	//状态迁移
	template<bool bReflection>
	constexpr void UpdateCrcStateImpl(uint8_t u8Data) noexcept
	{
		if constexpr (szTypeBits == 8)//8bit特化
		{
			uint8_t u8Index = (uint8_t)tCrcState ^ u8Data;
			tCrcState = tCrcTable[u8Index];
		}
		else//其它大小
		{
			uint8_t u8Index = 0;

			if constexpr (bReflection)
			{
				u8Index = (uint8_t)tCrcState ^ u8Data;
				tCrcState >>= 8;
			}
			else
			{
				u8Index = (uint8_t)(tCrcState >> (szTypeBits - 8)) ^ u8Data;
				tCrcState <<= 8;
			}

			tCrcState ^= tCrcTable[u8Index];
		}
	}

public:
	//预计算8bit crc状态迁移表
	constexpr void ResetCrcTable(CRCType tPoly, bool _bRefIn = true) noexcept
	{
		bRefIn = _bRefIn;

		if (bRefIn)
		{
			tPoly = CRC_TOOLS::ReverseBits(tPoly);
			for (uint64_t i = 0; i <= UINT8_MAX; ++i)
			{
				CRCType tCrcStateTmp = i & UINT8_MAX;

				for (uint8_t j = 0; j < 8; ++j)
				{
					tCrcStateTmp = (tCrcStateTmp & 0x01ULL) == 0x01ULL
						? (tCrcStateTmp >> 1) ^ tPoly
						: (tCrcStateTmp >> 1);
				}

				tCrcTable[i] = tCrcStateTmp;
			}
		}
		else
		{
			constexpr CRCType tUpperBit = ((CRCType)1) << (szTypeBits - 1);
			for (uint64_t i = 0; i <= UINT8_MAX; ++i)
			{
				CRCType tCrcStateTmp = (i & UINT8_MAX) << (szTypeBits - 8);

				for (uint8_t j = 0; j < 8; ++j)
				{
					tCrcStateTmp = (tCrcStateTmp & tUpperBit) == tUpperBit
						? (tCrcStateTmp << 1) ^ tPoly
						: (tCrcStateTmp << 1);
				}

				tCrcTable[i] = tCrcStateTmp;
			}
		}
	}

	//指定初始状态开始计算
	constexpr void ResetCrcState(CRCType tInit) noexcept
	{
		tCrcState = tInit;
	}

	//迭代（字节)
	constexpr void UpdateCrcState(uint8_t u8Data) noexcept
	{
		bRefIn
			? UpdateCrcStateImpl<true>(u8Data)
			: UpdateCrcStateImpl<false>(u8Data);
	}

	//迭代（内存）
	constexpr void UpdateCrcState(const void *pData, size_t szDataSize) noexcept
	{
		const uint8_t *pu8Data = (const uint8_t *)pData;

		if (bRefIn)
		{
			for (size_t i = 0; i < szDataSize; ++i)
			{
				UpdateCrcStateImpl<true>(pu8Data[i]);
			}
		}
		else
		{
			for (size_t i = 0; i < szDataSize; ++i)
			{
				UpdateCrcStateImpl<false>(pu8Data[i]);
			}
		}
	}

	//迭代（视图）
	template<typename T, size_t N>
	requires (sizeof(T) == 1 && !std::is_same_v<std::remove_cvref_t<T>, bool>)
	constexpr void UpdateCrcState(std::span<T, N> spanData) noexcept
	{
		if (bRefIn)
		{
			for (const auto & tByte: spanData)
			{
				UpdateCrcStateImpl<true>(std::bit_cast<uint8_t>(tByte));
			}
		}
		else
		{
			for (const auto &tByte : spanData)
			{
				UpdateCrcStateImpl<false>(std::bit_cast<uint8_t>(tByte));
			}
		}
	}

	//迭代（数组）
	template<typename T, size_t N>
	constexpr void UpdateCrcState(const T(&arrData)[N]) noexcept
	{
		UpdateCrcState((const void *)&arrData[0], sizeof(arrData));
	}

	//得到当前迭代的CRC结果
	[[nodiscard]]
	constexpr CRCType GetCrcState(bool bRefOut = true, CRCType tXorOut = 0) noexcept
	{
		if (bRefOut != bRefIn)//等效于bRefOut ^ bRefIn
		{
			return CRC_TOOLS::ReverseBits(tCrcState) ^ tXorOut;
		}
		else
		{
			return tCrcState ^ tXorOut;
		}
	}
};

//简易计算CRC，可编译期
template<typename CRC_T, typename T, size_t N>
requires (sizeof(T) == 1 && !std::is_same_v<std::remove_cvref_t<T>, bool>)
constexpr inline auto SimpleCrc
(
	std::span<T, N> spanData,
	typename CRC_T::CRCType tPoly,
	typename CRC_T::CRCType tInit,
	typename CRC_T::CRCType tXorOut = 0,
	bool bRefIn = true,
	bool bRefOut = true
)
{
	CRC_T crc{};
	crc.ResetCrcTable(tPoly, bRefIn);
	crc.ResetCrcState(tInit);
	crc.UpdateCrcState(spanData);
	return crc.GetCrcState(bRefOut, tXorOut);
}

//实例化提供

using CRC64 = CRC<64>;
using CRC32 = CRC<32>;
using CRC16 = CRC<16>;
using CRC8 = CRC<8>;
