#ifndef __THREAD_IS_IO_PENDING_H__
#define __THREAD_IS_IO_PENDING_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include "CommonDef.h"

namespace CommonLib {

	typedef enum _THREAD_INFORMATION_CLASS 
	{
		ThreadBasicInformation,
		ThreadTimes,
		ThreadPriority,
		ThreadBasePriority,
		ThreadAffinityMask,
		ThreadImpersonationToken,
		ThreadDescriptorTableEntry,
		ThreadEnableAlignmentFaultFixup,
		ThreadEventPair,
		ThreadQuerySetWin32StartAddress,
		ThreadZeroTlsCell,
		ThreadPerformanceCount,
		ThreadAmILastThread,
		ThreadIdealProcessor,
		ThreadPriorityBoost,
		ThreadSetTlsArrayAddress,
		ThreadIsIoPending,
		ThreadHideFromDebugger
	} THREAD_INFORMATION_CLASS, *PTHREAD_INFORMATION_CLASS;

	class CThreadIsIOPending
	{
		// Typedefs
		
		typedef LONG (WINAPI * LPFN_NT_QUERY_INFO_THREAD)(
														HANDLE hThread,
														THREAD_INFORMATION_CLASS eThreadInformationClass,
														PVOID lpThreadInformation,
														ULONG ulThreadInformationLength,
														PULONG ReturnLength
														);

	public:
		// Operations

		static BOOL IsPending(HANDLE hThread);

	private:

		static BOOL Initialize(VOID);

	private:

		static LPFN_NT_QUERY_INFO_THREAD NtQueryInformationThread;
	};

}

#endif // __THREAD_IS_IO_PENDING_H__