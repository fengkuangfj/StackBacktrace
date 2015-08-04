#pragma once

#include <Windows.h>
#include <stdio.h>
#include <Dbghelp.h>

#pragma comment(lib, "Dbghelp.lib")

/*++
*
* Routine Description:
*
*		This is a wrapper function for walk frame chain. It's purpose is to
*		prevent entering a function that has a huge stack usage to test some
*		if the current code path can take page faults.
*
*		N.B. This is an exported function that MUST probe the ability to take
*		page faults.
*
* Arguments:
*
*		Callers -	Supplies a pointer to an array that is to received the return
*					address values.
*
*		Count	-	Supplies the number of frames to be walked.
*
*		Flags	-	Supplies the flags value (unused).
*
* Return value:
*
*		Any return value from RtlpWalkFrameChain.
*
--*/
typedef
	ULONG
	(WINAPI * RTLWALKFRAMECHAIN)(
	__out	PVOID *	Callers,
	__in	ULONG	Count,
	__in	ULONG	Flags
	);

BOOL
	CALLBACK
	ReadProcessMemoryProc64(
	_In_  HANDLE  hProcess,
	_In_  DWORD64 lpBaseAddress,
	_Out_ PVOID   lpBuffer,
	_In_  DWORD   nSize,
	_Out_ LPDWORD lpNumberOfBytesRead
	);


class CStackBacktrace
{
public:
	BOOL
		Init();

	BOOL
		Unload();

	BOOL
		StackBacktrace();

private:
	BOOL
		WalkFrameChaim();
};
