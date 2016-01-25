///////////////////////////////////////////////////////////////////////////////
// Module Name: CPUUsage.h
// Written By: J.Liu
// Purpose: 获取CPU的使用率。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_CPU_USAGE_H__
#define __CMN_LIB_CPU_USAGE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include "CommonDef.h"

namespace CommonLib {

	const UINT SIC_SYSTEM_BASIC_INFORMATION			= 0;
	const UINT SIC_SYSTEM_PERFORMANCE_INFORMATION	= 2;
	const UINT SIC_SYSTEM_TIME_INFORMATION			= 3;

	typedef struct tagSystemBasicInformation
	{
		DWORD   dwUnknown1;
		ULONG   uKeMaximumIncrement;
		ULONG   uPageSize;
		ULONG   uMmNumberOfPhysicalPages;
		ULONG   uMmLowestPhysicalPage;
		ULONG   uMmHighestPhysicalPage;
		ULONG   uAllocationGranularity;
		PVOID   pLowestUserAddress;
		PVOID   pMmHighestUserAddress;
		ULONG   uKeActiveProcessors;
		BYTE    bKeNumberProcessors;
		BYTE    bUnknown2;
		WORD    wUnknown3;
	} SYSTEM_BASIC_INFORMATION, *LPSYSTEM_BASIC_INFORMATION;

	typedef struct tagSystemPerformanceInformation
	{
		LARGE_INTEGER   liIdleTime;
		DWORD           dwSpare[76];
	} SYSTEM_PERFORMANCE_INFORMATION, *LPSYSTEM_PERFORMANCE_INFORMATION;

	typedef struct tagSystemTimeInformation
	{
		LARGE_INTEGER liKeBootTime;
		LARGE_INTEGER liKeSystemTime;
		LARGE_INTEGER liExpTimeZoneBias;
		ULONG         uCurrentTimeZoneId;
		DWORD         dwReserved;
	} SYSTEM_TIME_INFORMATION, *LPSYSTEM_TIME_INFORMATION;

	class CCpu
	{
		// ntdll!NtQuerySystemInformation (NT specific!)
		//
		// The function copies the system information of the
		// specified type into a buffer
		//
		// NTSYSAPI
		// NTSTATUS
		// NTAPI
		// NtQuerySystemInformation(
		//    IN UINT SystemInformationClass,    // information type
		//    OUT PVOID SystemInformation,       // pointer to buffer
		//    IN ULONG SystemInformationLength,  // buffer size in bytes
		//    OUT PULONG ReturnLength OPTIONAL   // pointer to a 32-bit
		//                                       // variable that receives
		//                                       // the number of bytes
		//                                       // written to the buffer 
		// );
		typedef LONG (WINAPI * LPFN_NT_QUERY_SYS_INFO)(UINT, PVOID, ULONG, PULONG);

	public:
		// Operations

		static UINT GetUsed();

	private:

		static BOOL Initialize(VOID);

	private:
		
		static SYSTEM_BASIC_INFORMATION	s_sysBasicInfo;
		static LPFN_NT_QUERY_SYS_INFO	s_lpfnNTQuerySysInfo;
	};

}

#endif // __CMN_LIB_CPU_USEGE_H__