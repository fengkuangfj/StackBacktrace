#pragma once

#include "Command.h"



void OnStartDebug(const Command& cmd);
void OnShowRegisters(const Command& cmd);
void OnStopDebug(const Command& cmd);
void OnGo(const Command& cmd);
void OnDump(const Command& cmd);
void OnShowSourceLines(const Command& cmd);
void OnBreakPoint(const Command& cmd);
void OnStepIn(const Command& cmd);
void OnStepOver(const Command& cmd);
void OnStepOut(const Command& cmd);
void OnShowLocalVariables(const Command& cmd);
void OnShowGlobalVariables(const Command& cmd);
void OnFormatMemory(const Command& cmd);
void OnShowStackTrack(const Command& cmd);