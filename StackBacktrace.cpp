#include "StackBacktrace.h"

RTLWALKFRAMECHAIN	RtlWalkFrameChain	= NULL;
HANDLE				g_hProcess			= NULL;

BOOL
	WalkFrameChaim()
{
	BOOL			bRet						= FALSE;

	PVOID			ReturnAddress[MAX_PATH]		= {0};
	ULONG			FrameCount					= 0;
	ULONG			FrameNumber					= 0;
	HMODULE			hModule						= NULL;
	DWORD64			dw64Displacement			= 0;
	PSYMBOL_INFO	pSymbol						= NULL;
	DWORD           dwDisplacement				= 0;
	IMAGEHLP_LINE64 Line						= {0};
	CHAR			chDecoratedName[MAX_PATH]	= {0};
	CONTEXT         Context						= {0};
	HANDLE			hThread						= NULL;


	printf("[%s] begin \n", __FUNCTION__);
	printf("[%s] 0x%08p \n", __FUNCTION__, WalkFrameChaim);

	__try
	{
		hThread = GetCurrentThread();

		if (!RtlWalkFrameChain)
		{
			hModule = GetModuleHandle(L"ntdll.dll");
			if (!hModule)
			{
				printf("GetModuleHandle failed. (%d) \n", GetLastError());
				__leave;
			}

			RtlWalkFrameChain = (RTLWALKFRAMECHAIN)GetProcAddress(hModule, "RtlWalkFrameChain");
			if (!RtlWalkFrameChain)
			{
				printf("GetProcAddress failed. (%d) \n", GetLastError());
				__leave;
			}
		}

		FrameCount = RtlWalkFrameChain(ReturnAddress, _countof(ReturnAddress), 0);
		printf("StackCount %d \n", FrameCount);
		for (; FrameNumber < FrameCount; FrameNumber++)
		{
			printf("Stack[%d] 0x%08p \n", FrameNumber, ReturnAddress[FrameNumber]);

			if (pSymbol)
			{
				free(pSymbol);
				pSymbol = NULL;
			}

			pSymbol = (PSYMBOL_INFO)calloc(1, sizeof(SYMBOL_INFO) - sizeof(CHAR) + MAX_PATH);
			if (!pSymbol)
			{
				printf("calloc failed. (%d) \n", GetLastError());
				__leave;
			}

			pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			pSymbol->MaxNameLen = MAX_PATH;

			if (!SymFromAddr(
				g_hProcess,
				(DWORD64)ReturnAddress[FrameNumber],
				&dw64Displacement,
				pSymbol
				))
			{
				printf("SymFromAddr failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("Name %s Address 0x%08p \n", pSymbol->Name, pSymbol->Address);

			if (!UnDecorateSymbolName(
				pSymbol->Name,
				chDecoratedName,
				_countof(chDecoratedName),
				0
				))
			{
				printf("UnDecorateSymbolName failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("SymbolName %s \n", chDecoratedName);

			Context.ContextFlags = CONTEXT_FULL;
			if (!GetThreadContext(hThread, &Context))
			{
				printf("GetThreadContext failed. (%d) \n", GetLastError());
				__leave;
			}

			Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (!SymGetLineFromAddr64(
				g_hProcess,
				Context.Eip,
				&dwDisplacement,
				&Line
				))
			{
				if (487 == GetLastError())
					printf("487 \n");
				else
				{
					printf("SymGetLineFromAddr64 failed. (%d) \n", GetLastError());
					__leave;
				}
			}
			else
			{
				printf("FileName %s \n", Line.FileName);
				printf("LineNumber %d \n", Line.LineNumber);
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pSymbol)
		{
			free(pSymbol);
			pSymbol = NULL;
		}
	}

	printf("[%s] end \n", __FUNCTION__);

	return bRet;
}

BOOL
	Init()
{
	BOOL bRet = FALSE;


	__try
	{
		g_hProcess = GetCurrentProcess();

		if (!SymInitialize(g_hProcess, "G:\\GitHub\\StackBacktrace\\Application\\Debug", FALSE))
		{
			printf("SymInitialize failed. (%d) \n", GetLastError());
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			if (g_hProcess)
			{
				SymCleanup(g_hProcess);
				g_hProcess = NULL;
			}
		}
	}

	return bRet;
}

BOOL
	Unload()
{
	BOOL bRet = FALSE;


	__try
	{
		if (g_hProcess)
		{
			SymCleanup(g_hProcess);
			g_hProcess = NULL;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOL
	StackBacktrace()
{
	BOOL			bRet						= FALSE;

	STACKFRAME64	StackFrame64				= {0};
	CONTEXT         Context						= {0};
	
	HANDLE			hThread						= NULL;
	DWORD64			dw64Displacement			= 0;
	PSYMBOL_INFO	pSymbol						= NULL;
	DWORD           dwDisplacement				= 0;
	IMAGEHLP_LINE64 Line						= {0};
	CHAR			chDecoratedName[MAX_PATH]	= {0};


	printf("[%s] begin \n", __FUNCTION__);
	printf("[%s] 0x%08p \n", __FUNCTION__, StackBacktrace);
	
	__try
	{
		hThread = GetCurrentThread();

		Context.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(hThread, &Context))
		{
			printf("GetThreadContext failed. (%d) \n", GetLastError());
			__leave;
		}

// #ifdef _M_IX86  
// 		sf.AddrPC.Offset = c.Eip;  
// 		sf.AddrPC.Mode = AddrModeFlat;  
// 		sf.AddrStack.Offset = c.Esp;  
// 		sf.AddrStack.Mode = AddrModeFlat;  
// 		sf.AddrFrame.Offset = c.Ebp;  
// 		sf.AddrFrame.Mode = AddrModeFlat;  
// #elif _M_X64  
// 		dwImageType = IMAGE_FILE_MACHINE_AMD64;  
// 		sf.AddrPC.Offset = c.Rip;  
// 		sf.AddrPC.Mode = AddrModeFlat;  
// 		sf.AddrFrame.Offset = c.Rsp;  
// 		sf.AddrFrame.Mode = AddrModeFlat;  
// 		sf.AddrStack.Offset = c.Rsp;  
// 		sf.AddrStack.Mode = AddrModeFlat;  
// #elif _M_IA64  
// 		dwImageType = IMAGE_FILE_MACHINE_IA64;  
// 		sf.AddrPC.Offset = c.StIIP;  
// 		sf.AddrPC.Mode = AddrModeFlat;  
// 		sf.AddrFrame.Offset = c.IntSp;  
// 		sf.AddrFrame.Mode = AddrModeFlat;  
// 		sf.AddrBStore.Offset = c.RsBSP;  
// 		sf.AddrBStore.Mode = AddrModeFlat;  
// 		sf.AddrStack.Offset = c.IntSp;  
// 		sf.AddrStack.Mode = AddrModeFlat;  
// #else  
// #error "Platform not supported!"  
// #endif

		StackFrame64.AddrPC.Mode = AddrModeFlat;
		StackFrame64.AddrPC.Offset = Context.Eip;
		StackFrame64.AddrStack.Mode = AddrModeFlat;
		StackFrame64.AddrStack.Offset = Context.Esp;
		StackFrame64.AddrFrame.Mode = AddrModeFlat;
		StackFrame64.AddrFrame.Offset = Context.Ebp;

		do 
		{
			if (!StackWalk64(
				IMAGE_FILE_MACHINE_I386,
				g_hProcess,
				hThread,
				&StackFrame64,
				&Context,
				ReadProcessMemoryProc64,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL
				))
			{
				if (0 != GetLastError())
					printf("StackWalk64 failed. (%d) \n", GetLastError());
				
				break;
			}

			if (pSymbol)
			{
				free(pSymbol);
				pSymbol = NULL;
			}

			pSymbol = (PSYMBOL_INFO)calloc(1, sizeof(SYMBOL_INFO) - sizeof(CHAR) + MAX_PATH);
			if (!pSymbol)
			{
				printf("calloc failed. (%d) \n", GetLastError());
				__leave;
			}

			pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			pSymbol->MaxNameLen = MAX_PATH;

			if (!SymFromAddr(
				g_hProcess,
				StackFrame64.AddrPC.Offset,
				&dw64Displacement,
				pSymbol
				))
			{
				printf("SymFromAddr failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("Name %s Address 0x%08p \n", pSymbol->Name, pSymbol->Address);

			if (!UnDecorateSymbolName(
				pSymbol->Name,
				chDecoratedName,
				_countof(chDecoratedName),
				0
				))
			{
				printf("UnDecorateSymbolName failed. (%d) \n", GetLastError());
				__leave;
			}
			else
				printf("SymbolName %s \n", chDecoratedName);

			Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			if (!SymGetLineFromAddr64(
				g_hProcess,
				/*Context.Eip,*/
				StackFrame64.AddrPC.Offset,
				&dwDisplacement,
				&Line
				))
			{
				if (487 == GetLastError())
					printf("487 \n");
				else
				{
					printf("SymGetLineFromAddr64 failed. (%d) \n", GetLastError());
					__leave;
				}
			}
			else
			{
				printf("FileName %s \n", Line.FileName);
				printf("LineNumber %d \n", Line.LineNumber);
			}
		} while (TRUE);

		// WalkFrameChaim();

		bRet = TRUE;
	}
	__finally
	{
		if (pSymbol)
		{
			free(pSymbol);
			pSymbol = NULL;
		}
	}

	printf("[%s] end \n", __FUNCTION__);

	return bRet;
}

BOOL
	CALLBACK
	ReadProcessMemoryProc64(
	_In_  HANDLE  hProcess,
	_In_  DWORD64 lpBaseAddress,
	_Out_ PVOID   lpBuffer,
	_In_  DWORD   nSize,
	_Out_ LPDWORD lpNumberOfBytesRead
	)
{
	BOOL bRet = FALSE;


	__try
	{
		bRet = ReadProcessMemory(
			hProcess,
			(LPCVOID)lpBaseAddress,
			lpBuffer,
			nSize,
			lpNumberOfBytesRead
			);
		if (!bRet)
		{
			printf("ReadProcessMemory failed. (%d) \n", GetLastError());
			__leave;
		}
	}
	__finally
	{
		;
	}

	return bRet;
}
