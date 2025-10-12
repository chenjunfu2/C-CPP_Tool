#pragma once

#include <stdio.h>
#include <stdarg.h>

//inline防止重定义，_Printf_format_string_用于触发编译器参数类型匹配检查
inline void __MyAssert_Function__(const char *pFileName, size_t szLine, const char *pFunctionName, bool bValid, _Printf_format_string_ const char *pInfo = NULL, ...)
{
	if (bValid)
	{
		return;//校验成功返回
	}

	printf("Assertion Failure!\n    in file: %s\n    in line: %zu\n    in func: %s\n    in info: ", pFileName, szLine, pFunctionName);
	
	if (pInfo != NULL)
	{
		va_list vl;
		va_start(vl, pInfo);
		vprintf(pInfo, vl);
		va_end(vl);
	}
	else
	{
		printf("[No Info]");
	}
	
	//帮忙换行
	putchar('\n');

	exit(-114514);
}

//代理宏，延迟展开
#define __MY_ASSERT_FILE__ __FILE__
#define __MY_ASSERT_LINE__ __LINE__
#define __MY_ASSERT_FUNC__ __FUNCSIG__//使用完整函数名


#define MyAssert(v, ...) __MyAssert_Function__(__MY_ASSERT_FILE__, __MY_ASSERT_LINE__, __MY_ASSERT_FUNC__, (v), __VA_ARGS__)
