///////////////////////////////////////////////////////////////////////////////
// Module Name: Timer.h
// Written By: J.Liu
// Purpose: Win32多媒体定时器类.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_TIMER_H__
#define __CMN_LIB_TIMER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

///////////////////////////////////////////////////////////////////////////////

CMN_LIB_NS_BEG

	class CTimer
	{
	public:
		static MMRESULT CreateTimer(UINT nDelay, LPTIMECALLBACK lpfnTimeCallback, 
							 DWORD_PTR dwUserData = 0, UINT fuEvent = TIME_PERIODIC);

		static void DestroyTimer(MMRESULT nTimerID);

	private:
		static UINT GetTimerResolution();
	};

CMN_LIB_NS_END

#endif // __CMN_LIB_TIMER_H__