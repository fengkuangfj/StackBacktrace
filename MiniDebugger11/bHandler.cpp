#include <iostream>
#include <string>
#include "CmdHandlers.h"
#include "DebugSession.h"
#include "BreakPointHelper.h"
#include "HelperFunctions.h"


void DisplayBreakPoints();


//断点命令的处理函数。
//命令格式： b [address [d]]
//address使用16进制数
//如果省略了address，则显示已设置的断点
//如果省略了参数d，则设置断点
//没有省略则删除断点
void OnBreakPoint(const Command& cmd) {

	CHECK_DEBUGGEE;

	if (cmd.size() < 2) {

		DisplayBreakPoints();
		return;
	}

	int address = 0;
	BOOL isDel = FALSE;

	address = wcstoul(cmd[1].c_str(), NULL, 16);

	if (cmd.size() >= 3) {

		if (cmd[2] == TEXT("d")) {
			isDel = TRUE;
		}
		else {
			std::wcout << TEXT("Invalid params.") << std::endl;
			return;
		}
	}

	if (isDel == FALSE) {

		if (SetUserBreakPointAt(address) == TRUE) {
			std::wcout << TEXT("Set break point succeeded.") << std::endl;
		}
		else {
			std::wcout << TEXT("Set break point failed.") << std::endl;
		}
	}
	else {

		if (CancelUserBreakPoint(address) == TRUE) {
			std::wcout << TEXT("Cancel break point succeeded.") << std::endl;
		}
		else {
			std::wcout << TEXT("Cancel break point failed.") << std::endl;
		}
	}
}





//显示已设置的断点
void DisplayBreakPoints() {

	const std::list<BREAK_POINT> bpList = GetUserBreakPoints();

	if (bpList.size() == 0) {
		std::wcout << TEXT("No break point.") << std::endl;
		return;
	}

	for (std::list<BREAK_POINT>::const_iterator it = bpList.cbegin();
		 it != bpList.cend();
		 ++it) {

		PrintHex(it->address, FALSE);
		std::wcout << std::endl;
	}
}