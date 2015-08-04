#pragma once

#include <Windows.h>
#include <stdio.h>

BOOL
	GetModulePath(
	__in_opt	HMODULE	hModule,
	__in		LPTSTR	lpInBuf,
	__in		ULONG	ulInBufSizeCh
	);
