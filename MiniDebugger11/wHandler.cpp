#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <map>
#include <string>
#include "DebugSession.h"
#include "CmdHandlers.h"
#include "HelperFunctions.h"


typedef std::map<DWORD, std::wstring> ModuleBaseToNameMap;


BOOL CALLBACK EnumerateModuleCallBack(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext);



void OnShowStackTrack(const Command& cmd) {

	CHECK_DEBUGGEE;

	//枚举模块，建立模块的基址-名称表
	ModuleBaseToNameMap moduleMap;

	if (EnumerateLoadedModules64(
		GetDebuggeeHandle(),
		EnumerateModuleCallBack,
		&moduleMap) == FALSE) {
	
			std::wcout << TEXT("EnumerateLoadedModules64 failed: ") << GetLastError() << std::endl;
			return;
	}

	CONTEXT context;
	GetDebuggeeContext(&context);

	STACKFRAME64 stackFrame = { 0 };
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrPC.Offset = context.Eip;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Esp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Ebp;

	while (true) {

		//获取栈帧
		if (StackWalk64(
			IMAGE_FILE_MACHINE_I386,
			GetDebuggeeHandle(),
			GetDebuggeeThread(),
			&stackFrame,
			&context,
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL) == FALSE) {

				break;
		}

		PrintHex((DWORD)stackFrame.AddrPC.Offset, FALSE);
		std::wcout << TEXT("  ");

		//显示模块名称
		DWORD moduleBase = (DWORD)SymGetModuleBase64(GetDebuggeeHandle(), stackFrame.AddrPC.Offset);

		const std::wstring& moduleName = moduleMap[moduleBase];

		if (moduleName.length() != 0) {
			std::wcout << moduleName;
		}
		else {
			std::wcout << TEXT("??");
		}

		std::wcout << TEXT('!');

		//显示函数名称
		BYTE buffer[sizeof(SYMBOL_INFO) + 128 * sizeof(TCHAR)] = { 0 };
		PSYMBOL_INFO pSymInfo = (PSYMBOL_INFO)buffer;
		pSymInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymInfo->MaxNameLen = 128;
		
		DWORD64 displacement;

		if (SymFromAddr(
			GetDebuggeeHandle(),
			stackFrame.AddrPC.Offset,
			&displacement,
			pSymInfo) == TRUE) {

				std::wcout << pSymInfo->Name << std::endl;
		}
		else {

			std::wcout << TEXT("??") << std::endl;
		}
	}
}




BOOL CALLBACK EnumerateModuleCallBack(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext) {

	ModuleBaseToNameMap* pModuleMap = (ModuleBaseToNameMap*)UserContext;

	LPCWSTR name = wcsrchr(ModuleName, TEXT('\\')) + 1;

	(*pModuleMap)[(DWORD)ModuleBase] = name;

	return TRUE;
}