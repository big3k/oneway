///////////////////////////////////////////////////////////////////////////////
// Module Name: Semaphore.h
// Written By: J.Liu
// Purpose: 封装Win32信号量对象。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_SEMAPHORE_H__
#define __CMN_LIB_SEMAPHORE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include "SyncObject.h"

namespace CommonLib {

	class CSemaphore : public CSyncObject
	{
	public:
		// Constructor / Copy Constructor

		CSemaphore()
			{
			}
		explicit CSemaphore(HANDLE h)
			: CSyncObject(h)
			{			
			}
		CSemaphore(CSemaphore& s)
			: CSyncObject(s)
			{			
			}
		CSemaphore(LONG nInitialCount, LONG nMaxCount)
			{
			Create(NULL, nInitialCount, nMaxCount, NULL);
			}
		CSemaphore(LPSECURITY_ATTRIBUTES pSA, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName)
			{
			Create(pSA, nInitialCount, nMaxCount, pszName);
			}

	public:
		// Operations

		// Create a new semaphore
		BOOL Create(LPSECURITY_ATTRIBUTES lpSA, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName)
			{
			m_handle = ::CreateSemaphore(lpSA, nInitialCount, nMaxCount, pszName);
			return (m_handle != NULL);
			}

		// Open an existing named semaphore
		BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR lpszName)
			{
			m_handle = ::OpenSemaphore(dwAccess, bInheritHandle, lpszName);
			return (m_handle != NULL);
			}

		// Decrease the count of the semaphore
		BOOL Request(DWORD dwTimeout = INFINITE)
			{
			DWORD lRes = WaitForSingleObject(m_handle, dwTimeout);		
			return (lRes != WAIT_TIMEOUT && lRes != WAIT_FAILED);
			}

		// Increase the count of the semaphore
		BOOL Release(LONG nReleaseCount = 1, LPLONG pnOldCount = NULL)
			{		
			return (::ReleaseSemaphore(m_handle, nReleaseCount, pnOldCount));
			}
	};

}

#endif // __CMN_LIB_SEMAPHORE_H__