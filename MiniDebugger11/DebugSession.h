#pragma once

#include <windows.h>


#define STATUS_NONE 1
#define STATUS_SUSPENDED 2
#define STATUS_INTERRUPTED 3



void StartDebugSession(LPCTSTR path);
void ContinueDebugSession();
void StopDebugSeesion();
void HandledException(BOOL handled);
BOOL GetDebuggeeContext(CONTEXT* pContext);
BOOL SetDebuggeeContext(CONTEXT* pContext);
int GetDebuggeeStatus();
BOOL ReadDebuggeeMemory(unsigned int address, unsigned int length, void* pData);
BOOL WriteDebuggeeMemory(unsigned int address, unsigned int length, const void* pData);
HANDLE GetDebuggeeHandle();
HANDLE GetDebuggeeThread();



#define CHECK_DEBUGGEE \
	do { \
		if (GetDebuggeeStatus() == STATUS_NONE) {	\
			std::wcout << L"Debuggee has not started yet." << std::endl; \
			return; \
		} \
	} \
	while (0)