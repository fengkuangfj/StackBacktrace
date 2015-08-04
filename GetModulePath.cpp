#include "GetModulePath.h"

BOOL
	GetModulePath(
	__in HMODULE	hModule,
	__in LPTSTR		lpInBuf,
	__in ULONG		ulInBufSizeCh
	)
{
	BOOL	bRet		= FALSE;

	DWORD	dwResult	= 0;


	printf("[%s] begin \n", __FUNCTION__);

	__try
	{
		if (!lpInBuf || !ulInBufSizeCh)
		{
			printf("input arguments error. lpInBuf(%p) ulInBufSizeCh(%d) \n", lpInBuf, ulInBufSizeCh);
			__leave;
		}

		if (!hModule)
		{
			printf("the file used to create the calling process \n");
			hModule = GetModuleHandle(NULL);
			if (!hModule)
			{
				printf("GetModuleHandle failed. (%d) \n", GetLastError());
				__leave;
			}
		}
		else
			printf("the fully qualified path of the module \n");

		dwResult = GetModuleFileName(hModule, lpInBuf, ulInBufSizeCh);
		if (!dwResult)
		{
			printf("GetModuleFileName failed. (%d) \n", GetLastError());
			__leave;
		}
		else
			printf("%S \n", lpInBuf);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	printf("[%s] end \n", __FUNCTION__);

	return bRet;
}
