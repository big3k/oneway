///////////////////////////////////////////////////////////////////////////////
// Module Name: CharConverter.h
// Written By: J.Liu
// Purpose: ½«¿í×Ö·û×Ö·û´®×ª»»Îª¶à×Ö½Ú×Ö·û´®»ò½«¶à×Ö½Ú×Ö·û´®×ª»»Îª¿í×Ö·û´®.
///////////////////////////////////////////////////////////////////////////////

#include "CPU.h"

namespace CommonLib {

static inline double LargeInteger2Double(const LARGE_INTEGER& _LI_VAL) 
{
	return (static_cast<double>(_LI_VAL.HighPart) * 4.294967296E9 + 
			static_cast<double>(_LI_VAL.LowPart));
}

SYSTEM_BASIC_INFORMATION CCpu::s_sysBasicInfo;
CCpu::LPFN_NT_QUERY_SYS_INFO CCpu::s_lpfnNTQuerySysInfo = NULL;

UINT CCpu::GetUsed()
{
	static LARGE_INTEGER liOldIdleTime   = {0, 0};
	static LARGE_INTEGER liOldSystemTime = {0, 0};

	if (!Initialize())
	{
		CMN_LIB_VERIFY(FALSE);
		return (0);
	}

	// Get new CPU's idle time
	SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
	if (s_lpfnNTQuerySysInfo(SIC_SYSTEM_PERFORMANCE_INFORMATION,
		&SysPerfInfo, sizeof (SysPerfInfo), NULL) != NO_ERROR)
	{
		return (0);
	}

	// Get new system time
	SYSTEM_TIME_INFORMATION	SysTimeInfo;
	if (s_lpfnNTQuerySysInfo(SIC_SYSTEM_TIME_INFORMATION,
		&SysTimeInfo, sizeof (SysTimeInfo), NULL) != NO_ERROR)
	{
		return (0);
	}

	double dbCPUUsed = 0.0f;
	if (liOldIdleTime.QuadPart != 0)
	{
		// CurrentValue = NewValue - OldValue
		double dbIdleTime	=	LargeInteger2Double(SysPerfInfo.liIdleTime) - 
								LargeInteger2Double(liOldIdleTime);

		double dbSystemTime =	LargeInteger2Double(SysTimeInfo.liKeSystemTime) - 
								LargeInteger2Double(liOldSystemTime);

		// CurrentCpuIdle = IdleTime / SystemTime
		dbIdleTime /= dbSystemTime;

		// CurrentCpuUsage % = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
		dbCPUUsed = 100.0f - dbIdleTime * 100.0f / (double)s_sysBasicInfo.bKeNumberProcessors + 0.5f;
	}

	// store new CPU's idle and system time
	liOldIdleTime = SysPerfInfo.liIdleTime;
	liOldSystemTime = SysTimeInfo.liKeSystemTime;

	return (static_cast<UINT>(dbCPUUsed));
}

BOOL CCpu::Initialize(VOID)
{
	// Get NtQuerySystemInformation's address from ntdll.dll
	if (s_lpfnNTQuerySysInfo == NULL)
	{
		s_lpfnNTQuerySysInfo = (LPFN_NT_QUERY_SYS_INFO)GetProcAddress(GetModuleHandle("ntdll"), "NtQuerySystemInformation");
		CMN_LIB_ASSERT(s_lpfnNTQuerySysInfo != NULL);
		if (s_lpfnNTQuerySysInfo == NULL)
		{
			return (FALSE);
		}

		// Get number of processors in the system			
		if (s_lpfnNTQuerySysInfo(SIC_SYSTEM_BASIC_INFORMATION, &s_sysBasicInfo, sizeof (s_sysBasicInfo), NULL) != NO_ERROR)
		{
			CMN_LIB_VERIFY(FALSE);
			return (FALSE);
		}
	}	
	return (TRUE);
}

}