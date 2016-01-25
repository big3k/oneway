#ifndef __SERVICE_TEMPLATE_H__
#define __SERVICE_TEMPLATE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include <WinSvc.h>

namespace CommonLib {

#define MAX_SERVICE_NAME_LEN	256
#define PATH_NAME_QUOTES_SPACE	2

template <typename T>
	class CServiceModuleT
	{
	public:
		// Typedefs

		enum OPERATION { START_SERVICE, INSTALL_SERVICE, UNINSTALL_SERVICE };		

	public:
		// Constructor / Destructor

		CServiceModuleT(LPCTSTR lpszServiceName)
			{
			CMN_LIB_VERIFY(lpszServiceName != NULL);

            s_pMyself = this;
			m_hServiceStatus = NULL;
			m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
			m_status.dwCurrentState = SERVICE_STOPPED;
			m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			m_status.dwWin32ExitCode = NOERROR;
			m_status.dwServiceSpecificExitCode = NOERROR;
			m_status.dwCheckPoint = 0;
			m_status.dwWaitHint = 0;			
			_tcscpy(m_szServiceName, lpszServiceName);
			}

	public:
		// External Operations

		HRESULT Start(OPERATION eOpcode)
			{
			HRESULT hr = E_INVALIDARG;
			T* pT = static_cast<T*>(this);
			switch (eOpcode)
				{
			case START_SERVICE:
				hr = pT->StartService();
				break;

			case INSTALL_SERVICE:
				hr = pT->InstallService();
				break;

			case UNINSTALL_SERVICE:
				hr = pT->UninstallService();
				break;
				}
			return (hr);
			}
		HRESULT IsServiceInstalled() const
			{
			HRESULT hr = S_FALSE;
			SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCM != NULL)
				{
				SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
				if (hService != NULL)
					{	
					hr = S_OK;
					::CloseServiceHandle(hService);					
					}
				::CloseServiceHandle(hSCM);
				}			
			return (hr);
			}
		HRESULT GetWin32ExitCode() const
			{
			return (m_status.dwWin32ExitCode);
			}
		HRESULT GetServiceSpecificExitCode() const
			{
			return (m_status.dwServiceSpecificExitCode);
			}
	
	protected:
		// Internal Overridables

		HRESULT StartService()
			{
			if (IsServiceInstalled() == S_FALSE)
				{
				return (E_FAIL);
				}
			SERVICE_TABLE_ENTRY serviceEntry[] =
				{
					{ m_szServiceName, _ServiceMain },
					{ NULL, NULL }
				};
        #ifdef __NODEBUG_SERVICE__
		
            if (!::StartServiceCtrlDispatcher(serviceEntry))
				{
				return (HRESULT_FROM_WIN32(GetLastError()));
				}

        #else 

            _ServiceMain(0, NULL);

        #endif // __NODEBUG_SERVICE__
			return (S_OK);
			}
		HRESULT InstallService()
			{
			if (IsServiceInstalled() == S_OK)
				{
				return (S_OK);
				}
			// Get the executable file path
			TCHAR szFilePath[MAX_PATH + PATH_NAME_QUOTES_SPACE];
			ULONG ulFLen = ::GetModuleFileName(NULL, szFilePath + 1, MAX_PATH);
			if (ulFLen == 0 || ulFLen == MAX_PATH)
				{
				return (HRESULT_FROM_WIN32(GetLastError()));
				}

			// Quote the FilePath before calling CreateService
			szFilePath[0] = _T('\"');
			szFilePath[ulFLen + 1] = _T('\"');
			szFilePath[ulFLen + 2] = 0;

			SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCM == NULL)
				{
				return (HRESULT_FROM_WIN32(GetLastError()));
				}
			SC_HANDLE hService = ::CreateService(
												 hSCM, 
												 m_szServiceName,	// Internal use
												 m_szServiceName,	// External use
												 SERVICE_ALL_ACCESS,
												 SERVICE_WIN32_OWN_PROCESS,
												 SERVICE_DEMAND_START,
												 SERVICE_ERROR_NORMAL,
												 szFilePath, 
												 NULL,	// The service does not belong to a group. 
												 NULL,	// Not changing the existing tag. 
												 NULL,	// Not dependency on a group
												 NULL,	// Uses the LocalSystem account
												 NULL
												 );
			if (hService == NULL)
				{
				::CloseServiceHandle(hSCM);
				return (HRESULT_FROM_WIN32(GetLastError()));
				}
			::CloseServiceHandle(hService);
			::CloseServiceHandle(hSCM);			
			return (S_OK);
			}
		HRESULT UninstallService()
			{
			if (IsServiceInstalled() == S_FALSE)
				{
				return (S_OK);
				}

			SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (hSCM == NULL)
				{
				return (HRESULT_FROM_WIN32(GetLastError()));
				}
			// Request call ControlService and DeleteService function
			SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);
			if (hService == NULL)
				{
				::CloseServiceHandle(hSCM);
				return (HRESULT_FROM_WIN32(GetLastError()));
				}
			SERVICE_STATUS status;
			if (!::ControlService(hService, SERVICE_CONTROL_STOP, &status))
				{
				ULONG ulError = GetLastError();
				if (!((ulError == ERROR_SERVICE_NOT_ACTIVE) || 
					  (ulError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL && status.dwCurrentState == SERVICE_STOP_PENDING)))
					{
					return (HRESULT_FROM_WIN32(ulError));
					}
				}

			HRESULT hr = ::DeleteService(hService) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
			::CloseServiceHandle(hService);
			::CloseServiceHandle(hSCM);
			return (hr);
			}
		void ServiceMain(ULONG ulArgc, LPTSTR* lpszArgv)
			{	
			CMN_LIB_UNUSED(ulArgc);
			CMN_LIB_UNUSED(lpszArgv);			
			
        #ifdef __NODEBUG_SERVICE__

            // Register the control request handler
            m_status.dwCurrentState  = SERVICE_START_PENDING;
            m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
            if (m_hServiceStatus == (SERVICE_STATUS_HANDLE)0)
            {	
                m_status.dwWin32ExitCode = GetLastError();
                return;
            }
            m_status.dwWin32ExitCode = NOERROR;
            m_status.dwCheckPoint	 = 0;
            m_status.dwWaitHint		 = 0;	
            
            SetServiceStatus(SERVICE_START_PENDING);

        #endif // __NODEBUG_SERVICE__

            T* pT = static_cast<T*>(this);
            if (pT->InitRun())
				{

            #ifdef __NODEBUG_SERVICE__
                SetServiceStatus(SERVICE_RUNNING);
            #endif // __NODEBUG_SERVICE__
				
				pT->Run();

            #ifdef __NODEBUG_SERVICE__
				SetServiceStatus(SERVICE_STOP_PENDING);
            #endif // __NODEBUG_SERVICE__

                pT->TermRun();
				}

        #ifdef __NODEBUG_SERVICE__
			SetServiceStatus(SERVICE_STOPPED);
        #endif // __NODEBUG_SERVICE
			}
		BOOL InitRun()
			{
			return (S_OK);
			}
		VOID Run()
			{			
			}
		VOID TermRun()
			{			
			}
		void Handler(ULONG ulOpcode)
			{
			T* pT = static_cast<T*>(this);
			switch (ulOpcode)
				{
				case SERVICE_CONTROL_STOP:
					pT->OnStop();
					break;

				case SERVICE_CONTROL_PAUSE:
					pT->OnPause();
					break;

				case SERVICE_CONTROL_CONTINUE:
					pT->OnContinue();
					break;

				case SERVICE_CONTROL_INTERROGATE:
					pT->OnInterrogate();
					break;

				case SERVICE_CONTROL_SHUTDOWN:
					pT->OnShutdown();
					break;

				default:
					pT->OnUnknownRequest(ulOpcode);
					break;
				}
			}
		void OnStop()
			{			
			}
		void OnPause()
			{
			}
		void OnContinue()
			{
			}
		void OnInterrogate()
			{
			}
		void OnShutdown()
			{
			}
		void OnUnknownRequest(ULONG ulOpcode)
			{
			CMN_LIB_UNUSED(ulOpcode);
			}

	protected:
		// Internal Operations

		static void WINAPI _ServiceMain(ULONG ulArgc, LPTSTR* lpszArgv)
			{
			static_cast<T*>(s_pMyself)->ServiceMain(ulArgc, lpszArgv);
			}
		static void WINAPI _Handler(ULONG ulOpcode)
			{
			static_cast<T*>(s_pMyself)->Handler(ulOpcode);
			}
		void SetServiceStatus(ULONG ulState)
			{
			m_status.dwCurrentState = ulState;
			::SetServiceStatus(m_hServiceStatus, &m_status);
			}
        void SetWin32ExitCode(ULONG lExitCode)
        {
            m_status.dwWin32ExitCode = lExitCode;
        }
        void SetServiceSpecificExitCode(ULONG lServiceSpecificExitCode)
        {
            m_status.dwServiceSpecificExitCode = lServiceSpecificExitCode;
        }

	protected:
		// Data Members

		static CServiceModuleT*	s_pMyself;

		SERVICE_STATUS_HANDLE	m_hServiceStatus;
		SERVICE_STATUS			m_status;		
		TCHAR					m_szServiceName[MAX_SERVICE_NAME_LEN];		
	};

	template <typename T>
		CServiceModuleT<T>* CServiceModuleT<T>::s_pMyself = NULL;

}

#endif // __SERVICE_TEMPLATE_H__