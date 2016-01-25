///////////////////////////////////////////////////////////////////////////////
// Module Name: Tracer.h
// Written By: J.Liu
// Purpose: 调试帮助类和宏定义。
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_TRACER_H__
#define __CMN_LIB_TRACER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include <tchar.h>
#include <stdio.h>

namespace CommonLib {

	
	//
	// Trace Macro
	//
	#ifdef _DEBUG
	# define CMN_LIB_OUTPUT_FROM_ID(MsgId)	CommonLib::CTracer::OutputErrorMessage(MsgId, __CMN_LIB_FILE__, __CMN_LIB_FUNC__, __LINE__);
	# define CMN_LIB_TRACE					CommonLib::CTracer::OutputDebugString
	# define CMN_LIB_FUNCTION_ENTER			CMN_LIB_TRACE(TEXT("%s Enter!\r\n"), __CMN_LIB_FUNC__);
	# define CMN_LIB_FUNCTION_LEAVE			CMN_LIB_TRACE(TEXT("%s Leave!\r\n"), __CMN_LIB_FUNC__);
	#else
	# define CMN_LIB_OUTPUT_FROM_ID(MsgId)	__noop;
	# define CMN_LIB_TRACE					__noop;
	# define CMN_LIB_FUNCTION_ENTER			__noop;
	# define CMN_LIB_FUNCTION_LEAVE			__noop;
	#endif // _DEBUG

	//
	// CLASS Tracer
	//

	const int MAX_BUFFER_SIZE = 1024;
	class CTracer
	{
	public:
		// Static Operations

		static BOOL LookupDescription(ULONG ulMessageId, LPTSTR szBuffer, INT iSize)
		{			
			// Get the error code's textual description			
			BOOL bOk = ::FormatMessage(
									   FORMAT_MESSAGE_FROM_SYSTEM,
									   NULL, 
									   ulMessageId, 
									   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
									   szBuffer, 
									   iSize, 
									   NULL
									   );			

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				return (bOk);
			}

			if (!bOk) 
			{
				// Is it a network-related error?
				HMODULE hDll = LoadLibraryEx(
											 TEXT("netmsg.dll"), 
											 NULL, 
											 DONT_RESOLVE_DLL_REFERENCES
											 );
				if (hDll != NULL) 
				{
					bOk = ::FormatMessage(
										  FORMAT_MESSAGE_FROM_HMODULE,
										  hDll, 
										  ulMessageId, 
										  MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
										  szBuffer, 
										  iSize,
										  NULL
										  );
					FreeLibrary(hDll);
				}
			}			
			return (bOk);
		}

		static void OutputDebugString(LPCTSTR fmt, ...)
		{
			TCHAR szBuffer[MAX_BUFFER_SIZE] = { 0 };
			va_list args;
			va_start(args, fmt);
			_vstprintf(szBuffer, fmt, args);
			va_end(args);
			::OutputDebugString(szBuffer);
		}

		static void OutputErrorMessage(
									   ULONG ulMessageId,
									   LPCTSTR lpszFile,
									   LPCTSTR lpszFunc,
									   INT iLine
									   )
		{				
			TCHAR szBuffer[MAX_BUFFER_SIZE] = { 0 };
			LookupDescription(ulMessageId, szBuffer, MAX_BUFFER_SIZE);
			OutputDebugString(TEXT("File: %s Func: %s Line: %d Code: 0x%08x Desc: %s\r\n"), 
							  lpszFile, lpszFunc, iLine, ulMessageId, szBuffer);
		}
	};

}

#endif // __CMN_LIB_TRACER_H__