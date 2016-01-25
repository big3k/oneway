///////////////////////////////////////////////////////////////////////////////
// Module Name: SyncObject.h
// Written By: J.Liu
// Purpose: 定义Win32同步对象的公共基类。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_SYNC_OBJECT_H__
#define __CMN_LIB_SYNC_OBJECT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

namespace CommonLib {

	class CSyncObject
	{
	public:
		// Constructor / Destructor

		CSyncObject()
			: m_handle(NULL)
			{	
			}

		CSyncObject(CSyncObject& so) 
			: m_handle(NULL)
			{
			Attach(so.Datach());
			}

		explicit CSyncObject(HANDLE h) 
			: m_handle(h)
			{
			}

		~CSyncObject() 
			{
			if (m_handle != NULL)
				{
				Close();			
				}
			}

		CSyncObject& operator =(CSyncObject& so) 
			{
			if (this != &so)
				{
				if (m_handle != NULL)
					{
					Close();
					}
				Attach(so.Datach());
				}
			return (*this);
			}

	public:
		// Operations

		// Attach to an existing handle (takes ownership)
		void Attach(HANDLE h)
			{		
			m_handle = h;
			}		

		// Datach the handle frome the object (releases ownership)
		HANDLE Datach()
			{
			HANDLE h = m_handle;
			m_handle = NULL;
			return (h);
			}

		// Close the handle
		void Close()
			{
			if (m_handle != NULL)
				{
				::CloseHandle(m_handle);
				m_handle = NULL;
				}
			}

	public:
		// Attributes

		operator HANDLE() const
			{
			return (m_handle);
			}

	public:

		HANDLE m_handle;
	};

}

#endif // __CMN_LIB_SYNC_OBJECT_H__