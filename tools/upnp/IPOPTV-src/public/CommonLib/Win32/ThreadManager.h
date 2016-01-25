///////////////////////////////////////////////////////////////////////////////
// Module Name: ThreadManager.h
// Written By: J.Liu
// Purpose: 一个线程管理器。它维护一个线程池。使用启发式算法来管理线程。基于
//			完成端口来处理。
// Remark:		需要实现一个Worker类。改类实现两个方法：
//				(1)	Execute: 完成特定的工作。
//				(2) StayInPool: Worker对象所在的工作线程是否还需要继续留在池中。
//				该类声明如下所示:
//				struct CWorker
//				{
//				public:
//					// Typedefs
//					typedef MyReqeustType RequestType;
//		
//				public:
//					// Operations
//					
//					BOOL Initialize(void* pWorkerParam);
//					void Process(
//								 void* pWorkerParam,			
//								 BOOL bDequeue,					// 出对是否成功
//								 ULONG ulNumberOfBytes,			// 已传递的字节数
//								 PULONG_PTR lpCompletionKey,	
//								 OVERLAPPED* lpOv						 
//								 );
//					BOOL CanLeave(LPVOID pWorkerParam);
//					void Terminate(void* pWorkerParam);
//				};
//
// Reference:	(1) Programming Server-Side Applications for MS 2000.
//					Part I\Required Reading\
//					Chapter2 Device I/O and Interthread Communication\
//					Receiving Completed I/O Request Notifications
//				(2)	CThreadPool class in the ATL.
///////////////////////////////////////////////////////////////////////////////

#ifndef __THREAD_MANAGER_H__
#define __THREAD_MANAGER_H__

#include "CPU.h"
#include "ThreadCreator.h"

namespace CommonLib {

#define THREAD_EXIT_FLAG ((ULONG)-1)

	template <
			typename _Worker, 
			typename _ThreadCreator = CmnLibThreadCreator
			>
	class CThreadManager
	{
	public:
		// Constructor / Destructor

		CThreadManager()
		{
			m_hIocp = NULL;
			m_pWorkerParam = NULL;
			m_ulThreadWaitTime = 0;
			m_ulStackSize = 0;
			m_lMinThreads = 0;
			m_lMaxThreads = 0;
			m_lCrntThreads = 0;
			m_lBusyThreads = 0;
		}
		~CThreadManager()
		{
			Stop();
		}

	public:
		// Operations

		HRESULT Init(
					LPVOID pWorkerParam,
					ULONG ulThreadWaitTime = INFINITE,
					ULONG ulNumberOfConcurrentThreads = 0,
					ULONG ulStackSize = 0,
					HANDLE hComPort = INVALID_HANDLE_VALUE
					)
			{
			CMN_LIB_VERIFY(m_hIocp == NULL);

			if (m_hIocp != NULL)
				{
				// Already initialized
				return (ERROR_ALREADY_INITIALIZED);
				}		
			
			// Create IO completion port to queue the requests
			m_hIocp = CreateIoCompletionPort(hComPort, NULL, 0, ulNumberOfConcurrentThreads);
			if (m_hIocp == NULL)
				{
				// Failed creating the Io completion port			
				return (GetLastError());
				}

			m_pWorkerParam		= pWorkerParam;
			m_ulThreadWaitTime	= ulThreadWaitTime;
			m_ulStackSize		= ulStackSize;

			HRESULT hr = CreateThreads();
			if (FAILED(hr))
				{
				Stop();
				return (hr);
				}
			return (S_OK);
			}
		HRESULT Stop()
			{
			DestroyThreads();
			if (m_hIocp != NULL)
				{
				CloseHandle(m_hIocp);
				m_hIocp = NULL;
				}
			return (S_OK);
			}
		HRESULT AssociateWithQueue(HANDLE hDevice, ULONG_PTR ulComKey)
			{
			if (m_hIocp == NULL)
				{
				return (E_HANDLE);
				}
			if (CreateIoCompletionPort(hDevice, m_hIocp, ulComKey, 0) == NULL)
				{
				return (GetLastError());
				}
			return (S_OK);
			}
		HRESULT EnqueueRequest(ULONG ulCompletionKey, OVERLAPPED* pOv = NULL)
			{
			if (!PostQueuedCompletionStatus(m_hIocp, 0, (ULONG_PTR)ulCompletionKey, pOv))
				{
				return (GetLastError());
				}
			return (S_OK);
			}

	protected:
		// Internal Operations

		static DWORD WINAPI ThreadProc(void* pParam)
			{
			CThreadManager* pThis = reinterpret_cast<CThreadManager*>(pParam);
			return (pThis->InternalThreadProc());
			}
		HRESULT CreateThreads()
			{
			SYSTEM_INFO si;
			GetSystemInfo(&si);

			m_lMinThreads = si.dwNumberOfProcessors;
			m_lMaxThreads = si.dwNumberOfProcessors * 2;

			for (LONG i = 0; i < m_lMinThreads; i++)
				{
				HRESULT hr = CreateNewThread();
				if (FAILED(hr))
					{
					return (hr);
					}
				}
			return (S_OK);
			}
		HRESULT CreateNewThread()
			{		
			// First, Increment count of current thread
			InterlockedIncrement(&m_lCrntThreads);
			ULONG ulThreadId = 0;
			HANDLE hThread = _ThreadCreator::CreateThread(
														NULL, 
														m_ulStackSize, 
														ThreadProc, 
														this, 
														0, 
														&ulThreadId
														);
			if (hThread == NULL)
				{
				// 失败。递减当前线程数
				// Failed. Derement count of current thread
				InterlockedDecrement(&m_lCrntThreads);
				return (GetLastError());
				}
			CloseHandle(hThread);

			return (S_OK);
			}
		void DestroyThreads()
			{
			LONG lCrntThreads = m_lCrntThreads;
			for (LONG i = 0; i < lCrntThreads; i++)
				{
				PostQueuedCompletionStatus(m_hIocp, 0, THREAD_EXIT_FLAG, NULL);
				}
			while (m_lCrntThreads != 0)
				{
				Sleep(1000);
				}
			}
		DWORD InternalThreadProc()
			{
			ULONG		 ulTransferred	= 0;
			LPOVERLAPPED lpOv			= NULL;		
			ULONG_PTR	 ulCompletionKey= 0;
			ULONG		 ulTimeOut		= m_ulThreadWaitTime;
			BOOL		 bStayInPool	= TRUE;
			_Worker		 theWorker;

			// Thread is entering pool		
			InterlockedIncrement(&m_lBusyThreads);

			if (theWorker.Initialize(m_pWorkerParam))
				{
				while (bStayInPool)
					{
					// Thread stops executing and waits for something to do
					InterlockedDecrement(&m_lBusyThreads);

					BOOL bDequeue = GetQueuedCompletionStatus(
															  m_hIocp,
															  &ulTransferred, 
															  &ulCompletionKey,
															  &lpOv,
															  ulTimeOut
															  );
					ULONG ulIOError = GetLastError();

					// Thread has something to do, so it's busy
					LONG lBusyThreads = InterlockedIncrement(&m_lBusyThreads);

					if (ulCompletionKey == THREAD_EXIT_FLAG)
						{
						// To exit...
						break;
						}	

					// Should we add another thread to the pool?
 					if (lBusyThreads == m_lCrntThreads)
						{
						// All threads are busy?
						if (lBusyThreads < m_lMaxThreads)
							{
							// The pool isn't full
							if (CPUUsage::GetCPUUsage() < 75)
								{
								// CPU usage is below 75%, Add thread to pool
								CreateNewThread();
								}
							}
						}

					if (bDequeue || lpOv)
						{
						// Thread woke to process something; process it
						theWorker.Process(
										m_pWorkerParam, 
										bDequeue,
										ulTransferred, 
										ulCompletionKey,
										lpOv
										);
						}

					// Two case that we may be think decrement number of threads:
					// 1 case: Thread wait for time-out
					// 2 case: CPU usage is above 90%
					BOOL bLeave = (!bDequeue && (ulIOError == WAIT_TIMEOUT)) || 
								(CPUUsage::GetCPUUsage() > 90);
					if (bLeave && (m_lCrntThreads > m_lMinThreads))
						{					
						if (theWorker.CanLeave(m_pWorkerParam))
							{
							// There isn't much for the work to do, and this thread
							// can die because it has no outstanding I/O requests
							bStayInPool = FALSE;
							}
						}				
					} // End of while (bStayInPool)

					theWorker.Terminate(m_pWorkerParam);
				} // End of if (theWorker.Initialize(m_pWorkerParam))

			InterlockedDecrement(&m_lBusyThreads);
			InterlockedDecrement(&m_lCrntThreads);		

			return (0);
			}

	protected:

		HANDLE	m_hIocp;

		void*	m_pWorkerParam;
		ULONG	m_ulThreadWaitTime;
		ULONG	m_ulStackSize;

		LONG	m_lMinThreads;
		LONG	m_lMaxThreads;
		LONG	m_lCrntThreads;
		LONG	m_lBusyThreads;
	};

}

#endif // __THREAD_MANAGER_H__