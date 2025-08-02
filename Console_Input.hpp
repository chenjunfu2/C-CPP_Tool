#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define EOL -1

class Console_Input//用户交互
{
public:
	enum LeadCode : uint16_t//指定u16类型
	{
		Code_NL = 0,
		Code_00 = 1,
		Code_E0 = 2,
	};

	struct Key//用两个u16这样Key刚好是32bit
	{//[前导字节][按键码]
		uint16_t u16KeyCode;
		LeadCode enLeadCode = Code_NL;//带默认值方便只初始化u16KeyCode

		bool operator==(const Key &_Right) const noexcept
		{
			return u16KeyCode == _Right.u16KeyCode && enLeadCode == _Right.enLeadCode;
		}

		bool operator!=(const Key &_Right) const noexcept
		{
			return u16KeyCode != _Right.u16KeyCode || enLeadCode != _Right.enLeadCode;
		}

		size_t Hash() const noexcept
		{
			//正常情况下，u16KeyCode只会在0~255，而LeadCode只会在0~3
			uint16_t u16KeyCode = u16KeyCode & 0x00FF;//去掉高8bit
			uint16_t u16LeadCode = enLeadCode & 0x0003;//只留低2bit

			uint16_t u16HashCode = u16KeyCode | u16LeadCode << 8;//把2bit移动到8bit前面组成10bit
			return std::hash<uint16_t>{}(u16HashCode);//求hash
		}
	};

	struct KeyHash
	{
		size_t operator()(const Key &stKeyHash)
		{
			return stKeyHash.Hash();
		}
	};

	using CallBackFunc = long(const Key& stey);
	using Func = std::function<CallBackFunc>;
private:
	std::unordered_map<Key, Func, KeyHash> mapRegisterTable;
public:
	Console_Input(void) = default;
	~Console_Input(void) = default;

	//可以移动
	Console_Input(Console_Input &&) = default;
	Console_Input &operator = (Console_Input &&) = default;

	//禁止拷贝
	Console_Input(const Console_Input &) = delete;
	Console_Input &operator = (const Console_Input &) = delete;

	[[noreturn]] static void KeyCodeTest(void) noexcept//函数不会返回
	{
		/*
			上下左右方向键
			0xE0开始后跟
				 0x48
			0x4B 0x50 0x4D

			复合按键不是0x00开头就是0xE0开头
		*/
		while (true)
		{
			printf("0x%02X ", _getch());
		}
	}

	//注册键，重复注册则最新的按键替换最旧的
	void RegisterKey(const Key &stKey, Func fFunc)
	{
		mapRegisterTable[stKey] = fFunc;
	}

	//通过拷贝注册相同功能按键
	void CopyRegisteredKey(const Key &stTarget, const Key &stSource)
	{
		mapRegisterTable[stTarget] = mapRegisterTable[stSource];
	}

	//取消注册
	void UnRegisterKey(const Key &stKey) noexcept
	{
		mapRegisterTable.erase(stKey);
	}

	//查询是否已经注册
	bool IsKeyRegister(const Key &stKey) const noexcept
	{
		return mapRegisterTable.contains(stKey);
	}

	void Reset(void) noexcept
	{
		mapRegisterTable.clear();
	}

	static Key GetTranslateKey(void)
	{
		Key stKeyRet;
		int iInput = _getch();//获取第一次输入
		switch (iInput)
		{
		case 0x00://转义
			stKeyRet.enLeadCode = Code_00;
			stKeyRet.u16KeyCode = _getch();//重新获取
			break;
		case 0xE0://转义
			stKeyRet.enLeadCode = Code_E0;
			stKeyRet.u16KeyCode = _getch();//重新获取
			break;
		case EOL:
			throw std::runtime_error("Error: _getch() return EOL!");
		default://正常按键
			stKeyRet.enLeadCode = Code_NL;
			stKeyRet.u16KeyCode = iInput;//不用重新获取，直接就是按键
			break;
		}

		return stKeyRet;
	}

	static void WaitForKey(Key stKeyWait)
	{
		Key stKeyGet;
		do
		{
			stKeyGet = GetTranslateKey();
		} while (stKeyGet != stKeyWait);

		//执行到此说明已经等到目标键
		return;
	}

	static Key WaitForKeys(const std::unordered_set<Key, KeyHash> &setKeysWait)
	{
		Key stKeyGet;
		do
		{
			stKeyGet = GetTranslateKey();
		} while (!setKeysWait.contains(stKeyGet));

		//执行到此说明已经等到任一目标键
		return stKeyGet;//顺便返回一下让用户知道是哪个
	}

	long Once(void) const//不保证函数会不会抛出异常
	{
		Key stKetGet = GetTranslateKey();

		//获取函数
		auto it = mapRegisterTable.find(stKetGet);
		if (it == mapRegisterTable.end())
		{
			return 0;
		}

		//不为空则调用
		long lRet = it->second(it->first);
		if (lRet != 0)//不为0即为异常返回
		{
			return lRet;
		}
	}

	long Loop(void) const
	{
		long lRet = 0;

		do
		{
			lRet = Once();
		} while (lRet == 0);

		return lRet;
	}

	static void WaitAnyKey(void) noexcept
	{
		(void)_getch();
	}
};

#undef EOL