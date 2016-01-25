///////////////////////////////////////////////////////////////////////////////
// Module Name: Timer.cpp
// Written By: J.Liu
// Purpose: Implementation for CTimer class.
///////////////////////////////////////////////////////////////////////////////

#include "Timer.h"

#pragma comment (lib, "Winmm.lib.")

CMN_LIB_NS_BEG

//
// Resolution
//
const UINT TARGET_RESOLUTION = 1;

MMRESULT CTimer::CreateTimer(
							 UINT nDelay, 
							 LPTIMECALLBACK lpfnTimeCallback, 
							 DWORD_PTR dwUserData, 
							 UINT fuEvent
							 )
{		
	return (::timeSetEvent(nDelay, GetTimerResolution(), 
				lpfnTimeCallback, dwUserData, fuEvent));
}

void CTimer::DestroyTimer(
						  MMRESULT nTimerID
						  )
{
	if (nTimerID != NULL)
	{
		::timeKillEvent(nTimerID);
	}
}

UINT CTimer::GetTimerResolution()
{
	TIMECAPS tc;
	UINT wTimerRes;
	if (::timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
	{
		wTimerRes = 1;
	}
	else
	{
		wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
		::timeBeginPeriod(wTimerRes);
	}
	return (wTimerRes);
}

CMN_LIB_NS_END