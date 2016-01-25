///////////////////////////////////////////////////////////////////////////////
// Module Name: SyncObject.h
// Written By: J.Liu
// Purpose: 封装Win32互排量对象。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_MUTEX_H__
#define __CMN_LIB_MUTEX_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncObject.h"

namespace CommonLib {

	class CMutex : public CSyncObject
	{
	public:
		// Constructor / Copy Constructor
		
		CMutex()
			{
			}
		explicit CMutex(BOOL bInitialOwner)
			{ 
			Create(NULL, bInitialOwner, NULL); 
			}
		explicit CMutex(HANDLE h) 
			: CSyncObject(h) 
			{
			}
		CMutex(CMutex& m)
			: CSyncObject(m)
			{
			}
		
		CMutex(LPSECURITY_ATTRIBUTES lpSA, BOOL bInitialOwner, LPCTSTR lpszName) 
			{ 
			Create(lpSA, bInitialOwner, lpszName); 
			}	

	public:			
		// Operations

		BOOL Lock(DWORD dwTimeout = INFINITE)
			{
			CMN_LIB_VERIFY(m_handle);
			DWORD lRes = WaitForSingleObject(m_handle, dwTimeout);		
			return (lRes != WAIT_ABANDONED && lRes != WAIT_TIMEOUT && lRes != WAIT_FAILED);
			}

		BOOL Unlock()
			{
			CMN_LIB_VERIFY(m_handle);
			return (::ReleaseMutex(m_handle));
			}

		// Create a new mutex object
		BOOL Create(LPSECURITY_ATTRIBUTES lpSA, BOOL bInitialOwner, LPCTSTR lpszName)
			{
			m_handle = ::CreateMutex(lpSA, bInitialOwner, lpszName);
			return (m_handle != NULL);
			}

		// Open an existing named mutex
		BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR lpszName)
			{
			m_handle = ::OpenMutex(dwAccess, bInheritHandle, lpszName);
			return (m_handle != NULL);
			}
	};

}

#endif // __CMN_LIB_MUTEX_H__