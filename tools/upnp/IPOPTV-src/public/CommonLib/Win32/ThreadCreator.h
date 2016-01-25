#ifndef __THREAD_CREATOR_H__
#define __THREAD_CREATOR_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

namespace CommonLib {	

#ifdef _MT
	#include <process.h>
	//
	// CRT-Thread Create Macro
	//
	typedef UINT (__stdcall * CRT_THREAD_PROC)(void*);
	#define CMN_LIB_BEGIN_THREAD_EX(security, stack_size, start_address, arg_list, init_flag, thrdaddr) \
					(HANDLE)_beginthreadex((void*)(security), (unsigned int)(stack_size), \
					(CRT_THREAD_PROC)(start_address), (void*)(arg_list), \
					(unsigned)(init_flag), (unsigned*)(thrdaddr))

	class CCRTThreadCreator
	{
	public:
		static HANDLE CreateThread(
								   LPSECURITY_ATTRIBUTES lpsa, 
								   ULONG ulStackSize, 
								   LPTHREAD_START_ROUTINE pfnThreadProc, 
								   void* pvParam, 
								   ULONG ulCreationFlags, 
								   ULONG* pulThreadId
								   )
			{
			// sanity check for pdwThreadId
			CMN_LIB_VERIFY(sizeof (ULONG) == sizeof (unsigned int));			
			return (CMN_LIB_BEGIN_THREAD_EX(lpsa, ulStackSize, pfnThreadProc, pvParam, ulCreationFlags, pulThreadId));
			}
	};

#else

	class CWinThreadCreator
	{
	public:
		static HANDLE CreateThread(
								   LPSECURITY_ATTRIBUTES lpsa, 
								   ULONG ulStackSize, 
								   LPTHREAD_START_ROUTINE pfnThreadProc, 
								   void* pvParam, 
								   ULONG ulCreationFlags, 
								   ULONG* pulThreadId
								   )
			{
			return (::CreateThread(lpsa, ulStackSize, pfnThreadProc, pvParam, ulCreationFlags, pulThreadId));
			}
	};

#endif // _MT

#ifdef _MT
# define CmnLibThreadCreator CCRTThreadCreator
#else
# define CmnLibThreadCreator CWinThreadCreator
#endif 

}

#endif // __THREAD_CREATOR_H__