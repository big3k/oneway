#include <atlstr.h>
#include "CommonLib.h"
#include "UPNP.h"
#include <Iphlpapi.h>
#include <list>

typedef std::list<CString> GatewayList;

//#define __USE_LOG_TOOL__

#ifdef __USE_LOG_TOOL__
typedef CommonLib::CLogToolST MyLogTool;
MyLogTool g_logTool(_T(".\\upnp.log"));
#define	  SSDP_RESPONSE_FILE		_T(".\\SSDP_Response.log")
#define   SOAP_RESPONSE_FOR_DD_FILE _T(".\\SOAP_Response_for_Device_Description.log")
#define	  SOAP_RESPONSE_FOR_IC_FILE _T(".\\SOAP_Response_for_Invoke_Commond.log")
#define	  DEVICE_DESCRPTION_FILE	_T(".\\DeviceDescription.log")

#define FUNCTION_ENTER  g_logTool.Archive(CommonLib::SL_INFORMATION, _T(""), __CMN_LIB_FUNC__, __LINE__, TEXT("Entry."))
#define FUNCTION_LEAVE  g_logTool.Archive(CommonLib::SL_INFORMATION, _T(""), __CMN_LIB_FUNC__, __LINE__, TEXT("Leave."))
#define HR(_expression) { HRESULT hr = _expression; if (FAILED(hr)) { FUNCTION_LEAVE; return (hr); } }

void ArchiveDeviceDescription(const CString& szFileName, const CString& szDes)
{
	CFile af;
	if (af.Open(szFileName, CFile::modeCreate | CFile::modeWrite))
	{
		CArchive ar(&af, CArchive::store);
		ar << szDes;
		ar.Flush();
		ar.Close();
		af.Close();
	}
}

#define ARCHIVE_DEVICE_DESCRIPTION(file, des) ArchiveDeviceDescription(file, des)

#else

#define FUNCTION_ENTER  __noop
#define FUNCTION_LEAVE  __noop
#define HR(_expression) CMN_LIB_HR(_expression)

#define ARCHIVE_DEVICE_DESCRIPTION(file, des) __noop

#endif // __USE_LOG_TOOL__

#define UPNP_PORT_MAP0		_T("WANIPConnection")
#define UPNP_PORT_MAP1		_T("WANPPPConnection")
#define UPNPGETEXTERNALIP	_T("GetExternalIPAddress"),_T("NewExternalIPAddress")
#define UPNP_ADD_PORT_MAP	_T("AddPortMapping")
#define UPNP_DEL_PORT_MAP	_T("DeletePortMapping")

#define MAX_TMP_BUF_SIZE		1024
#define MAX_RCV_BUF_SIZE		10240

static const ULONG	UPNP_ADDR  = 0xFAFFFFEF;	// 239.255.255.250 
static const USHORT	UPNP_PORT  = 1900;			// 多播端口
static const LPTSTR URN_PREFIX = _T("urn:schemas-upnp-org:");

//
// Help Functions
//

const CString ConvertToString(int i)
{
	CString s;	
	s.Format(_T("%d"), i);
	return (s);
}

const CString ConstructToXMLNode(const CString& name, const CString& value)
{
	CString s;
	s.Format(_T("<%s>%s</%s>"), name.GetString(), value.GetString(), name.GetString());
	return (s);
}

const CString ConstructToXMLNode(const CString& name, int value)
{
	return (ConstructToXMLNode(name, ConvertToString(value)));	
}

const CString GetIPString(ULONG ulIP)
{	
	struct in_addr inAddr;
	inAddr.s_addr = ulIP;
	return (CString(inet_ntoa(inAddr)));
}

HRESULT GetAdapterInfo(GatewayList& listGwIp)   
{   
	FUNCTION_ENTER;

	PIP_ADAPTER_INFO   pAdapterInfo;   
	PIP_ADAPTER_INFO   pAdapter   =   NULL;   
	DWORD   dwRetVal   =   0;   

	pAdapterInfo   =   (IP_ADAPTER_INFO   *)   malloc(   sizeof(IP_ADAPTER_INFO)   );   
	unsigned   long   ulOutBufLen   =   sizeof(IP_ADAPTER_INFO);   

	//   Make   an   initial   call   to   GetAdaptersInfo   to   get   
	//   the   necessary   size   into   the   ulOutBufLen   variable   
	if   (GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen)   ==   ERROR_BUFFER_OVERFLOW)     
	{   
		free(pAdapterInfo);   
		pAdapterInfo   =   (IP_ADAPTER_INFO   *)   malloc   (ulOutBufLen);     
	}   

	if((dwRetVal   =   GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen))   ==   NO_ERROR)     
	{
		pAdapter   =   pAdapterInfo;   
		while   (pAdapter)     
		{   
			listGwIp.push_back( pAdapter->GatewayList.IpAddress.String );
			pAdapter   =   pAdapter->Next;   
		}   
	}   
	else     
	{   
		//TRACE("Call   to   GetAdaptersInfo   failed.\n");   
		return HRESULT_FROM_WIN32(WSAGetLastError());
	}   
	free(pAdapterInfo);
	FUNCTION_LEAVE;
	return NOERROR;   
}   

HRESULT SOAP_Request(
					 const CString& szAddress, 
					 USHORT usPort, 
					 const CString& szRequest,
					 CString& szResponse
					 )
{	
	FUNCTION_ENTER;

	CStringA szReqs(szRequest);
	CStringA szAddr(szAddress);

	SOCKADDR_IN saTarget;
	ZeroMemory(&saTarget, sizeof (SOCKADDR_IN));
	saTarget.sin_family		 = AF_INET;
	saTarget.sin_port		 = htons(usPort);
	saTarget.sin_addr.s_addr = inet_addr(szAddr.GetString());
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		FUNCTION_LEAVE;
		return (HRESULT_FROM_WIN32(WSAGetLastError()));
	}
	INT iRes = connect(s, (SOCKADDR*)&saTarget, sizeof (SOCKADDR_IN));
	if (iRes == SOCKET_ERROR)
	{	
		HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
		closesocket(s);
		FUNCTION_LEAVE;
		return (hr);
	}	
	iRes = send(s, szReqs.GetString(), szReqs.GetLength(), 0);
	if (iRes == SOCKET_ERROR)
	{
		HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
		closesocket(s);
		FUNCTION_LEAVE;
		return (hr);
	}	
	CHAR szBuffer[MAX_RCV_BUF_SIZE] = { 0 };

	INT iContentLen = 0; 
	INT iTotal = 0;
	while (1)
	{
		iRes = recv(s, szBuffer + iTotal, MAX_RCV_BUF_SIZE - iTotal, 0);		
		if (iRes == SOCKET_ERROR || iRes == 0)
		{
			HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
			closesocket(s);
			FUNCTION_LEAVE;
			return (hr);
		}
		iTotal += iRes;
		if (iTotal < 200)
		{
			continue;
		}
		else
		{
			if (iContentLen == 0)
			{
				CString szTemp = szBuffer;
				INT iFindPos = szTemp.Find(_T("CONTENT-LENGTH:"));
				if (iFindPos >= 0)
				{					
					CString szContentLen = szTemp.Tokenize(_T("\r\n"), iFindPos);										
					szContentLen.Trim();
					iFindPos = szContentLen.Find(_T(' '));
					szContentLen.Delete(0, iFindPos + 1);
					iContentLen = _ttoi(szContentLen.GetString());
				}
			}
		}
		if (iTotal >= iContentLen)
		{
			break;
		}
	}	
	closesocket(s);
	szResponse = szBuffer;	
	FUNCTION_LEAVE;
	return (NOERROR);
}

HRESULT SSDP_Request(SOCKET s, ULONG ulAddress, USHORT usPort, const CString& szRequest)
{
	FUNCTION_ENTER;

	CStringA szTemp(szRequest);

	SOCKADDR_IN saTarget;	
	ZeroMemory(&saTarget, sizeof (SOCKADDR_IN));
	saTarget.sin_family		 = AF_INET;
	saTarget.sin_port		 = htons(usPort);
	saTarget.sin_addr.s_addr = ulAddress;
	INT iRes = sendto(s, szTemp.GetString(), szTemp.GetLength(), 0, (SOCKADDR*)&saTarget, sizeof (SOCKADDR_IN));
	if (iRes == SOCKET_ERROR)
	{
		FUNCTION_LEAVE;
		return (HRESULT_FROM_WIN32(WSAGetLastError()));
	}
	FUNCTION_LEAVE;
	return (NOERROR);
}

const CString GetProperty(const CString& all, const CString& name)
{
	CString szBegTag = _T('<')  + name + _T('>');
	CString szEndTag = _T("</") + name + _T('>');
	CString szProperty;

	INT iBegPos = all.Find(szBegTag);
	if (iBegPos < 0)
	{
		return (CString());
	}
	INT iEndPos = all.Find(szEndTag);
	if (iBegPos >= iEndPos) 
	{
		return (CString());
	}
	return (all.Mid(iBegPos + szBegTag.GetLength(), iEndPos - iBegPos - szBegTag.GetLength()));
}

LPCTSTR ErrorToString(HRESULT hr)
{
	HRESULT hrtemp = MAKE_ERROR_CODE(0);

	LPCTSTR pszDes = NULL;
	switch (hr)
	{
	case E_NOT_IN_LAN:
		pszDes = _T("E_NOT_IN_LAN");
		break;

	case E_SEARCH_DEVICE:
		pszDes = _T("E_SEARCH_DEVICE");
		break;

	case E_INVALID_ACTION:
		pszDes = _T("E_INVALID_ACTION");
		break;

	case E_INVALID_ARGS:
		pszDes = _T("E_INVALID_ARGS");
		break;
		
	case E_ACTION_FAILED:
		pszDes = _T("E_ACTION_FAILED");
		break;

	case E_ARG_VALUE_INVALID:
		pszDes = _T("E_ARG_VALUE_INVALID");
		break;

	case E_STR_ARG_TOO_LONG:
		pszDes = _T("E_STR_ARG_TOO_LONG");
		break;

	case E_INVALID_CTRL_URL:
		pszDes = _T("E_INVALID_CTRL_URL");
		break;

	case E_CONFLICT_MAPPING:
		pszDes = _T("E_CONFLICT_MAPPING");
		break;

	default:
		pszDes = _T("E_UNKNOWN");
		break;
	}
	return (pszDes);
}

HRESULT IsResponseOK(const CString& szResponse)
{
	FUNCTION_ENTER;

	INT iPos = 0;
	CString szStatus = szResponse.Tokenize(_T("\r\n"), iPos);
	iPos = 0;
	szStatus.Tokenize(_T(" "), iPos);
	szStatus = szStatus.Tokenize(_T(" "), iPos);
	if (szStatus.IsEmpty() || szStatus[0] != '2') 
	{	
		const CString szErrorCode = GetProperty(szResponse, _T("errorCode"));
		INT iErrorCode = _ttoi(szErrorCode.GetString());
		FUNCTION_LEAVE;
		return (MAKE_ERROR_CODE(iErrorCode));
	}
	FUNCTION_LEAVE;
	return (S_OK);
}

const CString NGetAddressFromUrl(
								 const CString& szURL, 
								 CString& szPost, 
								 CString& szHost, 
								 USHORT& usPort
								 )
{
	FUNCTION_ENTER;

	CString s = szURL;

	szPost = szHost = _T("");	
	usPort  = 0;
	INT iPos = s.Find(_T("://"));
	if (iPos == 0)
	{
		return (CString());
	}
	s.Delete(0, iPos + 3);

	iPos = s.Find('/');
	if (iPos == 0) 
	{
		szHost = s;
		s = _T("");
	} 
	else 
	{
		szHost = s.Mid(0, iPos);
		s.Delete(0, iPos);
	}

	if (s.IsEmpty()) 
	{
		szPost = _T("");
	} 
	else 
	{
		szPost = s;
	}

	iPos = 0;
	CString szAddr = szHost.Tokenize(_T(":"), iPos);
	s = szHost.Tokenize(_T(":"), iPos);
	if (s.IsEmpty()) 
	{
		usPort = 80;
	} 
	else 
	{
		usPort = (USHORT)_tstoi(s);
	}
	FUNCTION_LEAVE;
	return (szAddr);
}

HRESULT GetDescriptionURL(
						  CString& szDescriptionURL, 
						  CString& szSearchTarget,
						  INT iVersion = 1
						  )
{
	FUNCTION_ENTER;
	if (iVersion <= 0)
	{
		iVersion = 1;
	}

	GatewayList GWList;
	HR(GetAdapterInfo(GWList));

	const INT TRY_COUNT		  = 10;
	const INT NUM_OF_SERVICES = 2;
	const LPSTR szTargets[][2]= 
	{
		{ UPNP_PORT_MAP1, _T("service") },
		{ UPNP_PORT_MAP0, _T("service") }		
	};

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	ULONG  ulOpt = 1;
	ioctlsocket(s, FIONBIO, &ulOpt);

	INT  iTry = 0;
	BOOL bOk  = FALSE;
	for (; iTry < TRY_COUNT && !bOk; ++iTry)
	{
		for (INT i = 0; i < NUM_OF_SERVICES; i++)
		{
			TCHAR szTarget[MAX_TMP_BUF_SIZE] = { 0 };
			_stprintf(szTarget, _T("%s%s:%s:%d"), URN_PREFIX, szTargets[i][1], szTargets[i][0], iVersion);
			TCHAR szRequest[MAX_TMP_BUF_SIZE] = { 0 };			
			_stprintf(szRequest, _T("M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: %d\r\nST: %s\r\n\r\n"),
					  1, szTarget);

			// 向多播地址发起请求
			if (FAILED(SSDP_Request(s, UPNP_ADDR, UPNP_PORT, szRequest)))
			{
				HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
				closesocket(s);
				FUNCTION_LEAVE;
				return (hr);
			}

			GatewayList::iterator itBeg = GWList.begin(),
								  itEnd = GWList.end();

			while (itBeg != itEnd)
			{
				// 向网关地址发起请求
				if (FAILED(SSDP_Request(s, inet_addr((*itBeg).GetString()), UPNP_PORT, szRequest)))
				{
					HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
					closesocket(s);
					return (hr);
				}
				++itBeg;
			}			
		}
		Sleep(10);
		CHAR szBuffer[MAX_RCV_BUF_SIZE] = { 0 };
		int iRes = recv(s, szBuffer, MAX_RCV_BUF_SIZE, 0);
		if (iRes == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				continue;
			}
			else
			{
				HRESULT hr = HRESULT_FROM_WIN32(WSAGetLastError());
				closesocket(s);
				FUNCTION_LEAVE;
				return (hr);
			}
		}
		
		ARCHIVE_DEVICE_DESCRIPTION(SSDP_RESPONSE_FILE, szBuffer);

		CString szResponse = szBuffer;		
		if (SUCCEEDED(IsResponseOK(szResponse)))
		{
			for (int i = 0; i < NUM_OF_SERVICES; i++) 
			{
				// 查找应答对应的服务
				TCHAR szTarget[MAX_TMP_BUF_SIZE] = { 0 };
				_stprintf(szTarget, _T("%s%s:%s:%d"), URN_PREFIX, szTargets[i][1], szTargets[i][0], iVersion);
				if (szResponse.Find(szTarget) >= 0) 
				{
					// 获取设备描述
					INT iFindPos = szResponse.Find(_T("LOCATION:"));
					if (iFindPos == -1)
					{
						closesocket(s);
						FUNCTION_LEAVE;
						return (E_FAIL);
					}					
					szDescriptionURL = szResponse.Tokenize(_T("\r\n"), iFindPos);
					if (szDescriptionURL.IsEmpty())
					{
						closesocket(s);
						FUNCTION_LEAVE;
						return (E_FAIL);
					}

					// 取得描述的URL
					szSearchTarget = szTarget;
					szDescriptionURL.Trim();
					iFindPos = szDescriptionURL.Find(_T(' '));
					szDescriptionURL.Delete(0, iFindPos + 1);					
					bOk = TRUE;
					break;
				}
			}			
		}		

	}

	if (iTry >= TRY_COUNT)
	{
		szDescriptionURL = _T("");
		szSearchTarget	 = _T("");
		closesocket(s);
		FUNCTION_LEAVE;
		return (E_SEARCH_DEVICE);
	}
	closesocket(s);
	FUNCTION_LEAVE;
	return (NOERROR);
}

HRESULT GetControlURL(
					  const CString& szDescriptionURL, 
					  const CString& szTarget,
					  CString& szControlURL
					  )
{
	FUNCTION_ENTER;
	CString szPost, szHost, szAddress;
	USHORT usPort = 0;
	szAddress = NGetAddressFromUrl(szDescriptionURL, szPost, szHost, usPort);
	if (szAddress.IsEmpty())
	{
		FUNCTION_LEAVE;
		return (E_FAIL);
	}
	// 请求设备描述的XML
	CString szRequest;
	szRequest.Append(_T("GET "));
	szRequest.Append(szPost);
	szRequest.Append(_T(" HTTP/1.1\r\nHOST: "));
	szRequest.Append(szHost);
	szRequest.Append(_T("\r\nACCEPT-LANGUAGE: en\r\n\r\n"));
	CString szResponse;
	HR(SOAP_Request(szAddress, usPort, szRequest, szResponse));
	ARCHIVE_DEVICE_DESCRIPTION(SOAP_RESPONSE_FOR_DD_FILE, szResponse);
	HR(IsResponseOK(szResponse));	

	CString szBaseURL = GetProperty(szResponse, _T("URLBase"));
	if(szBaseURL.IsEmpty()) 
	{		
		szBaseURL.Append(_T("http://"));
		szBaseURL.Append(szHost);
		szBaseURL.Append(_T("/"));
	}
	if(szBaseURL[szBaseURL.GetLength() - 1] != _T('/'))
	{
		szBaseURL.Append(_T("/"));
	}

	// 对描述表进行分析,获取控制点的URL
	CString szServiceType = _T("<serviceType>") + szTarget + _T("</serviceType>");
	CString szClone = szResponse;
	int iPos = szClone.Find(szServiceType);
	if (iPos >= 0) 
	{	
		szClone.Delete(0, iPos);
		szControlURL = GetProperty(szClone, _T("controlURL"));
		if (szControlURL.IsEmpty())
		{
			FUNCTION_LEAVE;
			return (E_FAIL);
		}	
		if (szControlURL[0] == '/') 
		{
			szControlURL = szBaseURL + szControlURL.Mid(1);		
		}		
	}
	FUNCTION_LEAVE;
	return (NOERROR);	
}

HRESULT InvokeCommand(const CString& szControlURL,
					  const CString& szTarget,
					  const CString& szCommonName, 
					  const CString& szArgs
					  )
{	
	FUNCTION_ENTER;

	CString szPost, szHost, szAddr;
	USHORT usPort = 0;
	szAddr = NGetAddressFromUrl(szControlURL, szPost, szHost, usPort);
	if (szAddr.IsEmpty())
	{
		FUNCTION_LEAVE;
		return (E_FAIL);
	}
	// 用SOAP语法构造调用命令
	CString szContext;
	CString szRequest;
	szContext.Append(_T("<?xml version=\"1.0\"?><s:Envelope\r\n    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    "));
	szContext.Append(_T("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n  <s:Body>\r\n    <u:"));
	szContext.Append(szCommonName);
	szContext.Append(_T(" xmlns:u=\""));
	szContext.Append(szTarget);
	szContext.Append(_T("\">\r\n"));
	szContext.Append(szArgs);
	szContext.Append(_T("    </u:"));
	szContext.Append(szCommonName);
	szContext.Append(_T(">\r\n  </s:Body>\r\n</s:Envelope>\r\n\r\n"));
	szRequest.Append(_T("POST "));
	szRequest.Append(szPost);
	szRequest.Append(_T(" HTTP/1.1\r\nHOST: "));
	szRequest.Append(szHost);
	szRequest.Append(_T("\r\nContent-Length: "));
	szRequest.Append(ConvertToString(CStringA(szContext).GetLength()));
	szRequest.Append(_T("\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \""));
	szRequest.Append(szTarget);
	szRequest.Append(_T("#"));
	szRequest.Append(szCommonName);
	szRequest.Append(_T("\"\r\n\r\n"));
	szRequest.Append(szContext);

	CString szResponse;	
	HR(SOAP_Request(szAddr, usPort, szRequest, szResponse));
	ARCHIVE_DEVICE_DESCRIPTION(SOAP_RESPONSE_FOR_IC_FILE, szResponse);
	HR(IsResponseOK(szResponse));
	FUNCTION_LEAVE;
	return (NOERROR);
}

BOOL IsInLAN(ULONG nIP)
{
	// filter LAN IP's
	// -------------------------------------------
	// 0.*
	// 10.0.0.0 - 10.255.255.255  class A
	// 172.16.0.0 - 172.31.255.255  class B
	// 192.168.0.0 - 192.168.255.255 class C

	BYTE nFirst = (BYTE)nIP;
	BYTE nSecond = (BYTE)(nIP >> 8);

	// check this 1st, because those LANs IPs are mostly spreaded
	if (nFirst == 192 && nSecond == 168) 
	{
		return (TRUE);
	}
	if (nFirst == 172 && nSecond >= 16 && nSecond <= 31)
	{
		return (TRUE);
	}
	if (nFirst == 0 || nFirst == 10)
	{
		return (TRUE);
	}
	return (FALSE); 
}

HRESULT AddPortMapping(
					   ULONG  ulLocalIP,
					   USHORT usExternalPort,
					   USHORT usInternalPort, 
					   PROTO_TYPE eProtoType, 					   
					   LPCTSTR pszDescription
					   )
{
	FUNCTION_ENTER;

	if ((eProtoType != PROTO_TCP && eProtoType != PROTO_UDP) || 
		usInternalPort == 0									 ||
		pszDescription == NULL)
	{
		FUNCTION_LEAVE;
		return (E_INVALIDARG);
	}

	if (!IsInLAN(ulLocalIP))
	{
		FUNCTION_LEAVE;
		return (E_NOT_IN_LAN);
	}

	if (usExternalPort == 0)
	{
		usExternalPort = usInternalPort;
	}

	CString szDescriptionURL;
	CString szSearchTarget;
	CString szControlURL;
	CString szArgs;

	HR(GetDescriptionURL(szDescriptionURL, szSearchTarget));	
	HR(GetControlURL(szDescriptionURL, szSearchTarget, szControlURL));

	// 构造命令参数
	szArgs.Append(ConstructToXMLNode(_T("NewRemoteHost"), _T("")));
	szArgs.Append(ConstructToXMLNode(_T("NewExternalPort"), usExternalPort));
	szArgs.Append(ConstructToXMLNode(_T("NewProtocol"), eProtoType == PROTO_UDP ? _T("UDP") : _T("TCP")));
	szArgs.Append(ConstructToXMLNode(_T("NewInternalPort"), usInternalPort));
	szArgs.Append(ConstructToXMLNode(_T("NewInternalClient"), GetIPString(ulLocalIP)));
	szArgs.Append(ConstructToXMLNode(_T("NewEnabled"), _T("1")));
	szArgs.Append(ConstructToXMLNode(_T("NewPortMappingDescription"), pszDescription));
	szArgs.Append(ConstructToXMLNode(_T("NewLeaseDuration"), 0));
	HR(InvokeCommand(szControlURL, szSearchTarget, UPNP_ADD_PORT_MAP, szArgs));

	FUNCTION_LEAVE;

	return (NOERROR);
}
HRESULT AddRandomExternalPortMapping(
                                    ULONG ulLocalIP, 
                                    USHORT& usExternalPort, 
                                    USHORT usInternalPort, 
                                    PROTO_TYPE eProtoType, 
                                    LPCTSTR pszDescription
                                    )
{
    FUNCTION_ENTER;

    HRESULT hr = NOERROR;
    while (1)
    {
        hr = AddPortMapping(ulLocalIP, usExternalPort, usInternalPort, eProtoType, pszDescription);
        if (SUCCEEDED(hr))
        {
            break;
        }
        else if (hr == E_CONFLICT_MAPPING)
        {
            srand(10000);
            USHORT usNewExtrPort = usExternalPort;            
            while (usExternalPort == usNewExtrPort)
            {
                usNewExtrPort = max(rand(), 4000);
            }
            usExternalPort = usNewExtrPort;
        }
        else
        {
            break;
        }
    }

    FUNCTION_LEAVE;
    return (hr);
}
HRESULT DelPortMapping(
					   USHORT usExternalPort, 
					   PROTO_TYPE eProtoType
					   )
{
	FUNCTION_ENTER;

	if ((eProtoType != PROTO_TCP && eProtoType != PROTO_UDP) || usExternalPort == 0)
	{
		return (E_INVALIDARG);
	}

	CString szDescriptionURL;
	CString szSearchTarget;
	CString szControlURL;
	CString szArgs;

	HR(GetDescriptionURL(szDescriptionURL, szSearchTarget));
	HR(GetControlURL(szDescriptionURL, szSearchTarget, szControlURL));
	
	// 构造命令参数
	szArgs.Append(ConstructToXMLNode(_T("NewRemoteHost"), _T("")));
	szArgs.Append(ConstructToXMLNode(_T("NewExternalPort"), ConvertToString(usExternalPort)));
	szArgs.Append(ConstructToXMLNode(_T("NewProtocol"), eProtoType == PROTO_UDP ? _T("UDP") : _T("TCP")));

	HR(InvokeCommand(szControlURL, szSearchTarget, UPNP_DEL_PORT_MAP, szArgs));

	FUNCTION_LEAVE;
	return (NOERROR);
}
