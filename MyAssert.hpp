#pragma once

#include <stdio.h>
#include <stdarg.h>

inline void __MyAssert_Function__(const char *pFileName, size_t szLine, const char *pFunctionName, bool bValid, const char *pInfo = NULL, ...)
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
#define __MY_ASSERT_FUNC__ __FUNCTION__

//cpp20 __VA_OPT__(,)，仅在__VA_ARGS__不为空时添加','以防止编译错误
//msvc需启用/Zc:preprocessor以使得预处理器识别此宏关键字
#define MyAssert(v, ...) __MyAssert_Function__(__MY_ASSERT_FILE__, __MY_ASSERT_LINE__, __MY_ASSERT_FUNC__, (v) __VA_OPT__(,) __VA_ARGS__)
