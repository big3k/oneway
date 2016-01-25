#include "MsWinsockUtil.h"

namespace CommonLib {

	LPFN_ACCEPTEX				MsWinsockUtil::lpfnAccepteEx			= NULL;
	LPFN_TRANSMITFILE			MsWinsockUtil::lpfnTransmitFile			= NULL;
	LPFN_GETACCEPTEXSOCKADDRS	MsWinsockUtil::lpfnGetAcceptExSockAddrs	= NULL;
	LPFN_DISCONNECTEX			MsWinsockUtil::lpfnDisconnectEx			= NULL;

	HRESULT MsWinsockUtil::LoadExtensionFunction(
												 SOCKET ActiveSocket, 
												 FunctionMask eValue
												 )
	{		
		if (GET_LOAD_FUNCTION(eValue, LOAD_ACCEPT_EX))
		{
			if (lpfnAccepteEx == NULL)
			{
				// AcceptEx Function
				GUID GUID_AcceptEx = WSAID_ACCEPTEX;
				CMN_LIB_HR(LoadExtensionFunction(ActiveSocket, GUID_AcceptEx, (void**)&lpfnAccepteEx));
			}			
		}
		if (GET_LOAD_FUNCTION(eValue, LOAD_TRANSMIT_FILE))
		{
			if (lpfnTransmitFile == NULL)
			{
				// TransmitFile Function
				GUID GUID_TransmitFile = WSAID_TRANSMITFILE;
				CMN_LIB_HR(LoadExtensionFunction(ActiveSocket, GUID_TransmitFile, (void**)&lpfnTransmitFile));
			}			
		}
		if (GET_LOAD_FUNCTION(eValue, LOAD_GET_ACCEPTEX_SOCK_ADDRS))
		{
			if (lpfnGetAcceptExSockAddrs == NULL)
			{
				// GetAcceptExSockaddrs Function
				GUID GUID_GetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
				CMN_LIB_HR(LoadExtensionFunction(ActiveSocket, GUID_GetAcceptExSockaddrs, (void**)&lpfnGetAcceptExSockAddrs));
			}			
		}
		if (GET_LOAD_FUNCTION(eValue, LOAD_DISCONNECT_EX))
		{
			if (lpfnDisconnectEx == NULL)
			{
				// DisconnectEx Function
				GUID GUID_DisconnectEx = WSAID_DISCONNECTEX;
				CMN_LIB_HR(LoadExtensionFunction(ActiveSocket, GUID_DisconnectEx, (void**)&lpfnDisconnectEx));
			}			
		}
		return (NOERROR);
	}

	HRESULT MsWinsockUtil::LoadExtensionFunction(
												 SOCKET ActiveSocket, 
												 GUID& FunctionID, 
												 void** ppFunc
												 )
	{
		ULONG ulBytes = 0;
		int iResult = WSAIoctl(
							   ActiveSocket, 
							   SIO_GET_EXTENSION_FUNCTION_POINTER,
							   &FunctionID, 
							   sizeof (GUID),
							   ppFunc,
							   sizeof (void*),
							   &ulBytes,
							   0,
							   0
							   );
		if (iResult == SOCKET_ERROR)
		{
			return (HRESULT_FROM_WIN32(WSAGetLastError()));
		}
		return (NOERROR);
	}

}