#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include "DebugSession.h"
#include "CmdHandlers.h"
#include "TypeHelper.h"
#include "HelperFunctions.h"



struct BaseTypeShortNameEntry {

	LPCWSTR shortName;
	CBaseTypeEnum cBaseType;
	int length;

} g_baseTypeShortNameMap[] = {

	{ TEXT("bool"), cbtBool, 1 },
	{ TEXT("char"), cbtChar, 1 },
	{ TEXT("uchar"), cbtUChar, 1 },
	{ TEXT("wchar"), cbtWChar, 2 },
	{ TEXT("short"), cbtShort, 2 },
	{ TEXT("ushort"), cbtUShort, 2 },
	{ TEXT("int"), cbtInt, 4 },
	{ TEXT("uint"), cbtUInt, 4 },
	{ TEXT("long"), cbtLong, 4 },
	{ TEXT("ulong"), cbtULong, 4 },
	{ TEXT("llong"), cbtLongLong, 8 },
	{ TEXT("ullong"), cbtULongLong, 8 },
	{ TEXT("float"), cbtFloat, 4 },
	{ TEXT("double"), cbtDouble, 8 },
	{ TEXT(""), cbtNone, 0 },
};




void FormatPointerType(DWORD address, int count);
void FormatCBaseType(DWORD address, CBaseTypeEnum cBaseType, int length, int count);
void FormatNameableType(DWORD address, LPCWSTR name, int count);



//格式化显示内存命令的处理函数。
//命令格式为 f address type
//address为内存地址，type为类型名称。
//当type为“*”时，表示指针。
//当type为以下的字符串时，表示相应的基本类型：
// bool       bool
// char       char
// uchar      unsigned char
// wchar      wchar_t
// short      short
// ushort     unsigned short
// int        int
// uint       unsigned int
// long       long
// ulong      unsigned long
// llong      long long
// ullong     unsigned long long
// float      float
// double     double
//当type是其它的字符串时，从调试符号中寻找
//名称与type相同的类型。
void OnFormatMemory(const Command& cmd) {

	CHECK_DEBUGGEE;

	if (cmd.size() < 3) {
		std::wcout << TEXT("Invalid params.") << std::endl;
		return;
	}

	DWORD address = wcstoul(cmd[1].c_str(), NULL, 16);

	DWORD count = 1;

	if (cmd.size() > 3) {
		count = _wtoi(cmd[3].c_str());
	}

	if (count == 0) {
		count = 1;
	}

	LPCWSTR typeName = cmd[2].c_str();

	//检查是否指针
	if (wcscmp(typeName, TEXT("*")) == 0) {

		FormatPointerType(address, count);
		return;
	}

	//检查是否基本类型
	for (int index = 0; wcscmp(g_baseTypeShortNameMap[index].shortName, TEXT("")) != 0; ++index) {

		if (wcscmp(g_baseTypeShortNameMap[index].shortName, typeName) == 0) {

			FormatCBaseType(
				address,
				g_baseTypeShortNameMap[index].cBaseType,
				g_baseTypeShortNameMap[index].length,
				count);

			return;
		}
	}

	FormatNameableType(address, typeName, count);
}



//显示指针类型
void FormatPointerType(DWORD address, int count) {

	BYTE data[4];

	for (int cur = 0; cur != count; ++cur) {

		if (ReadDebuggeeMemory(address, 4, data) == FALSE) {
			std::wcout << TEXT("Read memory failed.") << std::endl;
			return;
		}

		PrintHex(*((DWORD*)data), FALSE);

		address += 4;

		std::wcout << std::endl;
	}
}



//显示基本类型
void FormatCBaseType(DWORD address, CBaseTypeEnum cBaseType, int length, int count) {

	BYTE* pData = (BYTE*)malloc(sizeof(TCHAR) * length);

	for (int cur = 0; cur != count; ++cur) {

		if (ReadDebuggeeMemory(address, length, pData) == FALSE) {
			std::wcout << TEXT("Read memory failed.") << std::endl;
			return;
		}

		std::wcout << GetCBaseTypeValue(cBaseType, pData) << std::endl;

		address += length;
	}

	free(pData);
}



//显示有名称的类型
void FormatNameableType(DWORD address, LPCWSTR name, int count) {

	SYMBOL_INFO typeInfo;
	typeInfo.SizeOfStruct = sizeof(SYMBOL_INFO);
	typeInfo.MaxNameLen = 0;

	CONTEXT context;
	GetDebuggeeContext(&context);

	if (SymGetTypeFromName(
		GetDebuggeeHandle(),
		SymGetModuleBase64(GetDebuggeeHandle(), context.Eip),
		name,
		&typeInfo) == FALSE) {

		std::wcout << TEXT("No such type.") << std::endl;
		return;
	}

	//读取内存
	BYTE* pData = (BYTE*)malloc(sizeof(BYTE) * typeInfo.Size);

	for (int cur = 0; cur != count; ++cur) {

		if (ReadDebuggeeMemory(address, typeInfo.Size, pData) == FALSE) {

			std::wcout << TEXT("Read memory failed.") << std::endl;
			free(pData);
			return;
		}

		std::wcout << GetTypeValue(
						  typeInfo.TypeIndex,
						  (DWORD)typeInfo.ModBase,
						  address,
						  pData)
				   << std::endl;

		if (IsSimpleType(typeInfo.TypeIndex, (DWORD)typeInfo.ModBase) == FALSE && cur != count - 1) {
			std::wcout << std::endl;
		}

		address += typeInfo.Size;
	}

	free(pData);
}