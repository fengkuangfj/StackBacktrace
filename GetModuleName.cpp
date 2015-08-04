#include "GetModuleName.h"

BOOL
	GetModuleName(
	__in HMODULE hModule
	)
{
	BOOL	bRet					= FALSE;

	DWORD	dwResult				= 0;
	TCHAR	tchFileName[MAX_PATH]	= {0};
	TCHAR	tchProcName[MAX_PATH]	= {0};


	printf("[%s] begin \n", __FUNCTION__);

	__try
	{
		if (hModule)
		{
			printf("hModule 0x%08p \n", hModule);

			printf("the fully qualified path of the module \n");
			dwResult = GetModuleFileName(hModule, tchFileName, _countof(tchFileName));
			if (!dwResult)
			{
				printf("GetModuleFileName failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("%S \n", tchFileName);
		}
		else
		{
			printf("a handle to the file used to create the calling process \n");
			hModule = GetModuleHandle(NULL);
			if (!hModule)
			{
				printf("GetModuleHandle failed. (%d) \n", GetLastError());
				__leave;
			}

			printf("hModule 0x%08p \n", hModule);

			dwResult = GetModuleFileName(hModule, tchProcName, _countof(tchProcName));
			if (!dwResult)
			{
				printf("GetModuleFileName failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("%S \n", tchProcName);
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	printf("[%s] end \n", __FUNCTION__);

	return bRet;
}
