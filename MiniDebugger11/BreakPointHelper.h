#pragma once

#include <list>
#include <Windows.h>


#define BP_INIT 1       //初始断点
#define BP_CODE 2       //代码中的断点
#define BP_USER 3       //用户设置的断点
#define BP_STEP_OVER 4  //StepOver使用的断点
#define BP_STEP_OUT 5   //StepOut使用的断点


typedef struct {

	DWORD address;    //断点地址
	BYTE content;     //原指令第一个字节

} BREAK_POINT;



void InitializeBreakPointHelper();

int GetBreakPointType(DWORD address);

BOOL SetUserBreakPointAt(DWORD address);
BOOL RecoverUserBreakPoint(DWORD address);
BOOL CancelUserBreakPoint(DWORD address);

void SaveResetUserBreakPoint(DWORD address);
void ResetUserBreakPoint();
BOOL NeedResetBreakPoint();

const std::list<BREAK_POINT>& GetUserBreakPoints();

void SetTrapFlag();

void SetStepOverBreakPointAt(DWORD address);
void CancelStepOverBreakPoint();

void SetStepOutBreakPointAt(DWORD address);
void CancelStepOutBreakPoint();