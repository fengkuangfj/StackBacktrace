#include <list>
#include <iostream>
#include "BreakPointHelper.h"
#include "DebugSession.h"


BYTE SetBreakPointAt(DWORD address);
void RecoverBreakPoint(const BREAK_POINT* pBreakPoint);



static BREAK_POINT g_stepOverBp;
static BREAK_POINT g_stepOutBp;

static std::list<BREAK_POINT> g_userBpList;

static BOOL g_isInitBpOccured = FALSE;

static DWORD g_resetUserBpAddress;




//初始化断点组件
void InitializeBreakPointHelper() {

	g_userBpList.clear();
	g_isInitBpOccured = FALSE;

	g_stepOverBp.address = 0;
	g_stepOverBp.content = 0;

	g_stepOutBp.address = 0;
	g_stepOutBp.content = 0;

	g_resetUserBpAddress = 0;
}



//获取断点的类型
int GetBreakPointType(DWORD address) {

	//如果初始断点未触发，则必定是初始断点
	if (g_isInitBpOccured == FALSE) {
		g_isInitBpOccured = TRUE;
		return BP_INIT;
	}

	//检查是否StepOver断点
	//注意这个检查要先于用户断点的检查，否则如果
	//用户断点与StepOver断点都设在同一个地址的话，
	//会发生混乱。
	if (g_stepOverBp.address == address) {
		return BP_STEP_OVER;
	}

	//检查是否StepOut断点
	if (g_stepOutBp.address == address) {
		return BP_STEP_OUT;
	}

	//在用户设置的断点中寻找
	for (std::list<BREAK_POINT>::const_iterator it = g_userBpList.cbegin();
		 it != g_userBpList.cend();
		 ++it) {

		if (it->address == address) {
			return BP_USER;
		}
	}

	//以上条件都不满足，则是被调试进程中的断点
	return BP_CODE;
}



//在指定的地址设置断点
BOOL SetUserBreakPointAt(DWORD address) {
	
	//检查是否已存在该断点
	for (std::list<BREAK_POINT>::const_iterator it = g_userBpList.cbegin();
		 it != g_userBpList.cend();
		 ++it) {

		if (it->address == address) {

			std::wcout << TEXT("Break point has existed.") << std::endl;
			return FALSE;
		}
	}

	BREAK_POINT newBp;
	newBp.address = address;
	newBp.content = SetBreakPointAt(newBp.address);

	g_userBpList.push_back(newBp);

	return TRUE;
}



//设置StepOver使用的断点
void SetStepOverBreakPointAt(DWORD address) {

	g_stepOverBp.address = address;
	g_stepOverBp.content = SetBreakPointAt(address);
}



//取消指定地址的断点
BOOL CancelUserBreakPoint(DWORD address) {

	for (std::list<BREAK_POINT>::const_iterator it = g_userBpList.cbegin();
		 it != g_userBpList.cend();
		 ++it) {

		if (it->address == address) {

			RecoverBreakPoint(&*it);
			g_userBpList.erase(it);

			return TRUE;
		}
	}

	std::wcout << TEXT("Break point does not exist.") << std::endl;

	return FALSE;
}



//取消StepOver使用的断点
void CancelStepOverBreakPoint() {

	if (g_stepOverBp.address != 0) {

		RecoverBreakPoint(&g_stepOverBp);

		g_stepOverBp.address = 0;
		g_stepOverBp.content = 0;
	}
}



//恢复断点所在指令第一个字节的内容
BOOL RecoverUserBreakPoint(DWORD address) {
	
	for (std::list<BREAK_POINT>::const_iterator it = g_userBpList.cbegin();
		 it != g_userBpList.cend();
		 ++it) {

		if (it->address == address) {

			RecoverBreakPoint(&*it);
			return TRUE;
		}
	}

	std::wcout << TEXT("Break point does not exist") << std::endl;

	return FALSE;
}



//保存用于重新设置的用户断点地址
void SaveResetUserBreakPoint(DWORD address) {

	g_resetUserBpAddress = address;
}



//重新设置已保存的用户断点
void ResetUserBreakPoint() {

	for (std::list<BREAK_POINT>::iterator it = g_userBpList.begin(); 
		 it != g_userBpList.end();
		 ++it) {

		if (it->address == g_resetUserBpAddress) {

			SetBreakPointAt(it->address);

			g_resetUserBpAddress = 0;
		}
	}
}



//返回是否需要重新设置用户断点的指示值
BOOL NeedResetBreakPoint() {

	return g_resetUserBpAddress != 0;
}



//返回用户设置断点的链表
const std::list<BREAK_POINT>& GetUserBreakPoints() {

	return g_userBpList;
}



//设置TF标志位
void SetTrapFlag() {

	CONTEXT context;

	GetDebuggeeContext(&context);

	context.EFlags |= 0x100;

	SetDebuggeeContext(&context);
}



//重新设置所有的用户断点
void ResetUserBreakPoints() {

	for (std::list<BREAK_POINT>::iterator iterator = g_userBpList.begin();
		 iterator != g_userBpList.end();
		 ++iterator) {

		BYTE byte;
		ReadDebuggeeMemory(iterator->address, 1, &byte);

		if (byte != 0xCC) {
			SetBreakPointAt(iterator->address);
		}
	}
}



//在指定地址设置断点，返回值为原指令的第一个字节
BYTE SetBreakPointAt(DWORD address) {

	BYTE byte;
	ReadDebuggeeMemory(address, 1, &byte);
	
	BYTE intInst = 0xCC;
	WriteDebuggeeMemory(address, 1, &intInst);

	return byte;
}



//恢复断点所在指令的第一个字节
void RecoverBreakPoint(const BREAK_POINT* pBreakPoint) {

	WriteDebuggeeMemory(pBreakPoint->address, 1, &pBreakPoint->content);
}



//设置StepOut使用的断点
void SetStepOutBreakPointAt(DWORD address) {

	g_stepOutBp.address = address;
	g_stepOutBp.content = SetBreakPointAt(address);
}



//取消StepOut使用的断点
void CancelStepOutBreakPoint() {

	if (g_stepOutBp.address != 0) {

		RecoverBreakPoint(&g_stepOutBp);

		g_stepOutBp.address = 0;
		g_stepOutBp.content = 0;
	}
}

