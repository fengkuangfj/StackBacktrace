#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <Windows.h>
#include <DbgHelp.h>
#include "CmdHandlers.h"
#include "DebugSession.h"
#include "HelperFunctions.h"



void DisplaySourceLines(LPCTSTR sourceFile, int lineNum, unsigned int address, int after, int before);
void DisplayLine(LPCTSTR sourceFile, const std::wstring& line, int lineNumber, BOOL isCurLine);



//显示源代码命令的处理函数。
//命令格式为 l [after] [before]
//after表示显示当前代码行之后的行数，不能小于0
//before表示显示当前代码行之前的行数，不能小于0
//如果省略了after或before，它们的取值将是10
void OnShowSourceLines(const Command& cmd) {

	CHECK_DEBUGGEE;

	int afterLines = 10;
	int beforeLines = 10;

	if (cmd.size() > 1) {

		afterLines = _wtoi(cmd[1].c_str());

		if (afterLines < 0) {

			std::wcout << TEXT("Invalid params.") << std::endl;
			return;
		}
	}

	if (cmd.size() > 2) {

		beforeLines = _wtoi(cmd[2].c_str());

		if (beforeLines < 0) {

			std::wcout << TEXT("Invalid params.") << std::endl;
			return;
		}
	}

	//获取EIP
	CONTEXT context;
	GetDebuggeeContext(&context);

	//获取源文件以及行信息
	IMAGEHLP_LINE64 lineInfo = { 0 };
	lineInfo.SizeOfStruct = sizeof(lineInfo);
	DWORD displacement = 0;

	if (SymGetLineFromAddr64(
		GetDebuggeeHandle(),
		context.Eip,
		&displacement,
		&lineInfo) == FALSE) {

		DWORD errorCode = GetLastError();
		
		switch (errorCode) {

			// 126 表示还没有通过SymLoadModule64加载模块信息
			case 126:
				std::wcout << TEXT("Debug info in current module has not loaded.") << std::endl;
				return;

			// 487 表示模块没有调试符号
			case 487:
				std::wcout << TEXT("No debug info in current module.") << std::endl;
				return;

			default:
				std::wcout << TEXT("SymGetLineFromAddr64 failed: ") << errorCode << std::endl;
				return;
		}
	}

	DisplaySourceLines(
		lineInfo.FileName, 
		lineInfo.LineNumber,
		(unsigned int)lineInfo.Address,
		afterLines, 
		beforeLines);
}



//显示源文件中指定的行
void DisplaySourceLines(LPCTSTR sourceFile, int lineNum, unsigned int address, int after, int before) {

	std::wcout << std::endl;

	std::wifstream inputStream(sourceFile);

	if (inputStream.fail() == true) {

		std::wcout << TEXT("Open source file failed.") << std::endl 
				   << TEXT("Path: ") << sourceFile << std::endl;
		return;
	}

	inputStream.imbue(std::locale("chs", std::locale::ctype));

	int curLineNumber = 1;

	//计算从第几行开始输出
	int startLineNumber = lineNum - before;
	if (startLineNumber < 1) {
		startLineNumber = 1;
	}

	std::wstring line;

	//跳过不需要显示的行
	while (curLineNumber < startLineNumber) {

		std::getline(inputStream, line);
		++curLineNumber;
	}

	//输出开始行到当前行之间的行
	while (curLineNumber < lineNum) {

		std::getline(inputStream, line);
		DisplayLine(sourceFile, line, curLineNumber, FALSE);
		++curLineNumber;
	}

	//输出当前行
	getline(inputStream, line);
	DisplayLine(sourceFile, line, curLineNumber, TRUE);
	++curLineNumber;

	//输出当前行到最后行之间的行
	int lastLineNumber = lineNum + after;
	while (curLineNumber <= lastLineNumber) {

		if (!getline(inputStream, line)) {
			break;
		}

		DisplayLine(sourceFile, line, curLineNumber, FALSE);
		++curLineNumber;
	}

	inputStream.close();
}



//显示源文件中的一行。
void DisplayLine(LPCTSTR sourceFile, const std::wstring& line, int lineNumber, BOOL isCurLine) {

	if (isCurLine == TRUE) {
		std::wcout << TEXT("=>");
	}
	else {
		std::wcout << TEXT("  ");
	}

	LONG displacement;
	IMAGEHLP_LINE64 lineInfo = { 0 };
	lineInfo.SizeOfStruct = sizeof(lineInfo);

	if (SymGetLineFromName64(
		GetDebuggeeHandle(),
		NULL,
		sourceFile,
		lineNumber, 
		&displacement,
		&lineInfo) == FALSE) {

		std::wcout << TEXT("SymGetLineFromName64 failed: ") << GetLastError() << std::endl;
		return;
	}

	std::wcout << std::setw(4) << std::setfill(TEXT(' ')) << lineNumber << TEXT("  ");

	if (displacement == 0) {

		PrintHex((unsigned int)lineInfo.Address, FALSE);
	}
	else {

		std::wcout << std::setw(8) << TEXT(" ");
	}

	std::wcout << TEXT("  ") << line << std::endl;
}