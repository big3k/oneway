///////////////////////////////////////////////////////////////////////////////
// Module Name: CriticalSection.h
// Written By: J.Liu
// Purpose: 封装Win32临界区对象。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_CRITICAL_SECTION_H__
#define __CMN_LIB_CRITICAL_SECTION_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

namespace CommonLib {

	class CCriticalSection
	{
	public:
		// Constructor / Destructor

		CCriticalSection() 
			{
			::InitializeCriticalSection(&m_cs);
			}

		~CCriticalSection() 
			{
			::DeleteCriticalSection(&m_cs);
			}

	public:
		// Operations

		// Enter critical section
		void Lock() 
			{
			::EnterCriticalSection(&m_cs);
			}

		// Leave critical section
		void Unlock() 
			{
			::LeaveCriticalSection(&m_cs);
			}

	public:
		CRITICAL_SECTION m_cs;
	};

}

#endif // __CMN_LIB_CRITICAL_SECTION_H__