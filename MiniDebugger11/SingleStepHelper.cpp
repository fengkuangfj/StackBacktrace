#include <Windows.h>
#include <DbgHelp.h>
#include "DebugSession.h"
#include "SingleStepHelper.h"
#include "BreakPointHelper.h"



static BOOL GetCurrentLineInfo(LINE_INFO* pLineInfo);



//是否正在StepOver的指示值
static BOOL g_isBeingStepOver;

//是否正在StepOut的指示值 
static BOOL g_isBeingStepOut;

//保存行号信息的变量
static LINE_INFO g_lineInfo;

//是否正在逐条指令执行的指示值
static BOOL g_isBeingSingleInstruction;



//初始化单步执行组件
void InitializeSingleStepHelper() {

	g_lineInfo.fileName[0] = 0;
	g_lineInfo.lineNumber = 0;

	g_isBeingSingleInstruction = FALSE;
	g_isBeingStepOut = FALSE;
	g_isBeingStepOver = FALSE;
}



////开始单步执行
//void BeginSingleStep(BOOL stepOver) {
//
//	g_isBeingStepOver = stepOver;
//
//	g_isBeingSingleInstruction = FALSE;
//
//	GetCurrentLineInfo(&g_lineInfo);
//}
//
//
//
//
////结束单步执行
//void EndSingleStep() {
//
//	if (g_isBeingStepOver == TRUE) {
//		CancelStepOverBreakPoint();
//	}
//
//	g_isBeingStepOver = FALSE;
//}



//保存当前行的信息
void SaveCurrentLineInfo() {

	GetCurrentLineInfo(&g_lineInfo);
}



//获取行信息是否已改变的指示值
BOOL IsLineChanged() {

	LINE_INFO lineInfo;

	//如果当前指令没有对应的行信息，则认为仍在同一行上
	if (GetCurrentLineInfo(&lineInfo) == FALSE) {
		return FALSE;	
	}

	//如果有对应的行信息，则比较是否仍在同一行
	if (lineInfo.lineNumber == g_lineInfo.lineNumber && wcscmp(lineInfo.fileName, g_lineInfo.fileName) == 0) {
		return FALSE;
	}

	return TRUE;
}



//获取当前EIP所指地址对应的行号信息，返回TRUE。
//如果当前地址没有调试符号，或没有对应的行号信息，
//pLineInfo->fileName[0]和pLineInfo->lineNumber设为0，
//返回FALSE。
BOOL GetCurrentLineInfo(LINE_INFO* pLineInfo) {

	CONTEXT context;
	GetDebuggeeContext(&context);

	DWORD displacement;
	IMAGEHLP_LINE64 lineInfo = { 0 };
	lineInfo.SizeOfStruct = sizeof(lineInfo);

	if (SymGetLineFromAddr64(
		GetDebuggeeHandle(),
		context.Eip,
		&displacement,
		&lineInfo) == TRUE) {
			
		wcscpy_s(pLineInfo->fileName, lineInfo.FileName);
		pLineInfo->lineNumber = lineInfo.LineNumber;

		return TRUE;
	}
	else {

		pLineInfo->fileName[0] = 0;
		pLineInfo->lineNumber = 0;

		return FALSE;
	}
}



//如果指定地址处的指令是CALL，返回CALL指令的长度
//否则返回0
//判断的方法参考了《CALL指令有多少种写法》一文
// http://blog.ftofficer.com/2010/04/n-forms-of-call-instructions
int IsCallInstruction(DWORD address) {

	BYTE instruction[10];

	ReadDebuggeeMemory(
		address,
		sizeof(instruction) / sizeof(BYTE),
		instruction);

	switch (instruction[0]) {

		case 0xE8:
			return 5;

		case 0x9A:
			return 7;

		case 0xFF:
			switch (instruction[1]) {

				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x16:
				case 0x17:
				case 0xD0:
				case 0xD1:
				case 0xD2:
				case 0xD3:
				case 0xD4:
				case 0xD5:
				case 0xD6:
				case 0xD7:
					return 2;

				case 0x14:
				case 0x50:
				case 0x51:
				case 0x52:
				case 0x53:
				case 0x54:
				case 0x55:
				case 0x56:
				case 0x57:
					return 3;

				case 0x15:
				case 0x90:
				case 0x91:
				case 0x92:
				case 0x93:
				case 0x95:
				case 0x96:
				case 0x97:
					return 6;

				case 0x94:
					return 7;
			}

		default:
			return 0;
	}
}


//设置是否正在StepOver
void SetStepOver(BOOL value) {
	g_isBeingStepOver = value;
}


//获取是否正在StepOver的指示值
BOOL IsBeingStepOver() {
	return g_isBeingStepOver;
}



//设置是否正在逐条指令执行
void SetSingleInstruction(BOOL value) {

	g_isBeingSingleInstruction = value;
}



//获取是否正在逐条指令执行的指示值
//如果正在逐条指令执行的话，
//忽略所有调试器程序的断点和用户设置的断点
//以及设置TF位
BOOL IsBeingSingleInstruction() {

	return g_isBeingSingleInstruction;
}


//设置是否正在StepOut
void SetStepOut(BOOL value) {
	g_isBeingStepOut = value;
}

//获取是否正在StepOut的指示值
BOOL IsBeingStepOut() {
	return g_isBeingStepOut;
}


//获取当前函数内ret指令的地址
DWORD GetRetInstructionAddress() {

	CONTEXT context;
	GetDebuggeeContext(&context);

	DWORD64 displacement;
	SYMBOL_INFO symBol = { 0 };
	symBol.SizeOfStruct = sizeof(SYMBOL_INFO);

	if (SymFromAddr(
		GetDebuggeeHandle(),
		context.Eip, 
		&displacement,
		&symBol) == FALSE) {
			
		return 0;
	}

	DWORD endAddress = DWORD(symBol.Address + symBol.Size);
	int retLen;

	//检查是否三个字节的RET指令
	retLen = IsRetInstruction(endAddress - 3);

	if (retLen == 3) {
		return endAddress - retLen;
	}

	//检查是否一个字节的RET指令
	retLen = IsRetInstruction(endAddress - 1);

	if (retLen == 1) {
		return endAddress - retLen;
	}

	return 0;
}



//检查指定地址处的指令是否RET指令
//如果是，返回RET指令的长度
//否则返回0
int IsRetInstruction(DWORD address) {

	BYTE byte;
	ReadDebuggeeMemory(address, 1, &byte);

	if (byte == 0xC3 || byte == 0xCB) {
		return 1;
	}

	if (byte == 0xC2 || byte == 0xCA) {
		return 3;
	}

	return 0;
}