// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	HMODULE hModuleTemp			= NULL;
	TCHAR	tchPath[MAX_PATH]	= {0};


	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			printf("[%s] hModule %p \n", __FUNCTION__, hModule);

			GetModulePath(hModule, tchPath, _countof(tchPath));
			GetModulePath(NULL, tchPath, _countof(tchPath));

			__try
			{
				hModuleTemp = LoadLibrary(L"DynamicLibrary2.dll");
				if (!hModuleTemp)
				{
					printf("[%s] LoadLibrary failed. (%d) \n", __FUNCTION__, GetLastError());
					__leave;
				}
			}
			__finally
			{
				if (hModuleTemp)
				{
					FreeLibrary(hModuleTemp);
					hModuleTemp = NULL;
				}
			}

			break;
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
