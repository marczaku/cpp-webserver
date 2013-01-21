// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <conio.h>

#pragma warning(disable: 4996)
#pragma warning(disable: 4995)

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>
#include <strsafe.h>
#include <MMSystem.h>
#include <process.h>
#include <math.h>
#include <MSWSock.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "strsafe.lib")