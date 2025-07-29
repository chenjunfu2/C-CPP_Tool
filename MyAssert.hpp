#pragma once

#include <stdio.h>
#include <varargs.h>

void __MyAssert_Function__(const char *pFileName, size_t szLine, const char *pFunctionName, bool bValid, _Printf_format_string_ const char *pInfo, ...)
{
	if (bValid)
	{
		return;
	}

	printf("Assertion Failure!\n\tin file: %s\n\tin line: %zu\n\tin func: %s\n\tin info: ", pFileName, szLine, pFunctionName);

	va_list vl;
	va_start(vl, pInfo);
	vprintf(pInfo, vl);
	va_end(vl);

	putchar('\n');

	exit(-114514);
}

//代理宏，延迟展开
#define __MY_ASSERT_LINE__ __LINE__
#define __MY_ASSERT_FILE__ __FILE__
#define __MY_ASSERT_FUNC__ __FUNCTION__

#define MyAssert(v, i, ...) __MyAssert_Function__(__MY_ASSERT_FILE__, __MY_ASSERT_LINE__, __MY_ASSERT_FUNC__, (v), (i), __VA_ARGS__)