#ifndef __CMN_LIB_LOG_TOOL_H__
#define __CMN_LIB_LOG_TOOL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include "Win32\AutoLocker.h"

namespace CommonLib {

	const TCHAR* const LOG_TYPE_STRING[] = 
		{
			_T("INFO"),
			_T("WARNING"),	
			_T("ERROR"),
            _T("UNDEFINED")
		};

	enum LOG_TYPE
		{
            LT_INFO,		    // 信息
            LT_WARNING,			// 警告
			LT_ERROR,			// 错误
			LT_UNDEFINED        
		};

		#define MAX_LOG_OUTPUT_LEN	4096L // 日志输出长度
		
	template <typename _AutoLocker>
		class CLogTool
		{
			// Typedefs

			typedef _AutoLocker						MyAutoLocker;
			typedef typename AutoLocker::LockTraits MyLocker;

		public:
			// Constructor / Destructor

			CLogTool()
				: m_hLogFile(INVALID_HANDLE_VALUE)
				{
				}
			CLogTool(LPCTSTR lpszLogFileName)
				{
				Open(lpszLogFileName);
				}
			~CLogTool()
				{
				Close();
				}

		public:
			// Operations

			HRESULT Open(LPCTSTR lpszLogFilePath)
			{			
				CMN_LIB_CHK_POINTER(lpszLogFilePath);

				AutoLocker locker(m_locker);
				Close();		
				m_hLogFile = CreateFile(
										lpszLogFilePath, 
										GENERIC_WRITE, 
										FILE_SHARE_READ,
										NULL, 
										CREATE_ALWAYS, 
										0, 
										NULL
										);
				if (m_hLogFile == INVALID_HANDLE_VALUE)
				{
					return (HRESULT_FROM_WIN32(GetLastError()));
				}				
				return (NOERROR);
			}
			HRESULT	Archive(
							LOG_TYPE eLT, 
							LPCTSTR lpszFile, 
							LPCTSTR lpszFunc,
							INT iLine, 
							LPCTSTR fmt,
							...
							)
			{
				AutoLocker locker(m_locker);
				if (m_hLogFile == INVALID_HANDLE_VALUE)
				{
					return (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE));
				}
                static TCHAR szOutputBuffer[MAX_LOG_OUTPUT_LEN] = { 0 };
                INT iPrefixLen = -1;
                INT iLogLen = -1;
				iPrefixLen = FormatPrefix(szOutputBuffer, eLT, lpszFile, lpszFunc, iLine);
				va_list args;
				va_start(args, fmt);
				iLogLen = _vstprintf(szOutputBuffer + iPrefixLen, fmt, args);
				va_end(args);
                szOutputBuffer[iPrefixLen + iLogLen] = 0;
                CMN_LIB_HR(WriteLog(m_hLogFile, szOutputBuffer));                
				return (NOERROR);
			}
            HRESULT Archive(LOG_TYPE eLT, LPCTSTR fmt, ...)
            {
                AutoLocker locker(m_locker);
                if (m_hLogFile == INVALID_HANDLE_VALUE)
                {
                    return (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE));
                }
                static TCHAR szOutputBuffer[MAX_LOG_OUTPUT_LEN] = { 0 };
                INT iPrefixLen = -1;
                INT iLogLen = -1;
                iPrefixLen = SimplePrefix(szOutputBuffer, eLT);
                va_list args;
                va_start(args, fmt);
                iLogLen = _vstprintf(szOutputBuffer + iPrefixLen, fmt, args);
                va_end(args);
                szOutputBuffer[iPrefixLen + iLogLen] = 0;
                CMN_LIB_HR(WriteLog(m_hLogFile, szOutputBuffer));                
                return (NOERROR);
            }
			HRESULT	Archive(LPCTSTR fmt, ...)
			{
				AutoLocker locker(m_locker);
				if (m_hLogFile == INVALID_HANDLE_VALUE)
				{
					return (HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE));
				}
                static TCHAR szOutputBuffer[MAX_LOG_OUTPUT_LEN] = { 0 };
                INT iLogLen = 0;
				va_list args;
				va_start(args, fmt);
				iLogLen = _vstprintf(szOutputBuffer, fmt, args);
				va_end(args);
                szOutputBuffer[iLogLen] = 0;
                CMN_LIB_HR(WriteLog(m_hLogFile, szOutputBuffer));                
				return (S_OK);
			}
			void Close()
			{
				AutoLocker locker(m_locker);
				if (m_hLogFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(m_hLogFile);
					m_hLogFile = INVALID_HANDLE_VALUE;
				}
			}
			static LPCTSTR GetLogTypeString(LOG_TYPE eLT)
			{
				LPCTSTR lpszSLevel = NULL;
				const int COUNT = CMN_LIB_ARRAY_SIZE(LOG_TYPE_STRING);	
				for (int i = LT_INFO; i < (COUNT - 1); ++i)
				{
					if (eLT == i)
					{
						lpszSLevel = LOG_TYPE_STRING[i];
						break;
					}
				}
				if (lpszSLevel == NULL)
				{
					lpszSLevel = LOG_TYPE_STRING[COUNT];
				}
				return (lpszSLevel);
			}

		protected:

            static INT SimplePrefix(LPTSTR lpszBuffer, LOG_TYPE eLT)
            {
                SYSTEMTIME sysTime;
                GetLocalTime(&sysTime);			
                INT iWritten = _stprintf(
                                        lpszBuffer, 
                                        TEXT("[LT: %s][%4d-%02d-%02d|%02d:%02d:%02d:%d]"), 
                                        GetLogTypeString(eLT),                                        
                                        sysTime.wYear, 
                                        sysTime.wMonth, 
                                        sysTime.wDay, 
                                        sysTime.wHour,
                                        sysTime.wMinute, 
                                        sysTime.wSecond,
                                        sysTime.wMilliseconds
                                        );
                return (iWritten);
            }
			static INT FormatPrefix(
									LPTSTR lpszBuffer,
									LOG_TYPE eLT, 
									LPCTSTR lpszFile, 
									LPCTSTR lpszFunc,
									int iLine
									)
			{			
				SYSTEMTIME sysTime;
				GetLocalTime(&sysTime);			
				INT iWritten = _stprintf(
						                 lpszBuffer, 
						                 TEXT("[LT: %s] [File: %s Func: %s Line: %d] [%4d-%02d-%02d|%02d:%02d:%02d:%d]"), 
						                 GetLogTypeString(eLT),
						                 lpszFile,
						                 lpszFunc,
						                 iLine,
						                 sysTime.wYear, 
						                 sysTime.wMonth, 
						                 sysTime.wDay, 
						                 sysTime.wHour,
						                 sysTime.wMinute, 
						                 sysTime.wSecond,
						                 sysTime.wMilliseconds
						                 );
                return (iWritten);
			}

			static HRESULT WriteLog(HANDLE hFile, LPCTSTR szBuffer)
			{
				DWORD dwWritten = 0;
				DWORD dwToWrite = (DWORD)_tcslen(szBuffer) * sizeof (TCHAR);			
				BOOL bOk = WriteFile(hFile, szBuffer, dwToWrite, &dwWritten, NULL);			
				if (!bOk ||	dwToWrite != dwWritten)
				{						
					return (HRESULT_FROM_WIN32(GetLastError()));
				}
				return (NOERROR);
			}

		protected:
			// Members

			HANDLE	 m_hLogFile;
			MyLocker m_locker;
		};	

		typedef CLogTool<AutoLockerST> CLogToolST;
		typedef CLogTool<AutoLockerMT> CLogToolMT;

		#ifdef CMN_LIB_USE_IN_MT
			typedef CLogToolMT CMN_LIB_LOG_TOOL;
		#else
			typedef CLogToolST CMN_LIB_LOG_TOOL;
		#endif

}

#endif // __CMN_LIB_LOG_TOOL_H__