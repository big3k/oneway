///////////////////////////////////////////////////////////////////////////////
// Module Name: CommonDef.h
// Written By: J.Liu
// Purpose: 公共定义文件。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_COMMON_DEF_H__
#define __CMN_LIB_COMMON_DEF_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _WINDOWS_
#include <Windows.h>
#endif // _WINDOWS_

#include "Utility\Verify.h"
#include "String\TString.h"

// disable warning 'initializers put in unrecognized initialization area'
# pragma warning ( disable : 4075 )
// disable warning 'empty controlled statement found'
# pragma warning ( disable : 4390 )
// disable warning 'debug symbol greater than 255 chars'
# pragma warning ( disable : 4786 )
// disable warning 'function was declared deprecated"
# pragma warning ( disable : 4996)

//
// Utility use in Multi-threads
//

//
// Use this macro when you need thread-safe.
//
//#define CMN_LIB_USE_IN_MT

#ifdef UNICODE
#ifndef _UNICODE
	#define _UNICODE
#endif
#endif // UNICODE

#ifdef CMN_LIB_USE_IN_MT
# define CMN_LIB_VOLATILE volatile
#else
# define CMN_LIB_VOLATILE
#endif // _MT

//
// Namespace Macro
//
//#define CMN_LIB_NS_NAME CommonLib
//#define CommonLib namespace CommonLib {
//#define }

//
// EH Macro
//
#define CMN_LIB_TRY_BEGIN try {
#define CMN_LIB_CATCH(X) } catch (X) {
#define CMN_LIB_CATCH_ALL } catch (...) {
#define CMN_LIB_CATCH_END }
#define CMN_LIB_THROW(X) throw (X);
#define CMN_LIB_NOTHROW throw ()
#define CMN_LIB_RETHROW throw;

//
// CRT-Compiler Macro
//
#ifdef _UNICODE
# define __CMN_LIB_FILE__	L##__FILE__
# define __CMN_LIB_FUNC__	L##__FUNCTION__
#else
# define __CMN_LIB_FILE__	__FILE__
# define __CMN_LIB_FUNC__	__FUNCTION__
#endif // _UNICODE

//
// Safe Deleter Macro
//
#define CMN_LIB_SAFE_DELETE(_Ptr) { if (_Ptr) { delete _Ptr; _Ptr = NULL; } }
#define CMN_LIB_SAFE_DELETE_ARRAY(_Ptr) { if (_Ptr) { delete [] _Ptr; _Ptr = NULL; } }

//
// Get Number of Array's Element
//
#define CMN_LIB_ARRAY_SIZE(_Array) (sizeof (_Array) / sizeof (_Array[0]))

#define CMN_LIB_UNUSED(unused) unused

//
// HRESULT Value Helper Macro
//

#define CMN_LIB_HR(_Exp) { HRESULT hr = _Exp; if (FAILED(hr)) { return (hr);} }
#define CMN_LIB_CHK_HANDLE(_Handle) { if (_Handle == NULL) { return (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)); } }
#define CMN_LIB_CHK_POINTER(_Ptr) { if (_Ptr == NULL) { return (E_POINTER); } }
#define CMN_LIB_SAFE_ASSIGN(_Ptr, _Value) { CMN_LIB_CHK_POINTER(_Ptr); *_Ptr = _Value; return (S_OK); }
#define CMN_LIB_SAFE_RELEASE(_Ptr) { if (_Ptr) { _Ptr->Release(); _Ptr = NULL; } }
#define CMN_LIB_SAFE_PTR_ASSIGN(_Left, _Right) { if (_Right) { _Right->AddRef(); } if (_Left) { _Left->Release(); }	_Left = _Right;	}

namespace CommonLib {

//
// Safe Deleter Function Object
//
class CCmnLibSafeDeleter
{
public:
	template <typename _Ty>
		void operator ()(_Ty*& _Ptr)
		{
		CMN_LIB_SAFE_DELETE(_Ptr);
		}
};

//
// Singletion Class
//
template <typename _Ty>
	class CCmnLibSingleton
	{
	public:
		// Typedefs

		typedef typename _Ty  value_type;
		typedef typename _Ty* pointer_type;

	public:
		// Operations

		static BOOL CreateInstance()
			{
			if (s_pInstance == NULL)
				{
				s_pInstance = new value_type;
				}
			return (s_pInstance != NULL);
			}

		static pointer_type GetInstance()
			{
			return (s_pInstance);
			}

		static void DestroyInstance()
			{
			CMN_LIB_SAFE_DELETE(s_pInstance);
			}

	protected:

		static pointer_type s_pInstance;
	};

template <typename _Ty>
	typename CCmnLibSingleton<_Ty>::pointer_type CCmnLibSingleton<_Ty>::s_pInstance = NULL;

}

#endif // __CMN_LIB_COMMON_DEF_H__

