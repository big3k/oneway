#include "ThreadIsIOPending.h"
#include <tchar.h>

namespace CommonLib {

	CThreadIsIOPending::LPFN_NT_QUERY_INFO_THREAD 
			CThreadIsIOPending::NtQueryInformationThread = NULL;

	BOOL CThreadIsIOPending::IsPending(HANDLE hThread)
	{
		if (!Initialize())
		{
			CMN_LIB_VERIFY(FALSE);
			return (TRUE);
		}
		BOOL  bThreadIsIoPending = FALSE;
		ULONG ulReturnLength	 = 0;
		if (NtQueryInformationThread(
									 hThread, 
									 ThreadIsIoPending, 
									 (LPVOID)&bThreadIsIoPending, 
									 sizeof (BOOL),
									 &ulReturnLength
									 ) != NO_ERROR)
		{
			_tprintf(TEXT("NtQueryInformationThread Failed! Code: %08x\r\n"), GetLastError());
			return (TRUE);
		}
		return (bThreadIsIoPending);
	}

	BOOL CThreadIsIOPending::Initialize()
	{
		if (NtQueryInformationThread == NULL)
		{
			NtQueryInformationThread = (LPFN_NT_QUERY_INFO_THREAD)GetProcAddress(GetModuleHandle("ntdll"), "NtQueryInformationThread");
			CMN_LIB_ASSERT(NtQueryInformationThread != NULL);
			if (NtQueryInformationThread == NULL)
			{
				return (FALSE);
			}
		}	
		return (TRUE);
	}

}
