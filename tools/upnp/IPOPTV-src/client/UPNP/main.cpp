#include "CommonLib.h"
#include "UPNP.h"
#include <iostream>
#include <Winerror.h>
using namespace std;

enum ACTION
{
	ADD_PORT_MAP,
	DEL_PORT_MAP
};

int _tmain()
{
    WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ACTION eAction;
	ULONG ulIP;
	USHORT usExternalPort;
	USHORT usInternalPort;
	PROTO_TYPE eProtoType;

	const INT MAX_BUF_SIZE = 256;
	char szInputBuffer[MAX_BUF_SIZE] = { 0 };
	cout << "Entry Action(Add or Del): ";
	cin  >> szInputBuffer;
	strlwr(szInputBuffer);
	if (strcmp(szInputBuffer, "add") == 0)
	{
		eAction = ADD_PORT_MAP;
	}
	else if (strcmp(szInputBuffer, "del") == 0)
	{
		eAction = DEL_PORT_MAP;
	}
	else
	{
		cout << "Invalid args!(Pass: add or del)" << endl;
		return (-1);
	}
	ZeroMemory(szInputBuffer, MAX_BUF_SIZE);

	if (eAction == ADD_PORT_MAP)
	{
		cout << "Enter IP Address: ";
		cin	 >> szInputBuffer;
		ulIP = inet_addr(szInputBuffer);

		ZeroMemory(szInputBuffer, MAX_BUF_SIZE);
	}

	cout << "Enter External Port: ";
	cin	 >> usExternalPort;
	
	if (eAction == ADD_PORT_MAP)
	{
		cout << "Enter Internal Port: ";
		cin  >> usInternalPort;
	}

	cout << "Protocol Type(UDP or TCP): ";
	cin  >> szInputBuffer;
	strlwr(szInputBuffer);
	if (strcmp(szInputBuffer, "udp") == 0)
	{
		eProtoType = PROTO_UDP;
	}
	else if (strcmp(szInputBuffer, "tcp") == 0)
	{
		eProtoType = PROTO_TCP;
	}
	else
	{
		cout << "Invalid args!(pass: udp or tcp)" << endl;
		return (-1);
	}

	if (eAction == ADD_PORT_MAP)
	{
		cout << "Enter Description Information: ";
		cin  >> szInputBuffer;		
	}
	
	if (eAction == ADD_PORT_MAP)
	{
		HRESULT hr = AddRandomPortMapping(ulIP, usExternalPort, usInternalPort, eProtoType, szInputBuffer);
		if (FAILED(hr))
		{
			_tprintf(_T("Add Port Map failed! Code: %08x\r\n"), hr);
		}
		else
		{			
			_tprintf(_T("Add Port Map successful!"));
		}
	}
	else
	{
		HRESULT hr = DelPortMapping(usExternalPort, eProtoType);
		if (FAILED(hr))
		{
			_tprintf(_T("Delete Port Map failed! Code: %08x\r\n"), hr);			
		}
		else
		{
			_tprintf(_T("Delete Port Map successful!"));			
		}
	}
	
	WSACleanup();

	return (0);
}