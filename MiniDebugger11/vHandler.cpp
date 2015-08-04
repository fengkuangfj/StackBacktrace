#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <list>
#include "TypeHelper.h"
#include "CmdHandlers.h"
#include "DebugSession.h"
#include "HelperFunctions.h"


//用来保存一些变量信息的结构体
struct VARIABLE_INFO {
	DWORD address;
	DWORD modBase;
	DWORD size;
	DWORD typeID;
	std::wstring name;
};



void EnumSymbols(LPCWSTR expression);
BOOL CALLBACK EnumVariablesCallBack(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext);
DWORD GetSymbolAddress(PSYMBOL_INFO pSymbol);
BOOL IsSimpleType(DWORD typeID, DWORD modBase);
void ShowVariables(const std::list<VARIABLE_INFO>& varInfoList);
void ShowVariableSummary(const VARIABLE_INFO* pVarInfo);
void ShowVariableValue(const VARIABLE_INFO* pVarInfo);



//显示局部变量命令的处理函数。
//命令格式为 lv [expression]
//参数expression表示仅显示符合指定通配符表达式的变量，如果省略则显示全部变量。
void OnShowLocalVariables(const Command& cmd) {

	CHECK_DEBUGGEE;

	//获取当前函数的开始地址
	CONTEXT context;
	GetDebuggeeContext(&context);
	
	//设置栈帧
	IMAGEHLP_STACK_FRAME stackFrame = { 0 };
	stackFrame.InstructionOffset = context.Eip;

	if (SymSetContext(GetDebuggeeHandle(), &stackFrame, NULL) == FALSE) {

		if (GetLastError() != ERROR_SUCCESS) {
			std::wcout << TEXT("No debug info in current address.") << std::endl;
			return;
		}
	}

	LPCWSTR expression = NULL;

	if (cmd.size() >= 2) {
		expression = cmd[1].c_str();
	}

	//枚举符号
	std::list<VARIABLE_INFO> varInfoList;

	if (SymEnumSymbols(
		GetDebuggeeHandle(),
		0,
		expression,
		EnumVariablesCallBack,
		&varInfoList) == FALSE) {

		std::wcout << TEXT("SymEnumSymbols failed: ") << GetLastError() << std::endl;
	}

	ShowVariables(varInfoList);
}



//显示全局变量命令的处理函数。
//命令格式为 gv [expression]
//参数expression表示仅显示符合指定通配符表达式的变量，如果省略则显示全部变量。
void OnShowGlobalVariables(const Command& cmd) {

	CHECK_DEBUGGEE;

	LPCWSTR expression = NULL;

	if (cmd.size() >= 2) {
		expression = cmd[1].c_str();
	}

	//获取当前EIP
	CONTEXT context;
	GetDebuggeeContext(&context);

	//获取EIP对应的模块的基地址
	DWORD modBase = (DWORD)SymGetModuleBase64(GetDebuggeeHandle(), context.Eip);

	if (modBase == 0) {
		std::wcout << TEXT("SymGetModuleBase64 failed: ") << GetLastError() << std::endl;
		return;
	}

	std::list<VARIABLE_INFO> varInfoList;

	if (SymEnumSymbols(
		GetDebuggeeHandle(),
		modBase,
		expression,
		EnumVariablesCallBack,
		&varInfoList) == FALSE) {

		std::wcout << TEXT("SymEnumSymbols failed: ") << GetLastError() << std::endl;
	}

	ShowVariables(varInfoList);
}





BOOL CALLBACK EnumVariablesCallBack(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {

	std::list<VARIABLE_INFO>* pVarInfoList = (std::list<VARIABLE_INFO>*)UserContext;

	VARIABLE_INFO varInfo;

	if (pSymInfo->Tag == SymTagData) {

		varInfo.address = GetSymbolAddress(pSymInfo);
		varInfo.modBase = (DWORD)pSymInfo->ModBase;
		varInfo.size = SymbolSize;
		varInfo.typeID = pSymInfo->TypeIndex;
		varInfo.name = pSymInfo->Name;

		pVarInfoList->push_back(varInfo);
	}

	return TRUE;
}



//显示变量列表
void ShowVariables(const std::list<VARIABLE_INFO>& varInfoList) {

	//如果只有一个变量，则显示它所有的信息
	if (varInfoList.size() == 1) {

		ShowVariableSummary(&*varInfoList.cbegin());
		
		std::wcout << TEXT("  ");

		if (IsSimpleType(varInfoList.cbegin()->typeID, varInfoList.cbegin()->modBase) == FALSE) {
			std::wcout << std::endl;
		}

		ShowVariableValue(&*varInfoList.cbegin());

		std::wcout << std::endl;

		return;
	}
		
	for (auto iterator = varInfoList.cbegin(); iterator != varInfoList.cend(); ++iterator) {

		ShowVariableSummary(&*iterator);

		if (IsSimpleType(iterator->typeID, iterator->modBase) == TRUE) {

			std::wcout << TEXT("  ");
			ShowVariableValue(&*iterator);
		}

		std::wcout << std::endl;
	}
}




//显示变量的概要信息
void ShowVariableSummary(const VARIABLE_INFO* pVarInfo) {

	std::wcout << GetTypeName(pVarInfo->typeID, pVarInfo->modBase) << TEXT("  ") 
			   << pVarInfo->name << TEXT("  ") << pVarInfo->size << TEXT("  ");

	PrintHex(pVarInfo->address, FALSE);
}



//显示变量的值
void ShowVariableValue(const VARIABLE_INFO* pVarInfo) {

	//读取符号的内存
	BYTE* pData = (BYTE*)malloc(sizeof(BYTE) * pVarInfo->size);
	ReadDebuggeeMemory(pVarInfo->address, pVarInfo->size, pData);

	std::wcout << GetTypeValue(
				     pVarInfo->typeID,
					 pVarInfo->modBase,
					 pVarInfo->address,
					 pData);
	
	free(pData);
}





//获取符号的虚拟地址
//如果符号是一个局部变量或者参数，
//pSymbol->Address是相对于EBP的偏移，
//将两者相加就是符号的虚拟地址
DWORD GetSymbolAddress(PSYMBOL_INFO pSymbolInfo) {

	if ((pSymbolInfo->Flags & SYMFLAG_REGREL) == 0) {
		return DWORD(pSymbolInfo->Address);
	}

	//如果当前EIP指向函数的第一条指令，则EBP的值仍然是属于
	//上一个函数的，所以此时不能使用EBP，而应该使用ESP-4作
	//为符号的基地址。

	CONTEXT context;
	GetDebuggeeContext(&context);

	//获取当前函数的开始地址
	DWORD64 displacement;
	SYMBOL_INFO symbolInfo = { 0 };
	symbolInfo.SizeOfStruct = sizeof(SYMBOL_INFO);

	SymFromAddr(
		GetDebuggeeHandle(),
		context.Eip,
		&displacement,
		&symbolInfo);

	//如果是函数的第一条指令，则不能使用EBP
	if (displacement == 0) {
		return DWORD(context.Esp - 4 + pSymbolInfo->Address);
	}

	return DWORD(context.Ebp + pSymbolInfo->Address);
}
