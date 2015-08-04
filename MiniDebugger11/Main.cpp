#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <string>
#include <sstream>
#include "Command.h"
#include "CmdHandlers.h"





void SeparateCommand(const std::wstring& cmdLine, Command& cmd);
BOOL DispatchCommand(const Command& cmd);
void ClearCommand(Command& cmd);



int wmain(int argc, wchar_t** argv) {

	std::wcout.imbue(std::locale("chs", std::locale::ctype));

	std::wcout << TEXT("MiniDebugger by Zplutor") << std::endl << std::endl;

	Command cmd;
	std::wstring cmdLine;

	while (true) {
		
		std::wcout << std::endl << TEXT("> ");
		std::getline(std::wcin, cmdLine);
		
		SeparateCommand(cmdLine, cmd);

		if (DispatchCommand(cmd) == FALSE) {
			break;
		}

		ClearCommand(cmd);
	}

	return 0;
}



//以空白字符作为分隔符，将命令行拆分成几个部分，添加到Command中。
//两个双引号内的内容作为一部分。
//该方法未经完全测试，可能有BU。。
void SeparateCommand(const std::wstring& cmdLine, Command& cmd) {

	std::wistringstream cmdStream(cmdLine);

	std::wstring partialArg;
	std::wstring fullArg;
	BOOL isFullArg = TRUE;

	cmdStream >> partialArg;
	cmd.push_back(partialArg);

	while (cmdStream >> partialArg) {

		if (partialArg.at(0) == TEXT('\"')) {

			isFullArg = FALSE;

			if (partialArg.at(partialArg.length() - 1) != TEXT('\"')) {

				fullArg.append(partialArg);
				fullArg.append(L" ");
				continue;
			}
		}

		if (isFullArg == FALSE) {
			
			if (partialArg.at(partialArg.length() - 1) == L'\"') {

				isFullArg = TRUE;
				fullArg.append(partialArg);
				fullArg.erase(0, 1);
				fullArg.erase(fullArg.length() - 1, 1);
				cmd.push_back(fullArg);
			}
			else {

				fullArg.append(partialArg);
				fullArg.append(L" ");
			}

			continue;
		}

		cmd.push_back(partialArg);
	}
}



//根据命令的第一个部分调用相应的处理函数。
//如果命令为“q”，则返回FALSE，告诉调用
//方退出程序。其它情况均返回TRUE。
BOOL DispatchCommand(const Command& cmd) {

	static struct {
		LPCTSTR cmd;
		void (*handler)(const Command&);
	} cmdMap[] = {
	
		{ TEXT("s"), OnStartDebug},
		{ TEXT("g"), OnGo },
		{ TEXT("d"), OnDump },
		{ TEXT("l"), OnShowSourceLines },
		{ TEXT("b"), OnBreakPoint },
		{ TEXT("r"), OnShowRegisters },
		{ TEXT("in"), OnStepIn },
		{ TEXT("over"), OnStepOver },
		{ TEXT("out"), OnStepOut },
		{ TEXT("lv"), OnShowLocalVariables },
		{ TEXT("gv"), OnShowGlobalVariables },
		{ TEXT("f"), OnFormatMemory },
		{ TEXT("w"), OnShowStackTrack },
		{ TEXT("t"), OnStopDebug },
		{ NULL, NULL },
	};

	if (cmd.size() == 0) {
		return TRUE;
	}
	else if (cmd[0] == TEXT("q")) {
		return FALSE;
	}

	for (int index = 0; cmdMap[index].cmd != NULL; ++index) {

		if (cmd[0] == cmdMap[index].cmd) {

			cmdMap[index].handler(cmd);
			return TRUE; 
		}
	}

	std::wcout << TEXT("Invalid command.") << std::endl;
	return TRUE;
}



//清空命令结构。
void ClearCommand(Command& cmd) {

	cmd.clear();
}