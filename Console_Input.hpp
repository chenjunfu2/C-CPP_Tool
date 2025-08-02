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

class Console_Input//�û�����
{
public:
	enum LeadCode : uint16_t//ָ��u16����
	{
		Code_NL = 0,
		Code_00 = 1,
		Code_E0 = 2,
	};

	struct Key//������u16����Key�պ���32bit
	{//[ǰ���ֽ�][������]
		uint16_t u16KeyCode;
		LeadCode enLeadCode = Code_NL;//��Ĭ��ֵ����ֻ��ʼ��u16KeyCode

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
			//��������£�u16KeyCodeֻ����0~255����LeadCodeֻ����0~3
			uint16_t u16KeyCode = u16KeyCode & 0x00FF;//ȥ����8bit
			uint16_t u16LeadCode = enLeadCode & 0x0003;//ֻ����2bit

			uint16_t u16HashCode = u16KeyCode | u16LeadCode << 8;//��2bit�ƶ���8bitǰ�����10bit
			return std::hash<uint16_t>{}(u16HashCode);//��hash
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

	//�����ƶ�
	Console_Input(Console_Input &&) = default;
	Console_Input &operator = (Console_Input &&) = default;

	//��ֹ����
	Console_Input(const Console_Input &) = delete;
	Console_Input &operator = (const Console_Input &) = delete;

	[[noreturn]] static void KeyCodeTest(void) noexcept//�������᷵��
	{
		/*
			�������ҷ����
			0xE0��ʼ���
				 0x48
			0x4B 0x50 0x4D

			���ϰ�������0x00��ͷ����0xE0��ͷ
		*/
		while (true)
		{
			printf("0x%02X ", _getch());
		}
	}

	//ע������ظ�ע�������µİ����滻��ɵ�
	void RegisterKey(const Key &stKey, Func fFunc)
	{
		mapRegisterTable[stKey] = fFunc;
	}

	//ͨ������ע����ͬ���ܰ���
	void CopyRegisteredKey(const Key &stTarget, const Key &stSource)
	{
		mapRegisterTable[stTarget] = mapRegisterTable[stSource];
	}

	//ȡ��ע��
	void UnRegisterKey(const Key &stKey) noexcept
	{
		mapRegisterTable.erase(stKey);
	}

	//��ѯ�Ƿ��Ѿ�ע��
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
		int iInput = _getch();//��ȡ��һ������
		switch (iInput)
		{
		case 0x00://ת��
			stKeyRet.enLeadCode = Code_00;
			stKeyRet.u16KeyCode = _getch();//���»�ȡ
			break;
		case 0xE0://ת��
			stKeyRet.enLeadCode = Code_E0;
			stKeyRet.u16KeyCode = _getch();//���»�ȡ
			break;
		case EOL:
			throw std::runtime_error("Error: _getch() return EOL!");
		default://��������
			stKeyRet.enLeadCode = Code_NL;
			stKeyRet.u16KeyCode = iInput;//�������»�ȡ��ֱ�Ӿ��ǰ���
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

		//ִ�е���˵���Ѿ��ȵ�Ŀ���
		return;
	}

	static Key WaitForKeys(const std::unordered_set<Key, KeyHash> &setKeysWait)
	{
		Key stKeyGet;
		do
		{
			stKeyGet = GetTranslateKey();
		} while (!setKeysWait.contains(stKeyGet));

		//ִ�е���˵���Ѿ��ȵ���һĿ���
		return stKeyGet;//˳�㷵��һ�����û�֪�����ĸ�
	}

	long Once(void) const//����֤�����᲻���׳��쳣
	{
		Key stKetGet = GetTranslateKey();

		//��ȡ����
		auto it = mapRegisterTable.find(stKetGet);
		if (it == mapRegisterTable.end())
		{
			return 0;
		}

		//��Ϊ�������
		long lRet = it->second(it->first);
		if (lRet != 0)//��Ϊ0��Ϊ�쳣����
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