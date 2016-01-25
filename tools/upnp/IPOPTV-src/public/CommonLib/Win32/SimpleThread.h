#ifndef __SIMPLE_THREAD_H__
#define __SIMPLE_THREAD_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ThreadCreator.h"
#include "ThreadIsIOPending.h"

namespace CommonLib {

	template <typename _ThreadCreator = CmnLibThreadCreator>
		class CSimpleThread
		{
		public:
			// Constructor / Destructor

			CSimpleThread()
				: m_hThread(NULL), m_ulThreadId(0) 
				{
				}
			virtual ~CSimpleThread()
				{
				Terminate(0, 0);
				}

		protected:
			// Overridables

			static  UINT WINAPI ThreadProc(LPVOID lpVoid)
				{
				return (reinterpret_cast<CSimpleThread*>(lpVoid)->InternalThreadProc());
				}
			virtual UINT InternalThreadProc() 
				{
				}

		public:
			// Operations
			
			HRESULT Create(
						   ULONG ulCreationFlags = 0, 
						   ULONG ulStackSize = 0,
						   LPSECURITY_ATTRIBUTES lpAttributes = NULL
						   )
				{
				if (m_hThread != NULL)
					{
					return (NOERROR);
					}
				m_hThread = _ThreadCreator::Create(
												   lpAttributes, 
												   ulStackSize, 
												   CSimpleThread::ThreadProc,
												   this,
												   ulCreationFlags,
												   &m_ulThreadId
												   );
				if (m_hThread == NULL)
					{
					return (HRESULT_FROM_WIN32(GetLastError()));
					}
				return (NOERROR);
				}
			HRESULT SuspendThread(ULONG* pulPreviousSuspendCount)
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				ULONG ulCnt = ::SuspendThread(m_hThread);
				if (ulCnt == -1)
					{
					return (HRESULT_FROM_WIN32(GetLastError()));
					}
				CMN_LIB_SAFE_ASSIGN(pulPreviousSuspendCount, ulCnt);
				return (NOERROR);
				}
			HRESULT ResumeThread(ULONG* pulPreviousSuspendCount)
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				ULONG ulCnt = ::ResumeThread(m_hThread);
				if (ulCnt == -1)
					{
					return (HRESULT_FROM_WIN32(GetLastError()));
					}
				CMN_LIB_SAFE_ASSIGN(pulPreviousSuspendCount, ulCnt);				
				return (NOERROR);
				}
			HRESULT WaitThread(ULONG ulMilsecs = INFINITE, ULONG ulExitCode = 0)
				{
				if (m_hThread != NULL)
				{
					if (::WaitForSingleObject(m_hThread, ulMilsecs) != WAIT_OBJECT_0)
					{
						::TerminateThread(m_hThread, ulExitCode);
					}
					CloseHandle(m_hThread);
					m_hThread	 = NULL;
					m_ulThreadId = 0;
				}
				return (NOERROR);	// Always return NOERROR.
				}
		public:
			// Attributes
			
			HANDLE GetHandle() const 
				{ 
				return (m_hThread); 
				}
			ULONG GetIdentifier() const 
				{ 
				return (m_ulThreadId); 
				}
			HRESULT	GetPriority(INT* piPriority) const
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				INT iResult = ::GetThreadPriority(m_hThread);
				if (iResult == THREAD_PRIORITY_ERROR_RETURN)
					{
					return (HRESULT_FROM_WIN32(GetLastError()));
					}
				CMN_LIB_SAFE_ASSIGN(piPriority, iResult);
				return (NOERROR);
				}
			HRESULT SetPriority(INT iPriority)
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				INT iResult = SetThreadPriority(m_hThread, iPriority);
				if (iResult == 0)
					{
					return (HRESULT_FROM_WIN32(GetLastError()));
					}				
				return (NOERROR);
				}
			HRESULT GetExitCode(ULONG* pulExitCode) const
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				if (pulExitCode != NULL)
					{				
					if (!::GetExitCodeThread(m_hThread, pulExitCode))
						{
						return (HRESULT_FROM_WIN32(GetLastError()));
						}
					}
				return (NOERROR);			
				}
			HRESULT	ThreadHasIoPending()
				{
				CMN_LIB_CHK_HANDLE(m_hThread);
				return (CThreadIsIOPending::IsPending(m_hThread) ? S_OK : S_FALSE);
				}

		protected:
			// Members

			HANDLE	m_hThread;
			ULONG	m_ulThreadId;		
		};

}

#endif // __SIMPLE_THREAD_H__

