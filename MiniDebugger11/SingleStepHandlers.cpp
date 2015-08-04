#include <iostream>
#include "CmdHandlers.h"
#include "DebugSession.h"
#include "SingleStepHelper.h"
#include "BreakPointHelper.h"



void OnStepIn(const Command& cmd) {

	CHECK_DEBUGGEE;

	SaveCurrentLineInfo();

	SetTrapFlag();

	SetSingleInstruction(TRUE);

	ContinueDebugSession();
}



void OnStepOver(const Command& cmd) {

	CHECK_DEBUGGEE;

	SaveCurrentLineInfo();

	SetStepOver(TRUE);

	//检查当前指令是否CALL指令
	CONTEXT context;
	GetDebuggeeContext(&context);

	int callLen = IsCallInstruction(context.Eip);

	//如果是，则在下一条指令处设置断点
	if (callLen != 0) {

		SetStepOverBreakPointAt(context.Eip + callLen);
		SetSingleInstruction(FALSE);
	}
	//如果不是，则设置TF位
	else {

		SetTrapFlag();
		SetSingleInstruction(TRUE);
	}

	ContinueDebugSession();
}



void OnStepOut(const Command& cmd) {

	CHECK_DEBUGGEE;

	SetStepOut(TRUE);

	SetSingleInstruction(FALSE);

	DWORD retAddress = GetRetInstructionAddress();

	if (retAddress != 0) {

		SetStepOutBreakPointAt(retAddress);
	}

	ContinueDebugSession();
}