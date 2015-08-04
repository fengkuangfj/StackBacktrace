#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <DbgHelp.h>
#include "DebugSession.h"
#include "HelperFunctions.h"
#include "BreakPointHelper.h"
#include "SingleStepHelper.h"



DWORD GetEntryPointAddress();
void SetBreakPointAtEntryPoint();
BOOL DispatchDebugEvent(const DEBUG_EVENT* pDebugEvent);

BOOL OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO*);
BOOL OnThreadCreated(const CREATE_THREAD_DEBUG_INFO*);
BOOL OnException(const EXCEPTION_DEBUG_INFO*);
BOOL OnProcessExited(const EXIT_PROCESS_DEBUG_INFO*);
BOOL OnThreadExited(const EXIT_THREAD_DEBUG_INFO*);
BOOL OnOutputDebugString(const OUTPUT_DEBUG_STRING_INFO*);
BOOL OnRipEvent(const RIP_INFO*);
BOOL OnDllLoaded(const LOAD_DLL_DEBUG_INFO*);
BOOL OnDllUnloaded(const UNLOAD_DLL_DEBUG_INFO*);

BOOL OnNormalBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo);
BOOL OnUserBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo);
BOOL OnBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo);
BOOL OnSingleStep(const EXCEPTION_DEBUG_INFO* pInfo);
BOOL SingleStepHandler();
void BackDebuggeeEip();
BOOL OnStepOutBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo);

static HANDLE g_hProcess = NULL;
static HANDLE g_hThread = NULL;
static DWORD g_processID = 0;
static DWORD g_threadID = 0;

static int g_debuggeeStatus = STATUS_NONE;
static DWORD g_continueStatus = DBG_EXCEPTION_NOT_HANDLED;

//如果该值为TRUE，则无论g_continueStatus的值是什么，都会以DBG_CONTINUE
//调用ContineDebugEvent。该值用于断点功能。
static BOOL g_alwaysContinue = FALSE;



//获取被调试进程的状态。
int GetDebuggeeStatus() {
	return g_debuggeeStatus;
}

//获取被调试进程的句柄
HANDLE GetDebuggeeHandle() {
	return g_hProcess;
}


//获取被调试进程主线程句柄
HANDLE GetDebuggeeThread() {
	return g_hThread;
}


//读取被调试进程的内存。
BOOL ReadDebuggeeMemory(unsigned int address, unsigned int length, void* pData) {

	SIZE_T bytesRead;

	return ReadProcessMemory(g_hProcess, (LPCVOID)address, pData, length, &bytesRead);
}


//将指定的内容写进被调试进程的内存
BOOL WriteDebuggeeMemory(unsigned int address, unsigned int length, const void* pData) {

	SIZE_T byteWriten;

	return WriteProcessMemory(g_hProcess, (LPVOID)address, pData, length, &byteWriten);
}


//启动被调试进程。
void StartDebugSession(LPCTSTR path) {

	if (g_debuggeeStatus != STATUS_NONE) {
		std::wcout << TEXT("Debuggee is running.") << std::endl;
		return;
	}

	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = { 0 };

	if (CreateProcess(
		path,
		NULL,
		NULL,
		NULL,
		FALSE,
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE | CREATE_SUSPENDED,
		NULL,
		NULL,
		&si,
		&pi) == FALSE) {

		std::wcout << TEXT("CreateProcess failed: ") << GetLastError() << std::endl;
		return;
	}

	g_hProcess = pi.hProcess;
	g_hThread = pi.hThread;
	g_processID = pi.dwProcessId;
	g_threadID = pi.dwThreadId;

	g_debuggeeStatus = STATUS_SUSPENDED;

	std::wcout << TEXT("Debugee has started and was suspended.") << std::endl;
}



//指示是否处理了异常。如果handled为TRUE，表示处理了异常，
//否则为没有处理异常。
void HandledException(BOOL handled) {

	g_continueStatus = (handled == TRUE) ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
}



//继续被调试进程的执行。根据被调试进程的状态会执行不同的操作。
//如果DispatchDebugEvent返回TRUE，则继续被调试进程执行；
//如果返回FALSE，则暂停被调试进程，交给用户来处理。
void ContinueDebugSession() {

	if (g_debuggeeStatus == STATUS_NONE) {

		std::wcout << TEXT("Debuggee is not started yet.") << std::endl;
		return;
	}

	if (g_debuggeeStatus == STATUS_SUSPENDED) {

		ResumeThread(g_hThread);
	}
	else {

		ContinueDebugEvent(
			g_processID,
			g_threadID,
			g_alwaysContinue == TRUE ? DBG_CONTINUE : g_continueStatus);

		g_alwaysContinue = FALSE;
	}

	DEBUG_EVENT debugEvent;

	while (WaitForDebugEvent(&debugEvent, INFINITE) == TRUE) {

		if (DispatchDebugEvent(&debugEvent) == TRUE) {

			ContinueDebugEvent(g_processID, g_threadID, g_continueStatus);
		}
		else {

			break;
		}
	}
}



//根据调试事件的类型调用不同的处理函数。
BOOL DispatchDebugEvent(const DEBUG_EVENT* pDebugEvent) {

	switch (pDebugEvent->dwDebugEventCode) {

		case CREATE_PROCESS_DEBUG_EVENT:
			return OnProcessCreated(&pDebugEvent->u.CreateProcessInfo);
 
		case CREATE_THREAD_DEBUG_EVENT:
			return OnThreadCreated(&pDebugEvent->u.CreateThread);
 
		case EXCEPTION_DEBUG_EVENT:
			return OnException(&pDebugEvent->u.Exception);
 
		case EXIT_PROCESS_DEBUG_EVENT:
			return OnProcessExited(&pDebugEvent->u.ExitProcess);
 
		case EXIT_THREAD_DEBUG_EVENT:
			return OnThreadExited(&pDebugEvent->u.ExitThread);
 
		case LOAD_DLL_DEBUG_EVENT:
			return OnDllLoaded(&pDebugEvent->u.LoadDll);
 
		case OUTPUT_DEBUG_STRING_EVENT:
			return OnOutputDebugString(&pDebugEvent->u.DebugString);
 
		case RIP_EVENT:
			return OnRipEvent(&pDebugEvent->u.RipInfo);
 
		case UNLOAD_DLL_DEBUG_EVENT:
			return OnDllUnloaded(&pDebugEvent->u.UnloadDll);

		default:
			std::wcout << TEXT("Unknown debug event.") << std::endl;
			return FALSE;
	}
}



BOOL OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO* pInfo) {

	InitializeBreakPointHelper();
	InitializeSingleStepHelper();

	//初始化符号处理器
	//注意，这里不能使用pInfo->hProcess，因为g_hProcess和pInfo->hProcess
	//的值并不相同，而其它DbgHelp函数使用的是g_hProcess。
	if (SymInitialize(g_hProcess, NULL, FALSE) == TRUE) {
	
		//加载模块的调试信息
		DWORD64 moduleAddress = SymLoadModule64(
			g_hProcess,
			pInfo->hFile, 
			NULL,
			NULL,
			(DWORD64)pInfo->lpBaseOfImage,
			0);

		if (moduleAddress != 0) {
					
			SetBreakPointAtEntryPoint();
		}
		else {
			std::wcout << TEXT("SymLoadModule64 failed: ") << GetLastError() << std::endl;
		}
	}
	else {

		std::wcout << TEXT("SymInitialize failed: ") << GetLastError() << std::endl;
	}

	CloseHandle(pInfo->hFile);
	CloseHandle(pInfo->hThread);
	CloseHandle(pInfo->hProcess);

	return TRUE;
}



BOOL OnThreadCreated(const CREATE_THREAD_DEBUG_INFO* pInfo) {

	CloseHandle(pInfo->hThread);

	return TRUE;
}



//发生异常的时候应该通知用户，交由用户来处理，所以应返回FALSE。
BOOL OnException(const EXCEPTION_DEBUG_INFO* pInfo) {

	switch (pInfo->ExceptionRecord.ExceptionCode) {

		case EXCEPTION_BREAKPOINT:
			return OnBreakPoint(pInfo);

		case EXCEPTION_SINGLE_STEP:
			return OnSingleStep(pInfo);
	}

	std::wcout << TEXT("An exception was occured at ");
	PrintHex((unsigned int)pInfo->ExceptionRecord.ExceptionAddress, FALSE);
	std::wcout << TEXT(".") << std::endl << TEXT("Exception code: ");
	PrintHex(pInfo->ExceptionRecord.ExceptionCode, TRUE);
	std::wcout << std::endl;

	if (pInfo->dwFirstChance == TRUE) {

		std::wcout << TEXT("First chance.") << std::endl;
	}
	else {

		std::wcout << TEXT("Second chance.") << std::endl;
	}

	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



//处理断点异常
BOOL OnBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo) {

	//获取断点类型
	int bpType = GetBreakPointType(DWORD(pInfo->ExceptionRecord.ExceptionAddress));

	switch (bpType) {

		//如果是初始断点，继续执行
		case BP_INIT:
			HandledException(TRUE);
			return TRUE;

		case BP_CODE:
			return OnNormalBreakPoint(pInfo);

		case BP_USER:
			return OnUserBreakPoint(pInfo);

		case BP_STEP_OVER:
			CancelStepOverBreakPoint();
			BackDebuggeeEip();
			return SingleStepHandler();

		case BP_STEP_OUT:
			return OnStepOutBreakPoint(pInfo);
	}

	return TRUE;
}



BOOL OnNormalBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo) {

	if (IsBeingSingleInstruction() == TRUE) {
		HandledException(TRUE);
		return TRUE;
	}

	//如果正在StepOver，则停止StepOver
	if (IsBeingStepOver() == TRUE) {
		CancelStepOverBreakPoint();
		SetStepOver(FALSE);
	}

	//如果正在StepOut，则停止StepOut
	if (IsBeingStepOut() == TRUE) {
		CancelStepOutBreakPoint();
		SetStepOut(FALSE);
	}

	std::wcout << TEXT("A break point occured at ");
	PrintHex((DWORD)pInfo->ExceptionRecord.ExceptionAddress, FALSE);
	std::wcout << TEXT(".") << std::endl;

	g_alwaysContinue = TRUE;

	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



//用户断点的处理函数
BOOL OnUserBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo) {
		
	//恢复断点所在指令的第一个字节
	RecoverUserBreakPoint((DWORD)pInfo->ExceptionRecord.ExceptionAddress);

	//将EIP回退一个字节
	BackDebuggeeEip();

	//设置TF标志
	SetTrapFlag();

	//保存断点地址，在单步执行时恢复
	SaveResetUserBreakPoint((DWORD)pInfo->ExceptionRecord.ExceptionAddress);

	return OnNormalBreakPoint(pInfo);
}



//StepOut断点的处理函数
BOOL OnStepOutBreakPoint(const EXCEPTION_DEBUG_INFO* pInfo) {

	//恢复断点所在指令的第一个字节
	CancelStepOutBreakPoint();

	//将EIP回退一个字节
	BackDebuggeeEip();

	//检查当前指令是否RET指令
	CONTEXT context;
	GetDebuggeeContext(&context);

	if (IsRetInstruction(context.Eip) != 0) {

		//读取函数返回地址
		DWORD retAddress;
		ReadDebuggeeMemory(context.Esp, sizeof(DWORD), &retAddress);

		//在返回地址处设置断点
		SetStepOutBreakPointAt(retAddress);

		HandledException(TRUE);
		return TRUE;
	}

	SetStepOut(FALSE);

	g_alwaysContinue = TRUE;
	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



//单步执行异常的处理函数。
BOOL OnSingleStep(const EXCEPTION_DEBUG_INFO* pInfo) {

	if (NeedResetBreakPoint() == TRUE) {
	
		//恢复用户断点
		ResetUserBreakPoint();
	}

	if (IsBeingSingleInstruction() == TRUE) {
		return SingleStepHandler();
	}

	HandledException(TRUE);
	return TRUE;
}



//单步执行的处理函数
BOOL SingleStepHandler() {

	//如果当前指令所在行没有改变，则继续单步执行
	if (IsLineChanged() == FALSE) {

		//如果正在StepOver
		if (IsBeingStepOver() == TRUE) {

			//检查是否CALL指令
			CONTEXT context;
			GetDebuggeeContext(&context);
			int callLen = IsCallInstruction(context.Eip);

			//如果是,则在下一条指令处设置断点
			if (callLen != 0) {
				SetStepOverBreakPointAt(context.Eip + callLen);
				SetSingleInstruction(FALSE);	
			}
			else {
				SetTrapFlag();
				SetSingleInstruction(TRUE);
			}
		}
		else {
			SetTrapFlag();
			SetSingleInstruction(TRUE);
		}

		HandledException(TRUE);
		return TRUE;
	}

	//如果行已经改变，则停止单步执行
	if (IsBeingStepOver() == TRUE) {
		SetStepOver(FALSE);
	}

	g_alwaysContinue = TRUE;
	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



//被调试进程已退出，不应再继续执行，所以返回FALSE。
BOOL OnProcessExited(const EXIT_PROCESS_DEBUG_INFO* pInfo) {

	std::wcout << TEXT("Debuggee was terminated.") << std::endl
			   << TEXT("Exit code: ") << pInfo->dwExitCode << std::endl;

	//清理符号信息
	SymCleanup(g_hProcess);

	//由于在处理这个事件的时候Debuggee还未真正结束，
	//所以要调用一次ContinueDebugEvent，使它顺利结束。
	ContinueDebugEvent(g_processID, g_threadID, DBG_CONTINUE);

	CloseHandle(g_hThread);
	CloseHandle(g_hProcess);

	g_hProcess = NULL;
	g_hThread = NULL;
	g_processID = 0;
	g_threadID = 0;
	g_debuggeeStatus = STATUS_NONE;
	g_continueStatus = DBG_EXCEPTION_NOT_HANDLED;
	g_alwaysContinue = FALSE;

	return FALSE;
}



BOOL OnThreadExited(const EXIT_THREAD_DEBUG_INFO* pInfo) {

	return TRUE;
}



//被调试进程输出调试信息，应通知用户并由用户来处理，返回FALSE。
BOOL OnOutputDebugString(const OUTPUT_DEBUG_STRING_INFO* pInfo) {

	BYTE* pBuffer = (BYTE*)malloc(pInfo->nDebugStringLength);

	SIZE_T bytesRead;

	ReadProcessMemory(
		g_hProcess,
		pInfo->lpDebugStringData,
		pBuffer, 
		pInfo->nDebugStringLength,
		&bytesRead);

	int requireLen = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED,
		(LPCSTR)pBuffer,
		pInfo->nDebugStringLength,
		NULL,
		0);

	TCHAR* pWideStr = (TCHAR*)malloc(requireLen * sizeof(TCHAR));

	MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED,
		(LPCSTR)pBuffer,
		pInfo->nDebugStringLength,
		pWideStr,
		requireLen);

	std::wcout << TEXT("Debuggee debug string: ") << pWideStr <<  std::endl;

	free(pWideStr);
	free(pBuffer);

	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



BOOL OnRipEvent(const RIP_INFO* pInfo) {

	std::wcout << TEXT("A RIP_EVENT occured.") << std::endl;

	g_debuggeeStatus = STATUS_INTERRUPTED;
	return FALSE;
}



BOOL OnDllLoaded(const LOAD_DLL_DEBUG_INFO* pInfo) {

	//加载模块的调试信息
	DWORD64 moduleAddress = SymLoadModule64(
		g_hProcess,
		pInfo->hFile, 
		NULL,
		NULL,
		(DWORD64)pInfo->lpBaseOfDll,
		0);

	if (moduleAddress == 0) {

		std::wcout << TEXT("SymLoadModule64 failed: ") << GetLastError() << std::endl;
	}

	CloseHandle(pInfo->hFile);

	return TRUE;
}



BOOL OnDllUnloaded(const UNLOAD_DLL_DEBUG_INFO* pInfo) {

	SymUnloadModule64(g_hProcess, (DWORD64)pInfo->lpBaseOfDll);

	return TRUE;
}



//获取被调试进程的主线程的上下文环境。
BOOL GetDebuggeeContext(CONTEXT* pContext) {

	pContext->ContextFlags = CONTEXT_FULL;

	if (GetThreadContext(g_hThread, pContext) == FALSE) {

		std::wcout << TEXT("GetThreadContext failed: ") << GetLastError() << std::endl;
		return FALSE;
	}

	return TRUE;
}


//设置被调试进程的主线程的上下文环境
BOOL SetDebuggeeContext(CONTEXT* pContext) {

	if (SetThreadContext(g_hThread, pContext) == FALSE) {

		std::wcout << TEXT("SetThreadContext failed: ") << GetLastError() << std::endl;
	}

	return TRUE;
}



//停止被调试进程的执行。这里要调用一次ContinueDebugSession，
//让调试器捕获到EXIT_PROCESS_DEBUG_EVENT事件。
void StopDebugSeesion() {

	if (TerminateProcess(g_hProcess, -1) == TRUE) {

		ContinueDebugSession();
	}
	else {

		std::wcout << TEXT("TerminateProcess failed: ") << GetLastError() << std::endl;
	}
}



//在入口点函数设置断点
void SetBreakPointAtEntryPoint() {

	//在Main函数设置断点
	DWORD mainAddress = GetEntryPointAddress();

	if (mainAddress != 0) {

		if (SetUserBreakPointAt(mainAddress) == FALSE) {
			std::wcout << TEXT("Set break point at entry function failed.") << std::endl;	
		}
	}
	else {

		std::wcout << TEXT("Cannot find entry function.") << std::endl;
	}
}


//获取入口点函数的地址
DWORD GetEntryPointAddress() {

	static LPCTSTR entryPointNames[] = {
		TEXT("main"),
		TEXT("wmain"),
		TEXT("WinMain"),
		TEXT("wWinMain"),
	};

	SYMBOL_INFO symbolInfo = { 0 };
	symbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);

	for (int index = 0; index != sizeof(entryPointNames) / sizeof(LPCTSTR); ++index) {

		if (SymFromName(g_hProcess, entryPointNames[index], &symbolInfo) == TRUE) {
			return (DWORD)symbolInfo.Address;
		}
	}

	return 0;
}



//将被调试进程的EIP回退一
void BackDebuggeeEip() {

	CONTEXT context;
	GetDebuggeeContext(&context);
	context.Eip -= 1;
	SetDebuggeeContext(&context);
}