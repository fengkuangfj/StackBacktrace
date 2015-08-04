#include <Windows.h>
#include <DbgHelp.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include "TypeHelper.h"
#include "DebugSession.h"
#include "HelperFunctions.h"



CBaseTypeEnum GetCBaseType(int typeID, DWORD modBase);
std::wstring GetBaseTypeName(int typeID, DWORD modBase);
std::wstring GetPointerTypeName(int typeID, DWORD modBase);
std::wstring GetArrayTypeName(int typeID, DWORD modBase);
std::wstring GetEnumTypeName(int typeID, DWORD modBase);
std::wstring GetNameableTypeName(int typeID, DWORD modBase);
std::wstring GetUDTTypeName(int typeID, DWORD modbase);
std::wstring GetFunctionTypeName(int typeID, DWORD modBase);

char ConvertToSafeChar(char ch);
wchar_t ConvertToSafeWChar(wchar_t ch);
std::wstring GetBaseTypeValue(int typeID, DWORD modBase, const BYTE* pData);
std::wstring GetPointerTypeValue(int typeID, DWORD modBase, const BYTE* pData);
std::wstring GetEnumTypeValue(int typeID, DWORD modBase, const BYTE* pData);
std::wstring GetArrayTypeValue(int typeID, DWORD modBase, DWORD address, const BYTE* pData);
std::wstring GetUDTTypeValue(int typeID, DWORD modBase, DWORD address, const BYTE* pData);
BOOL GetDataMemberInfo(DWORD memberID, DWORD modBase, DWORD address, const BYTE* pData, std::wostringstream& valueBuilder);
BOOL VariantEqual(VARIANT var, CBaseTypeEnum cBaseType, const BYTE* data);




struct BaseTypeEntry {

	CBaseTypeEnum type;
	const LPCWSTR name;

} g_baseTypeNameMap[] = {

	{ cbtNone, TEXT("<no-type>") },
	{ cbtVoid, TEXT("void") },
	{ cbtBool, TEXT("bool") },
	{ cbtChar, TEXT("char") },
	{ cbtUChar, TEXT("unsigned char") },
	{ cbtWChar, TEXT("wchar_t") },
	{ cbtShort, TEXT("short") },
	{ cbtUShort, TEXT("unsigned short") },
	{ cbtInt, TEXT("int") },
	{ cbtUInt, TEXT("unsigned int") },
	{ cbtLong, TEXT("long") },
	{ cbtULong, TEXT("unsigned long") },
	{ cbtLongLong, TEXT("long long") },
	{ cbtULongLong, TEXT("unsigned long long") },
	{ cbtFloat, TEXT("float") },
	{ cbtDouble, TEXT("double") },
	{ cbtEnd, TEXT("") },
};





//判断是否简单的类型
BOOL IsSimpleType(DWORD typeID, DWORD modBase) {

	DWORD symTag;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_SYMTAG,
		&symTag);

	switch (symTag) {

		case SymTagBaseType:
		case SymTagPointerType:
		case SymTagEnum:
			return TRUE;

		default:
			return FALSE;
	}
}




//获取指定类型的名称
std::wstring GetTypeName(int typeID, DWORD modBase) {

	DWORD typeTag;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_SYMTAG,
		&typeTag);

	switch (typeTag) {
		
		case SymTagBaseType:
			return GetBaseTypeName(typeID, modBase);

		case SymTagPointerType:
			return GetPointerTypeName(typeID, modBase);

		case SymTagArrayType:
			return GetArrayTypeName(typeID, modBase);

		case SymTagUDT:
			return GetUDTTypeName(typeID, modBase);

		case SymTagEnum:
			return GetEnumTypeName(typeID, modBase);

		case SymTagFunctionType:
			return GetFunctionTypeName(typeID, modBase);

		default:
			return L"??";
	}
}




//获取基本类型的名称。
std::wstring GetBaseTypeName(int typeID, DWORD modBase) {

	CBaseTypeEnum baseType = GetCBaseType(typeID, modBase);

	int index = 0;

	while (g_baseTypeNameMap[index].type != cbtEnd) {

		if (g_baseTypeNameMap[index].type == baseType) {
			break;
		}

		++index;
	}

	return g_baseTypeNameMap[index].name;
}



//获取表示C/C++基本类型的枚举
//假定typeID已经是基本类型的ID.
CBaseTypeEnum GetCBaseType(int typeID, DWORD modBase) {

	//获取BaseTypeEnum
	DWORD baseType;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_BASETYPE,
		&baseType);

	//获取基本类型的长度
	ULONG64 length;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_LENGTH,
		&length);

	switch (baseType) {

		case btVoid:
			return cbtVoid;

		case btChar:
			return cbtChar;

		case btWChar:
			return cbtWChar;

		case btInt:
			switch (length) {
				case 2:  return cbtShort;
				case 4:  return cbtInt;
				default: return cbtLongLong;
			}

		case btUInt:
			switch (length) {
				case 1:  return cbtUChar;
				case 2:  return cbtUShort;
				case 4:  return cbtUInt;
				default: return cbtULongLong;
			}

		case btFloat:
			switch (length) {
				case 4:  return cbtFloat;
				default: return cbtDouble;
			}

		case btBool:
			return cbtBool;

		case btLong:
			return cbtLong;

		case btULong:
			return cbtULong;

		default:
			return cbtNone;
	}
}




//获取指针类型的名称
std::wstring GetPointerTypeName(int typeID, DWORD modBase) {

	//获取是指针类型还是引用类型
	BOOL isReference;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_IS_REFERENCE,
		&isReference);

	//获取指针所指对象的类型的typeID
	DWORD innerTypeID;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	return GetTypeName(innerTypeID, modBase) + (isReference == TRUE ? TEXT("&") : TEXT("*"));
}



//获取数组类型的名称
std::wstring GetArrayTypeName(int typeID, DWORD modBase) {

	//获取数组元素的TypeID
	DWORD innerTypeID;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	//获取数组元素个数
	DWORD elemCount;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_COUNT,
		&elemCount);

	std::wostringstream strBuilder;

	strBuilder << GetTypeName(innerTypeID, modBase) << TEXT('[') << elemCount << TEXT(']');

	return strBuilder.str();
}



//获取函数类型的名称
std::wstring GetFunctionTypeName(int typeID, DWORD modBase) {

	std::wostringstream nameBuilder;

	//获取参数数量
	DWORD paramCount;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&paramCount);

	//获取返回值的名称
	DWORD returnTypeID;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_TYPEID,
		&returnTypeID);

	nameBuilder << GetTypeName(returnTypeID, modBase);

	//获取每个参数的名称
	BYTE* pBuffer = (BYTE*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG) * paramCount);
	TI_FINDCHILDREN_PARAMS* pFindParams = (TI_FINDCHILDREN_PARAMS*)pBuffer;
	pFindParams->Count = paramCount;
	pFindParams->Start = 0;

	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	nameBuilder << TEXT('(');

	for (int index = 0; index != paramCount; ++index) {

		DWORD paramTypeID;
		SymGetTypeInfo(
			GetDebuggeeHandle(),
			modBase,
			pFindParams->ChildId[index],
			TI_GET_TYPEID,
			&paramTypeID);

		if (index != 0) {
			nameBuilder << TEXT(", ");
		}

		nameBuilder << GetTypeName(paramTypeID, modBase);
	}

	nameBuilder << TEXT(')');

	free(pBuffer);

	return nameBuilder.str();
}



//获取枚举类型的名称
std::wstring GetEnumTypeName(int typeID, DWORD modBase) {

	return GetNameableTypeName(typeID, modBase);
}



//获取UDT类型的名称
std::wstring GetUDTTypeName(int typeID, DWORD modBase) {

	return GetNameableTypeName(typeID, modBase);
}



//获取具有名称的类型的名称
std::wstring GetNameableTypeName(int typeID, DWORD modBase) {

	WCHAR* pBuffer;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_SYMNAME,
		&pBuffer);

	std::wstring typeName(pBuffer);

	LocalFree(pBuffer);

	return typeName;
}







//将指定地址处的内存，并以相应类型的形式表现出来
std::wstring GetTypeValue(int typeID, DWORD modBase, DWORD address, const BYTE* pData) {

	DWORD typeTag;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_SYMTAG,
		&typeTag);

	switch (typeTag) {
		
		case SymTagBaseType:
			return GetBaseTypeValue(typeID, modBase, pData);

		case SymTagPointerType:
			return GetPointerTypeValue(typeID, modBase, pData);

		case SymTagEnum:
			return GetEnumTypeValue(typeID, modBase, pData);

		case SymTagArrayType:
			return GetArrayTypeValue(typeID, modBase, address, pData);

		case SymTagUDT:
			return GetUDTTypeValue(typeID, modBase, address, pData);

		case SymTagTypedef:

			//获取真正类型的ID
			DWORD actTypeID;
			SymGetTypeInfo(
				GetDebuggeeHandle(),
				modBase,
				typeID,
				TI_GET_TYPEID,
				&actTypeID);

			return GetTypeValue(actTypeID, modBase, address, pData);

		default:
			return L"??";
	}
}



//获取基本类型变量的值
std::wstring GetBaseTypeValue(int typeID, DWORD modBase, const BYTE* pData) {

	CBaseTypeEnum cBaseType = GetCBaseType(typeID, modBase);

	return GetCBaseTypeValue(cBaseType, pData);
}



//获取C/C++基本类型的值
std::wstring GetCBaseTypeValue(CBaseTypeEnum cBaseType, const BYTE* pData) {

	std::wostringstream valueBuilder;

	switch (cBaseType) {

		case cbtNone:
			valueBuilder << TEXT("??");
			break;

		case cbtVoid:
			valueBuilder << TEXT("??");
			break;

		case cbtBool:
			valueBuilder << (*pData == 0 ? L"false" : L"true");
			break;

		case cbtChar:
			valueBuilder << ConvertToSafeChar(*((char*)pData));
			break;

		case cbtUChar:
			valueBuilder << std::hex 
						 << std::uppercase 
						 << std::setw(2) 
						 << std::setfill(TEXT('0')) 
						 << *((unsigned char*)pData);
			break;

		case cbtWChar:
			valueBuilder << ConvertToSafeWChar(*((wchar_t*)pData));
			break;

		case cbtShort:
			valueBuilder << *((short*)pData);
			break;

		case cbtUShort:
			valueBuilder << *((unsigned short*)pData);
			break;

		case cbtInt:
			valueBuilder << *((int*)pData);
			break;

		case cbtUInt:
			valueBuilder << *((unsigned int*)pData);
			break;

		case cbtLong:
			valueBuilder << *((long*)pData);
			break;

		case cbtULong:
			valueBuilder << *((unsigned long*)pData);
			break;

		case cbtLongLong:
			valueBuilder << *((long long*)pData);
			break;

		case cbtULongLong:
			valueBuilder << *((unsigned long long*)pData);
			break;

		case cbtFloat:
			valueBuilder << *((float*)pData);
			break;

		case cbtDouble:
			valueBuilder << *((double*)pData);
			break;
	}

	return valueBuilder.str();
}



//获取指针类型变量的值
std::wstring GetPointerTypeValue(int typeID, DWORD modBase, const BYTE* pData) {

	std::wostringstream valueBuilder;

	valueBuilder << std::hex << std::uppercase << std::setfill(TEXT('0')) << std::setw(8) << *((DWORD*)pData);

	return valueBuilder.str();
}



//获取枚举类型变量的值
std::wstring GetEnumTypeValue(int typeID, DWORD modBase, const BYTE* pData) {

	std::wstring valueName;

	//获取枚举值的基本类型
	CBaseTypeEnum cBaseType = GetCBaseType(typeID, modBase);

	//获取枚举值的个数
	DWORD childrenCount;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&childrenCount);

	//获取每个枚举值
	TI_FINDCHILDREN_PARAMS* pFindParams =
		(TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + childrenCount * sizeof(ULONG));
	pFindParams->Start = 0;
	pFindParams->Count = childrenCount;

	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	for (int index = 0; index != childrenCount; ++index) {

		//获取枚举值
		VARIANT enumValue;
		SymGetTypeInfo(
			GetDebuggeeHandle(),
			modBase,
			pFindParams->ChildId[index],
			TI_GET_VALUE,
			&enumValue);

		if (VariantEqual(enumValue, cBaseType, pData) == TRUE) {

			//获取枚举值的名称
			WCHAR* pBuffer;
			SymGetTypeInfo(
				GetDebuggeeHandle(),
				modBase,
				pFindParams->ChildId[index],
				TI_GET_SYMNAME,
				&pBuffer);

			valueName = pBuffer;
			LocalFree(pBuffer);

			break;
		}
	}

	free(pFindParams);

	//如果没有找到对应的枚举值，则显示它的基本类型值
	if (valueName.length() == 0) {

		valueName = GetBaseTypeValue(typeID, modBase, pData);
	}

	return valueName;
}



//将一个VARIANT类型的变量与一块内存区数据进行比较，判断是否相等。
//内存区数据的类型由cBaseType参数指定
BOOL VariantEqual(VARIANT var, CBaseTypeEnum cBaseType, const BYTE* pData) {

	switch (cBaseType) {

		case cbtChar:
			return var.cVal == *((char*)pData);

		case cbtUChar:
			return var.bVal == *((unsigned char*)pData);

		case cbtShort:
			return var.iVal == *((short*)pData);

		case cbtWChar:
		case cbtUShort:
			return var.uiVal == *((unsigned short*)pData);

		case cbtUInt:
			return var.uintVal == *((int*)pData);

		case cbtLong:
			return var.lVal == *((long*)pData);

		case cbtULong:
			return var.ulVal == *((unsigned long*)pData);

		case cbtLongLong:
			return var.llVal == *((long long*)pData);

		case cbtULongLong:
			return var.ullVal == *((unsigned long long*)pData);

		case cbtInt:
		default:
			return var.intVal == *((int*)pData);
	}
}



//获取数组类型变量的值
//只获取最多32个元素的值
std::wstring GetArrayTypeValue(int typeID, DWORD modBase, DWORD address, const BYTE* pData) {

	//获取元素个数，如果元素个数大于32,则设置为32
	DWORD elemCount;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_COUNT,
		&elemCount);

	elemCount = elemCount > 32 ? 32 : elemCount;

	//获取数组元素的TypeID
	DWORD innerTypeID;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_TYPEID,
		&innerTypeID);

	//获取数组元素的长度
	ULONG64 elemLen;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		innerTypeID,
		TI_GET_LENGTH,
		&elemLen);

	std::wostringstream valueBuilder;

	for (int index = 0; index != elemCount; ++index) {

		DWORD elemOffset = DWORD(index * elemLen);

		valueBuilder << TEXT("  [") << index << TEXT("]  ")
					 << GetTypeValue(innerTypeID, modBase, address + elemOffset, pData + index * elemLen);

		if (index != elemCount - 1) {
			valueBuilder << std::endl;
		}
	}

	return valueBuilder.str();
}



//获取用户定义类型的值
std::wstring GetUDTTypeValue(int typeID, DWORD modBase, DWORD address, const BYTE* pData) {

	std::wostringstream valueBuilder;

	//获取成员个数
	DWORD memberCount;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_GET_CHILDRENCOUNT,
		&memberCount);

	//获取成员信息
	TI_FINDCHILDREN_PARAMS* pFindParams =
		(TI_FINDCHILDREN_PARAMS*)malloc(sizeof(TI_FINDCHILDREN_PARAMS) + memberCount * sizeof(ULONG));
	pFindParams->Start = 0;
	pFindParams->Count = memberCount;

	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		typeID,
		TI_FINDCHILDREN,
		pFindParams);

	//遍历成员
	for (int index = 0; index != memberCount; ++index) {

		BOOL isDataMember = GetDataMemberInfo(
			pFindParams->ChildId[index],
			modBase,
			address,
			pData,
			valueBuilder);

		if (isDataMember == TRUE) {
			valueBuilder << std::endl;
		}
	}

	valueBuilder.seekp(-1, valueBuilder.end);
	valueBuilder.put(0);

	return valueBuilder.str();
}



//获取数据成员的信息
//如果成员是数据成员，返回TRUE
//否则返回FALSE
BOOL GetDataMemberInfo(DWORD memberID, DWORD modBase, DWORD address, const BYTE* pData, std::wostringstream& valueBuilder) {

	DWORD memberTag;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		memberID,
		TI_GET_SYMTAG,
		&memberTag);

	if (memberTag != SymTagData && memberTag != SymTagBaseClass) {
		return FALSE;
	}

	valueBuilder << TEXT("  ");

	DWORD memberTypeID;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		memberID,
		TI_GET_TYPEID,
		&memberTypeID);

	//输出类型
	valueBuilder << GetTypeName(memberTypeID, modBase);

	//输出名称
	if (memberTag == SymTagData) {

		WCHAR* name;
		SymGetTypeInfo(
			GetDebuggeeHandle(),
			modBase,
			memberID,
			TI_GET_SYMNAME,
			&name);

		valueBuilder << TEXT("  ") << name;

		LocalFree(name);
	}
	else {

		valueBuilder << TEXT("  <base-class>");
	}

	//输出长度
	ULONG64 length;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		memberTypeID,
		TI_GET_LENGTH,
		&length);

	valueBuilder << TEXT("  ") << length;

	//输出地址
	DWORD offset;
	SymGetTypeInfo(
		GetDebuggeeHandle(),
		modBase,
		memberID,
		TI_GET_OFFSET,
		&offset);

	DWORD childAddress = address + offset;

	valueBuilder << TEXT("  ") << std::hex << std::uppercase << std::setfill(TEXT('0')) << std::setw(8) << childAddress << std::dec;

	//输出值
	if (IsSimpleType(memberTypeID, modBase) == TRUE) {

		valueBuilder << TEXT("  ") 
						<< GetTypeValue(
							memberTypeID,
							modBase,
							childAddress, 
							pData + offset);
	}

	return TRUE;
}



//将char类型的字符转换成可以输出到控制台的字符,
//如果ch是不可显示的字符，则返回一个问号，
//否则直接返回ch。
//小于0x1E和大于0x7F的值都不能显示。
char ConvertToSafeChar(char ch) {

	if (ch < 0x1E || ch > 0x7F) {
		return '?';
	}

	return ch;
}



//将wchar_t类型的字符转换成可以输出到控制台的字符,
//如果当前的代码页不能显示ch，则返回一个问号，
//否则直接返回ch。
wchar_t ConvertToSafeWChar(wchar_t ch) {

	if (ch < 0x1E) {
		return L'?';
	}

	char buffer[4];

	size_t convertedCount;
	wcstombs_s(&convertedCount, buffer, 4, &ch, 2);

	if (convertedCount == 0) {
		return L'?';
	}
	
	return ch;
}