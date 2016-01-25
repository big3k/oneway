///////////////////////////////////////////////////////////////////////////////
// Module Name: Event.h
// Written By: J.Liu
// Purpose: 封装Win32事件对象。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_EVENT_H__
#define __CMN_LIB_EVENT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncObject.h"

namespace CommonLib {

	class CEvent : public CSyncObject
	{
	public:
		// Constructor / Copy Constructor

		CEvent() 
			{
			}
		explicit CEvent(HANDLE h) 
			: CSyncObject(h)
			{
			}
		CEvent(CEvent& e) 
			: CSyncObject(e)
			{
			}
		CEvent(BOOL bManualReset, BOOL bInitialState)
			{
			Create(NULL, bManualReset, bInitialState, NULL);
			}
		CEvent(LPSECURITY_ATTRIBUTES lpSA, BOOL bManualRest, BOOL bInitialState, LPCTSTR lpszName)
			{
			Create(lpSA, bManualRest, bInitialState, lpszName);
			}


	public:
		// Operations

		// Create a new event
		BOOL Create(LPSECURITY_ATTRIBUTES lpSA, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpszName)
			{
			m_handle = ::CreateEvent(lpSA, bManualReset, bInitialState, lpszName);
			return (m_handle != NULL);
			}

		// Open an existing named event
		BOOL Open(DWORD dwAccess, BOOL bInheritHandle, LPCTSTR lpszName)
			{
			m_handle = ::OpenEvent(dwAccess, bInheritHandle, lpszName);
			return (m_handle != NULL);
			}

		// Set the event to the signaled state
		BOOL SetEvent() 
			{
			CMN_LIB_VERIFY(m_handle);
			return (::SetEvent(m_handle));
			}

		// Set the event to the non-signaled state
		BOOL Reset() 
			{		
			CMN_LIB_VERIFY(m_handle);
			return (::ResetEvent(m_handle));
			}

		// Pulse the event (signals waiting objects, then resets)
		BOOL Pulse() 
			{
			CMN_LIB_VERIFY(m_handle);
			return (::PulseEvent(m_handle));
			}
	};

}

#endif // __UTILITY_EVENT_H__