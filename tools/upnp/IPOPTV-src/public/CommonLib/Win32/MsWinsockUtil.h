#ifndef __MS_WINSOCK_UTILITY_H__
#define __MS_WINSOCK_UTILITY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include <mswsock.h>
#include "CommonDef.h"

///////////////////////////////////////////////////////////////////////////////

namespace CommonLib {

	class MsWinsockUtil
	{
	public:
		// Typedefs

		enum FunctionMask
		{
			LOAD_ACCEPT_EX				 = 0x00000001,
			LOAD_TRANSMIT_FILE			 = 0x00000002,
			LOAD_GET_ACCEPTEX_SOCK_ADDRS = 0x00000004,
			LOAD_DISCONNECT_EX			 = 0x00000008,
			LOAD_ALL					 = 0x0000000F
		};

	public:
		// Operations

		static HRESULT LoadExtensionFunction(SOCKET ActiveSocket, FunctionMask eValue);

		static BOOL AcceptEx(
							 SOCKET sListenSocket, 
							 SOCKET sAcceptSocket,
							 PVOID lpOutputBuffer, 
							 DWORD dwReceiveDataLength,
							 DWORD dwLocalAddressLength, 
							 DWORD dwRemoteAddressLength,
							 LPDWORD lpdwBytesReceived, 
							 LPOVERLAPPED lpOverlapped
							 )
		{
			CMN_LIB_VERIFY(lpfnAccepteEx != NULL);
			return (lpfnAccepteEx(
								  sListenSocket,
								  sAcceptSocket,
								  lpOutputBuffer,
								  dwReceiveDataLength,
								  dwLocalAddressLength,
								  dwRemoteAddressLength,
								  lpdwBytesReceived,
								  lpOverlapped
								  ));
		}

		static BOOL TransmitFile(
								 SOCKET hSocket,
								 HANDLE hFile,
								 DWORD nNumberOfBytesToWrite,
								 DWORD nNumberOfBytesPerSend,
								 LPOVERLAPPED lpOverlapped,
								 LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
								 DWORD dwFlags
								 )
		{
			CMN_LIB_VERIFY(lpfnTransmitFile != NULL);
			return (lpfnTransmitFile(
									 hSocket,
									 hFile,
									 nNumberOfBytesToWrite,
									 nNumberOfBytesPerSend,
									 lpOverlapped,
									 lpTransmitBuffers,
									 dwFlags
									 ));
		}

		static void GetAcceptExSockAddrs(
										 PVOID lpOutputBuffer,
										 DWORD dwReceiveDataLength,
										 DWORD dwLocalAddressLength,
										 DWORD dwRemoteAddressLength,
										 LPSOCKADDR* LocalSockaddr,
										 LPINT LocalSockaddrLength,
										 LPSOCKADDR* RemoteSockaddr,
										 LPINT RemoteSockaddrLength
										 )
		{
			CMN_LIB_VERIFY(lpfnGetAcceptExSockAddrs != NULL);
			lpfnGetAcceptExSockAddrs(
									 lpOutputBuffer,
									 dwReceiveDataLength,
									 dwLocalAddressLength,
									 dwRemoteAddressLength,
									 LocalSockaddr,
									 LocalSockaddrLength,
									 RemoteSockaddr,
									 RemoteSockaddrLength
									 );
		}

		static BOOL DisconnectEx(
								 SOCKET hSocket,
								 LPOVERLAPPED lpOverlapped,
								 DWORD dwFlags,
								 DWORD reserved
								 )
		{
			CMN_LIB_VERIFY(lpfnDisconnectEx != NULL);
			return (lpfnDisconnectEx(hSocket, lpOverlapped, dwFlags, reserved));
		}	

	private:
		// Internal Operations

		static BOOL GET_LOAD_FUNCTION(FunctionMask eValue, FunctionMask eMask)
		{
			return ((eValue & eMask) > 0);
		}
		static HRESULT LoadExtensionFunction(SOCKET ActiveSocket, GUID& FunctionID, void** ppFunc);

	private:		

		static LPFN_ACCEPTEX				lpfnAccepteEx;
		static LPFN_TRANSMITFILE			lpfnTransmitFile;
		static LPFN_GETACCEPTEXSOCKADDRS	lpfnGetAcceptExSockAddrs;
		static LPFN_DISCONNECTEX			lpfnDisconnectEx;
	};

}

#endif // __MS_WINSOCK_UTILITY_H__