#pragma once

#include <stdio.h>//printf
#include <stdlib.h>//exit
#include <stdarg.h>//va_arg

//必须没有提前定义过任何此类宏
#if !defined(__COMPILER_MSVC__) &&\
	!defined(__COMPILER_GCC__) &&\
	!defined(__COMPILER_CLANG__)

	//先预定义所有可能的编译器宏
	#define __COMPILER_MSVC__ 0
	#define __COMPILER_GCC__ 0
	#define __COMPILER_CLANG__ 0
	
	//后实际判断是哪个编译器，是就替换它自己的宏为1
	#if defined(_MSC_VER)
		#undef  __COMPILER_MSVC__
		#define __COMPILER_MSVC__ 1
		#define __COMPILER_NAME__ "MSVC"
	#elif defined(__GNUC__)
		#undef  __COMPILER_GCC__
		#define __COMPILER_GCC__ 1
		#define __COMPILER_NAME__ "GCC"
	#elif defined(__clang__)
		#undef  __COMPILER_CLANG__
		#define __COMPILER_CLANG__ 1
		#define __COMPILER_NAME__ "Clang"
	#else
		#define __COMPILER_NAME__ "Unknown"
	#endif
#endif


#if __COMPILER_MSVC__
#define PRINTF_FORMAT_ARGS _Printf_format_string_
#define PRINTF_FORMAT_ATTR
#elif __COMPILER_GCC__ || __COMPILER_CLANG__
#define PRINTF_FORMAT_ARGS
#define PRINTF_FORMAT_ATTR __attribute__((__format__ (__printf__, 5, 6)))
#else
#define PRINTF_FORMAT_ARGS
#define PRINTF_FORMAT_ATTR
#endif


PRINTF_FORMAT_ATTR
inline void __MyAssert_Function__(const char *pFileName, size_t szLine, const char *pFunctionName, bool bValid, PRINTF_FORMAT_ARGS const char *pInfo = NULL, ...)
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

//cpp20的__VA_OPT__(,)，仅在__VA_ARGS__不为空时添加','以防止编译错误
//msvc需启用"/Zc:preprocessor"以使得预处理器识别此宏关键字（哎呀msvc你怎么这么坏呀）
#define MyAssert(v, ...) __MyAssert_Function__(__MY_ASSERT_FILE__, __MY_ASSERT_LINE__, __MY_ASSERT_FUNC__, (v) __VA_OPT__(,) __VA_ARGS__)

#undef PRINTF_FORMAT_ATTR
#undef PRINTF_FORMAT_ARGS

#undef __COMPILER_NAME__
#undef __COMPILER_CLANG__
#undef __COMPILER_GCC__
#undef __COMPILER_MSVC__