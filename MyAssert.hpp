#pragma once

#include <stdio.h>
#include <varargs.h>

//inline��ֹ�ض��壬_Printf_format_string_���ڴ�����������������ƥ����
inline void __MyAssert_Function__(const char *pFileName, size_t szLine, const char *pFunctionName, bool bValid, _Printf_format_string_ const char *pInfo, ...)
{
	if (bValid)
	{
		return;//У��ɹ�����
	}

	printf("Assertion Failure!\n    in file: %s\n    in line: %zu\n    in func: %s\n    in info: ", pFileName, szLine, pFunctionName);
	
	va_list vl;
	va_start(vl, pInfo);
	vprintf(pInfo, vl);
	va_end(vl);
	//��æ����
	putchar('\n');

	exit(-114514);
}

//����꣬�ӳ�չ��
#define __MY_ASSERT_LINE__ __LINE__
#define __MY_ASSERT_FILE__ __FILE__
#define __MY_ASSERT_FUNC__ __FUNCSIG__//ʹ������������

#define MyAssert(v, i, ...) __MyAssert_Function__(__MY_ASSERT_FILE__, __MY_ASSERT_LINE__, __MY_ASSERT_FUNC__, (v), (i), __VA_ARGS__)