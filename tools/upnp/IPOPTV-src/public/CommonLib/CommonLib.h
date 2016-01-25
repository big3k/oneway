///////////////////////////////////////////////////////////////////////////////
// Module Name: CommonLib.h
// Written By: J.Liu
// Purpose: 公共库头文件.
///////////////////////////////////////////////////////////////////////////////

#ifndef __COMMON_LIB_H__
#define __COMMON_LIB_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning (disable : 4100)

#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料

#include "Win32\MsWinsockUtil.h"
#include "Win32\CriticalSection.h"
#include "Win32\Semaphore.h"
#include "Win32\Mutex.h"
#include "Win32\Event.h"
#include "Win32\CPU.h"
#include "Win32\SimpleThread.h"
#include "Win32\ThreadManager.h"
#include "Win32\ThreadIsIOPending.h"
#include "Win32\ServiceModuleT.h"
#include "Memory\LookasideList.h"
#include "Memory\Allocator.h"
#include "Utility\Tracer.h"
#include "Utility\LogTool.h"
#include "Utility\Utility.h"
#include "String\TString.h"
#include "String\CharConverter.h"
#include "Encode\MD5.h"
#include "Manipulation\CircuitQueue.h"
#include "Manipulation\FunctionMapT.h"

#ifdef CMN_LIB_USE_IN_MT
#pragma message ("Use thread safe in common library! -- Slow --")
#else
#pragma message ("Not use thread safe in common library! -- Fast --")
#endif // CMN_LIB_USE_IN_MT

#endif // __COMMON_LIB_H__