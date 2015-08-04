// Application.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

BOOL
	TestGetModuleName()
{
	BOOL	bRet				= FALSE;

	HMODULE hModule				= NULL;
	TCHAR	tchPath[MAX_PATH]	= {0};


	__try
	{
		GetModulePath(NULL, tchPath, _countof(tchPath));

		hModule = LoadLibrary(L"DynamicLibrary1.dll");
		if (!hModule)
		{
			printf("[%s] LoadLibrary failed. (%d) \n", __FUNCTION__, GetLastError());
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hModule)
		{
			FreeLibrary(hModule);
			hModule = NULL;
		}
	}

	return bRet;
}

VOID
	Func3()
{
	printf("[%s] 0x%08p \n", __FUNCTION__, Func3);

	// WalkFrameChaim();
	StackBacktrace();
}

VOID
	Func2()
{
	printf("[%s] 0x%08p \n", __FUNCTION__, Func2);

	Func3();
}

VOID
	Func1()
{
	printf("[%s] 0x%08p \n", __FUNCTION__, Func1);

	Func2();
}

BOOL
	TestStackBachtrace()
{
	BOOL bRet = FALSE;


	printf("[%s] 0x%08p \n", __FUNCTION__, TestStackBachtrace);

	__try
	{
		Func1();

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("[%s] 0x%08p \n", __FUNCTION__, _tmain);

	TestGetModuleName();

// 	Init();
// 
// 	TestStackBachtrace();
// 
// 	Unload();

	_getch();

	return 0;
}
