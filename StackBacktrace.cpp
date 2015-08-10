#include "StackBacktrace.h"

RTLWALKFRAMECHAIN	RtlWalkFrameChain	= NULL;
HANDLE				g_hProcess			= NULL;

BOOL
	CStackBacktrace::WalkFrameChaim()
{
	BOOL			bRet						= FALSE;

	PVOID			ReturnAddress[MAX_PATH]		= {0};
	ULONG			FrameCount					= 0;
	ULONG			FrameNumber					= 0;
	HMODULE			hModule						= NULL;


	printf("[%s] 0x%08p \n", __FUNCTION__, &CStackBacktrace::WalkFrameChaim);
	printf("[%s] begin \n", __FUNCTION__);

	__try
	{
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
			printf("Stack[%d] 0x%08p \n", FrameNumber, ReturnAddress[FrameNumber]);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	printf("[%s] end \n", __FUNCTION__);

	return bRet;
}

BOOL
	CStackBacktrace::Init()
{
	BOOL	bRet		= FALSE;

	DWORD	dwOptions	= 0;


	__try
	{
		g_hProcess = GetCurrentProcess();

		if (!SymInitialize(g_hProcess, "D:\\StackBacktrace\\Application\\Debug", TRUE))
		{
			printf("SymInitialize failed. (%d) \n", GetLastError());
			__leave;
		}

		dwOptions = SymGetOptions();
		dwOptions |= SYMOPT_LOAD_LINES;
		SymSetOptions(dwOptions);

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
	CStackBacktrace::Unload()
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
	CStackBacktrace::StackBacktrace()
{
	BOOL					bRet						= FALSE;

	STACKFRAME64			StackFrame64				= {0};

	DWORD64					dw64Displacement			= 0;
	PSYMBOL_INFO			pSymbol						= NULL;
	DWORD					dwDisplacement				= 0;
	IMAGEHLP_LINE64			Line						= {0};
	CHAR					chDecoratedName[MAX_PATH]	= {0};
	LPEXCEPTION_POINTERS	lpExceptionPointers			= NULL;
	HANDLE					hThread						= NULL;
	CONTEXT					Context						= {0};
	CHAR					chHomeDir[MAX_PATH]			= {0};


	printf("[%s] 0x%08p \n", __FUNCTION__, &CStackBacktrace::StackBacktrace);
	printf("[%s] begin \n", __FUNCTION__);
	
	__try
	{
		hThread = GetCurrentThread();

		RtlCaptureContext(&Context);

		Context.ContextFlags = CONTEXT_ALL;

		StackFrame64.AddrPC.Mode = AddrModeFlat;
		StackFrame64.AddrPC.Offset = Context.Eip;
		StackFrame64.AddrStack.Mode = AddrModeFlat;
		StackFrame64.AddrStack.Offset = Context.Esp;
		StackFrame64.AddrFrame.Mode = AddrModeFlat;
		StackFrame64.AddrFrame.Offset = Context.Ebp;

		pSymbol = (PSYMBOL_INFO)calloc(1, sizeof(SYMBOL_INFO) - sizeof(CHAR) + MAX_PATH);
		if (!pSymbol)
		{
			printf("calloc failed. (%d) \n", GetLastError());
			__leave;
		}

		do 
		{
			ZeroMemory(pSymbol, sizeof(SYMBOL_INFO) - sizeof(CHAR) + MAX_PATH);

			pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			pSymbol->MaxNameLen = MAX_PATH;

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
				if (0 == GetLastError())
					break;

				printf("StackWalk64 failed. (%d) \n", GetLastError());
				__leave;
			}

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
				printf("[%s] 0x%08p \n", pSymbol->Name, pSymbol->Address);

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
				printf("[%s] \n", chDecoratedName);

			ZeroMemory(&Line, sizeof(Line));

			Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

			if (!SymGetLineFromAddr64(
				g_hProcess,
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
				printf("[%s] (%d) \n", Line.FileName, Line.LineNumber - 1);
		} while (TRUE);

// 		ZeroMemory(chHomeDir, sizeof(chHomeDir));
// 		if (!SymGetHomeDirectory(hdBase, chHomeDir, _countof(chHomeDir)))
// 		{
// 			printf("SymGetHomeDirectory failed. (%d) \n", GetLastError());
// 			__leave;
// 		}
// 		else
// 			printf("%s \n", chHomeDir);
// 
// 		ZeroMemory(chHomeDir, sizeof(chHomeDir));
// 		if (!SymGetHomeDirectory(hdSrc, chHomeDir, _countof(chHomeDir)))
// 		{
// 			printf("SymGetHomeDirectory failed. (%d) \n", GetLastError());
// 			__leave;
// 		}
// 		else
// 			printf("%s \n", chHomeDir);
// 
// 		ZeroMemory(chHomeDir, sizeof(chHomeDir));
// 		if (!SymGetHomeDirectory(hdSym, chHomeDir, _countof(chHomeDir)))
// 		{
// 			printf("SymGetHomeDirectory failed. (%d) \n", GetLastError());
// 			__leave;
// 		}
// 		else
// 			printf("%s \n", chHomeDir);

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
