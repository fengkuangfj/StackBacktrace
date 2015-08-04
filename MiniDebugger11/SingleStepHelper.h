#pragma once

#include <Windows.h>


typedef struct {

	WCHAR fileName[MAX_PATH];
	DWORD lineNumber; 

} LINE_INFO;



void InitializeSingleStepHelper();

void SaveCurrentLineInfo();
BOOL IsLineChanged();

void SetSingleInstruction(BOOL);
BOOL IsBeingSingleInstruction();

void SetStepOver(BOOL);
BOOL IsBeingStepOver();

void SetStepOut(BOOL);
BOOL IsBeingStepOut();


DWORD GetRetInstructionAddress();
int IsRetInstruction(DWORD address);
int IsCallInstruction(DWORD address);