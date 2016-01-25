///////////////////////////////////////////////////////////////////////////////
// Module Name: AutoLocker.h
// Written By: J.Liu
// Purpose: 自动锁定器类。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_AUTO_LOCKER_H__
#define __CMN_LIB_AUTO_LOCKER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CriticalSection.h"

namespace CommonLib {

	//
	// Locker Traits Definition
	//

	class CLockTraitsST
	{
	public:
		VOID Lock()  
			{
			}
		VOID Unlock()
			{
			}
	};

	class CLockTraitsMT
	{
	public:
		VOID Lock()  
			{ 
			m_cs.Lock();   
			}
		VOID Unlock()
			{
			m_cs.Unlock();
			}

	private:

		CCriticalSection m_cs;
	};

	template <typename _LockTraits>
		class CAutoLocker
		{
		public:
			// Typedefs

			typedef _LockTraits LockTraits;

		public:
			CAutoLocker(typename LockTraits& lt)
				: m_lt(lt)
				{
				m_lt.Lock();
				}
			~CAutoLocker()
				{
				m_lt.Unlock();
				}

		private:

			LockTraits& m_lt;
		};

	///////////////////////////////////////////////////////////////////////////

		typedef CAutoLocker<CLockTraitsST> AutoLockerST;
		typedef CAutoLocker<CLockTraitsMT> AutoLockerMT;

	#ifdef CMN_LIB_IN_MT
		typedef AutoLockerMT AutoLocker;
	#else
		typedef AutoLockerST AutoLocker;
	#endif // CMN_LIB_IN_MT

}

#endif __CMN_LIB_AUTO_LOCKER_H__