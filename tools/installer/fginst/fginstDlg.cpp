// fginstDlg.cpp : implementation file
//

#include "stdafx.h"
#include "fginst.h"
#include "fginstDlg.h"
#include "afxmt.h"
#include "cbuffer.h"
#include "md5.h"
#include "rc4.h"
#include "io.h"
#include "afxinet.h"
#define REGULAR_CONN_TIMEOUT 5000 

enum {
	URL_TYPE_WIKIFORTIO = 0,
	URL_TYPE_LIVE = 1,
};


#define WINDOWS_TITLE	"自由门下载器"
#define FILE_NAME_MID	"i"

extern CONFIG_OPTION option; 
extern unsigned char sPassword[17];
extern char *sLinksBuf;

int nTodayDay = 0;
char sFilePath[MAX_PATH] = {0};


SOCKET  SockConnect(SOCKADDR  *addr, int* err = NULL);
SOCKET  SockConnect(SOCKADDR  *addr, long timeout, int* err = NULL);

static int Connectex(SOCKET s, const struct sockaddr *name, long timeout)
{
    // As connect() but with timeout setting.
    int            rc = 0;
    ULONG          ulB;
    struct timeval Time;
    fd_set         FdSet;
	
    ulB = TRUE; // Set socket to non-blocking mode
    ioctlsocket(s, FIONBIO, &ulB);
	
    if (connect(s, name, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
        if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
            // now wait for the specified time
            FD_ZERO(&FdSet);
            FD_SET(s, &FdSet);
			
            Time.tv_sec  = timeout / 1000L;
            Time.tv_usec = (timeout % 1000) * 1000;
            rc = select(0, NULL, &FdSet, NULL, &Time);
        }
    }
	
    ulB = FALSE; // Restore socket to blocking mode
    ioctlsocket(s, FIONBIO, &ulB);
	
    return (rc > 0) ? 0 : SOCKET_ERROR;
}

bool SockGetAddr(LPCSTR host, INT port, SOCKADDR_IN& sin)
{
	IN_ADDR		iaHost;
	LPHOSTENT	lpHostEntry = NULL;
	
	iaHost.s_addr = inet_addr(host);
	if (iaHost.s_addr == INADDR_NONE)
	{
		// Wasn't an IP address string, assume it is a name
		lpHostEntry = gethostbyname(host);
		if(lpHostEntry == NULL)
			return false;
		sin.sin_addr = * ((LPIN_ADDR)*lpHostEntry->h_addr_list);
	} 
	else
		sin.sin_addr = iaHost;
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons((unsigned short)port);
	
	return true;
}

SOCKET SockConnect(SOCKADDR *addr, int* err)
{	
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return INVALID_SOCKET;
	
    if (connect(s, addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		closesocket(s);
		s = INVALID_SOCKET;
		if (err) {
			*err = WSAGetLastError();
		}
	}
	
	return s;
}

SOCKET SockConnect(SOCKADDR *addr, long timeout, int* err)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return INVALID_SOCKET;

    if (Connectex(s, addr, timeout) == SOCKET_ERROR)
	{
        closesocket(s);
		s = INVALID_SOCKET;
		if (err) {
			*err = WSAGetLastError();
		}
    }
	
	return s;
}

bool SockReadable(SOCKET& s, long timeout)
{
    fd_set rfd;
	
    FD_ZERO(&rfd);
    FD_SET(s, &rfd);
	
	struct timeval time;
	time.tv_sec  = timeout / 1000L;
	time.tv_usec = (timeout % 1000) * 1000;
	
    int ret = select(0, &rfd, NULL, NULL, &time);
    if (ret < 0)
	{
	//	LOG_ERR("SockReadable: err=%d ",WSAGetLastError());
		closesocket(s);
		s = INVALID_SOCKET;
    }
	
    return (ret > 0);
}

bool SockSend(SOCKET& s, const char* data, int length, int& err)
{
	int nRet = 0, sofar = 0, left = 0;
	while ((left = length - sofar) > 0)
	{
		nRet = send(s, data+sofar, left, 0);
		if (nRet < 0)
		{
			//Error
			closesocket(s);
			s = INVALID_SOCKET;
			err = WSAGetLastError();
			return false;
		}
		sofar += nRet;
	}
	
	return true;
}

#define USER_AGENT			"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)"
#define WIKI_HTTP_GET		"GET %s HTTP/1.1\r\nAccept:  */*\r\nAccept-Language: zh-cn\r\nAccept-Encoding: gzip, deflate\r\nUser-Agent: %s\r\nHost: %s\r\nConnection: Keep-Alive\r\n\r\n"
#define WIKI_HTTP_POST		"POST %s HTTP/1.1\r\nAccept:  */*\r\nReferer: %s\r\nAccept-Language: zh-cn\r\nContent-Type: application/x-www-form-urlencoded\r\nAccept-Encoding: gzip, deflate\r\nUser-Agent: %s\r\nHost: %s\r\nContent-Length: %d\r\nConnection: Keep-Alive\r\nCache-Control: no-cache\r\nCookie: %s\r\n\r\n%s"
#define WIKI_POST_STRING	"act=download&fid=%d&fileName=%s"
#define LIVE_HTTP_GET		"GET %s HTTP/1.1\r\nAccept: */*\r\nUser-Agent: %s\r\nHost: %s\r\n\r\n"

BOOL DownloadUrl_live(LPCSTR host, USHORT port, LPCSTR objPath, CString csName, CString &csNewURL)
{
	SOCKADDR_IN sin;
	SOCKET sock = INVALID_SOCKET;
	CString csHead;
	CBuffer reqBuf;
	int err;
	char bBuf[MAX_BUFFER_SIZE + 1] = {0};
	int cbBuf = MAX_BUFFER_SIZE;
	int cbRead = 0;
	char *s = NULL;
	BOOL re = false;
	CString url;
	CString csContent = "";

	csNewURL = "";
	SockGetAddr(host, port, sin);
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {
		csHead.Format(LIVE_HTTP_GET,
			objPath, USER_AGENT, host);
		SockSend(sock, csHead, csHead.GetLength(), err);
		
		if (SockReadable(sock, REGULAR_CONN_TIMEOUT)) {
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "HTTP/1.1 200")  && strstr(bBuf, "\r\n\r\n")){// && strstr(bBuf, "downloadUrl:")){
					s = strstr(bBuf, "\r\n\r\n"); 
					csContent = s;
					
					CString str;
					str.Format("%s\\x3fdownload\\x26psid\\x3d1', downloadUrl: '", csName);
					while(SockReadable(sock, REGULAR_CONN_TIMEOUT)){
						if(cbRead = recv(sock, bBuf, cbBuf, 0), cbRead > 0) {
							bBuf[cbRead] = 0x00;
							csContent += bBuf;
							if(csContent.Find(str) != -1){
								url = csContent.Mid(csContent.Find(str) + str.GetLength());
								if((url.Find("http") != -1) && (url.Find("\\x3fdownload") != -1)){
									//MessageBox(0,url, csName, 0);
									break;
								}
							}
						}
					}

					if(csContent.Find(str) != -1){
						url = csContent.Mid(csContent.Find(str) + str.GetLength());
						url = url.Mid(url.Find("http"));
						url = url.Left(url.Find("\\x3fdownload"));
						url.Replace("\\x3a", ":");
						url.Replace("\\x2f", "/");
						csNewURL = url;
						re = true;
					}
				}
			}
		}
		closesocket(sock);
	}
	
	return re;
}

BOOL DownloadUrl_wiki(LPCSTR host, USHORT port, LPCSTR objPath, LPCTSTR objName, CString &csNewURL, CString &csCookie)
{
	SOCKADDR_IN sin;
	SOCKET sock = INVALID_SOCKET;
	CString csHead, tmp, csPostStr;
	CBuffer reqBuf, respBuf;
	int err;
	char bBuf[MAX_BUFFER_SIZE] = {0};
	int cbBuf = MAX_BUFFER_SIZE;
	int cbRead = 0;
	char *s = NULL;
	BOOL re = false;
	
	csCookie = "";
	csNewURL = "";
	SockGetAddr(host, port, sin);
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {
		csHead.Format(WIKI_HTTP_GET,
			objPath, USER_AGENT, host);
		SockSend(sock, csHead, csHead.GetLength(), err);
		
		if (SockReadable(sock, REGULAR_CONN_TIMEOUT)) {
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "HTTP/1.1 200")  && strstr(bBuf, "\r\n\r\n")){
					s = strstr(bBuf, "Set-Cookie: ");
					char *sEnd = strstr(s, ";");
					if(sEnd) {
						csCookie = CString(s + 12, sEnd - s -12);
					} else {
						sEnd = strstr(s, "\r\n");
						csCookie = CString(s + 12, sEnd - s - 12); 
					}
				}
			}
		}
		closesocket(sock);
	}

	if (csCookie == ""){
		goto Error;
	}
		
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {
		CString csRef;
		unsigned long nID = 0;
		sscanf(objPath, "/%d", &nID);
		csPostStr.Format(WIKI_POST_STRING, nID, objName);
		int nActLen = csPostStr.GetLength();

		csRef.Format("http://%s%s", host, objPath);
		csHead.Format(WIKI_HTTP_POST,
			objPath, csRef, USER_AGENT, host, nActLen, csCookie, csPostStr);
		SockSend(sock, csHead, csHead.GetLength(), err);
		
		csCookie += "; ";
		if (SockReadable(sock, REGULAR_CONN_TIMEOUT)) {
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "HTTP/1.1 ")  && strstr(bBuf, "\r\n\r\n")){
					
					s = strstr(bBuf, "Set-Cookie: ");
					char *sEnd = strstr(s, ";");
					if(sEnd) {
						csCookie += CString(s + 12, sEnd - s -12);
					} else {
						sEnd = strstr(s, "\r\n");
						csCookie += CString(s + 12, sEnd - s - 12);
					}
					
					s = strstr(bBuf, "Location: ");
					sEnd = strstr(s, "\r\n");
					csNewURL = CString(s + 10, sEnd - s -10);
					re =  true;
				}
			}
		}
		closesocket(sock);
	}

Error:

	return re;

}


#define HTTP_RANGE1		"Range: bytes=%d-%d\r\n"
#define HTTP_RANGE2		"Range: bytes=%d-\r\n"
#define HTTP_HEADER		"GET %s HTTP/1.1\r\nReferer: %s\r\nAccept: */*\r\n%sUser-Agent: %s\r\nHost: %s\r\nConnection: Keep-Alive\r\nCache-Control: no-cache\r\n%s%s\r\n"

//自动判断几个线程下载文件大小标志
#define MINBLOCKSIZE	100000
//控制最多几个线程同时下载
#define MAXBLOCKNUMBER	5
#define MAX_BUFFER_SIZE 8192

typedef struct{
	int nFileLength;
	int nBlockCount;
	int nStartPos[MAXBLOCKNUMBER];
	int nEndPos[MAXBLOCKNUMBER];
	int nBlockLength[MAXBLOCKNUMBER];
	int nFinishedLength[MAXBLOCKNUMBER]; //make sure, this var should be the last var in DownloadRangeFileInfo since I will check the pos
}DownloadRangeFileInfoStruct;


typedef struct{
	char host[30];
	int  port;
	char obj[150];
	FILE *filepointer;
	bool bFinished[MAXBLOCKNUMBER];
	CCriticalSection DownloadFileCS;

	DownloadRangeFileInfoStruct DownloadRangeFileInfo;

	int nBlockIndex;  //it is diff in each thread
	CString cookie;
}DownloadRangeStruct;


int nThreadCount = 0;
#define MAX_RETRY_TIMES		50

//获取下载没有完成的文件的下载信息
bool GetDownloadHeader(char *filename, int orgsize, DownloadRangeFileInfoStruct &fileInfo)
{
	FILE *fp=fopen(filename, "rb");
	if(!fp)
		return false;

	fseek(fp, 0L, 2);
	unsigned int filesize = ftell(fp);

	//since at least one block, so at least one DownloadRangeFileHead here
	if(filesize != orgsize + sizeof(fileInfo)){
		fclose(fp);
		return false;
	}

	fseek(fp, orgsize, 0);
	fread((char*)&fileInfo, 1, sizeof(fileInfo), fp);
	fclose(fp);

	//记录的大小与现在大小不符,或者块数不符
	if(fileInfo.nFileLength != orgsize || fileInfo.nBlockCount > MAXBLOCKNUMBER)
		return false;

	return true;
}

bool UpdateDownloadFileInfo(char *filename, int orgsize, DownloadRangeFileInfoStruct &fileInfo)
{
	FILE *fp = fopen(filename, "r+b");
	if(!fp)
		return false;
	
	fseek(fp, orgsize, 0);
	if(fwrite((char*)&fileInfo, 1, sizeof(fileInfo), fp) == sizeof(fileInfo)){
		fclose(fp);
		return true;
	} else{
		fclose(fp);
		return false;
	}
}

bool UpdateFinishedInfo(FILE *fp, int orgsize, int nBlockIndex, int *nNewFinishedLength)
{
	fseek(fp, orgsize + sizeof(int) * (1 + 1 + 3 * MAXBLOCKNUMBER +  nBlockIndex) , 0);
	bool re = (fwrite((char*)nNewFinishedLength, 1, sizeof(int), fp) == sizeof(int));
	return re;
}


UINT _DownloadUrlRange(LPVOID p)
{
	DownloadRangeStruct *pDownloadRange = (DownloadRangeStruct *)(long)p;
	int nBlockIndex = pDownloadRange->nBlockIndex;
	if(nBlockIndex < 0 || nBlockIndex >= MAXBLOCKNUMBER){
		//error
		return 0;
	}

	int nDownloadTimes = 0;

	SOCKADDR_IN sin;
	SOCKET sock = INVALID_SOCKET;
	CString csHead, tmp;
	CBuffer reqBuf, respBuf;
	int err;//, len = 0;
	char bBuf[MAX_BUFFER_SIZE] = {0};
	int cbBuf = MAX_BUFFER_SIZE;
	int cbRead = 0;
	int nFPPos = 0;

	SockGetAddr(pDownloadRange->host, pDownloadRange->port, sin);

	pDownloadRange->bFinished[nBlockIndex] = FALSE;
startDownload:
	nDownloadTimes ++;

	if(pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex] >= 
		pDownloadRange->DownloadRangeFileInfo.nBlockLength[nBlockIndex]){
		//finish this block
		pDownloadRange->bFinished[nBlockIndex] = TRUE;
		return 1;
	}

	//not finished, ...
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {

		nFPPos = pDownloadRange->DownloadRangeFileInfo.nStartPos[nBlockIndex]
			+ pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex];

		if(pDownloadRange->DownloadRangeFileInfo.nEndPos[nBlockIndex] != 0){
			tmp.Format(HTTP_RANGE1, nFPPos, pDownloadRange->DownloadRangeFileInfo.nEndPos[nBlockIndex]);
		} else {
			tmp.Format(HTTP_RANGE2, nFPPos);
		}

		if(pDownloadRange->cookie != ""){
			csHead.Format(HTTP_HEADER, pDownloadRange->obj, pDownloadRange->host, tmp, USER_AGENT, pDownloadRange->host, "Cookie: ", pDownloadRange->cookie);
		} else {
			csHead.Format(HTTP_HEADER, pDownloadRange->obj, pDownloadRange->host, tmp, USER_AGENT, pDownloadRange->host, "", "");
		}

		SockSend(sock, csHead, csHead.GetLength(), err);

		if (SockReadable(sock, REGULAR_CONN_TIMEOUT)) {
			//get the 1st recv buf
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "HTTP/1.1 206")  && strstr(bBuf, "\r\n\r\n")){

					pDownloadRange->DownloadFileCS.Lock();
					fseek(pDownloadRange->filepointer, nFPPos, 0);
					char *s = strstr(bBuf, "\r\n\r\n");
					int nHeaderLen = s - bBuf + 4;
					fwrite(s + 4,   1,  cbRead - nHeaderLen, pDownloadRange->filepointer);

					nFPPos += cbRead - nHeaderLen;
					pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex] += cbRead - nHeaderLen;
					UpdateFinishedInfo(pDownloadRange->filepointer, pDownloadRange->DownloadRangeFileInfo.nFileLength, nBlockIndex, &pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex]);
					
					fflush(pDownloadRange->filepointer);
					pDownloadRange->DownloadFileCS.Unlock();

					memset(bBuf, 0, cbBuf);

					while(cbRead = recv(sock, bBuf, cbBuf, 0), cbRead > 0) {
						pDownloadRange->DownloadFileCS.Lock();
						fseek(pDownloadRange->filepointer, nFPPos, 0);
						fwrite(bBuf,   1,  cbRead, pDownloadRange->filepointer);

						nFPPos += cbRead;
						pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex] += cbRead;
						UpdateFinishedInfo(pDownloadRange->filepointer, pDownloadRange->DownloadRangeFileInfo.nFileLength, nBlockIndex, &pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex]);
					
						fflush(pDownloadRange->filepointer);
						pDownloadRange->DownloadFileCS.Unlock();

						Sleep(1);
						memset(bBuf, 0, cbBuf);

						if(pDownloadRange->DownloadRangeFileInfo.nFinishedLength[nBlockIndex] >= pDownloadRange->DownloadRangeFileInfo.nBlockLength[nBlockIndex]){
							break;
						}
					}
				}
			}

		}
		closesocket(sock);
	} else{
		Sleep(500);
	}


	if(nDownloadTimes  < MAX_RETRY_TIMES){
		Sleep(200);
		goto startDownload;
	}

	pDownloadRange->bFinished[nBlockIndex] = TRUE;
	return 0;
}

bool GetUrlFileSize(LPCSTR host, USHORT port, LPCSTR obj, unsigned long &FileSize, CString cookie)
{
	SOCKADDR_IN sin;
	SOCKET sock = INVALID_SOCKET;
	CString csHead, tmp;
	int err;
	char bBuf[MAX_BUFFER_SIZE] = {0};
	int cbBuf = MAX_BUFFER_SIZE;
	int cbRead = 0;
	bool result = false;
	
	SockGetAddr(host, port, sin);
	
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {
		
		tmp.Format(HTTP_RANGE1, 0, 1); //only first 1 byte for test
		if (cookie != ""){
			csHead.Format(HTTP_HEADER, obj, host, tmp, USER_AGENT, host, "Cookie: ", cookie);
		} else {
			csHead.Format(HTTP_HEADER, obj, host, tmp, USER_AGENT, host, "", "");
		}
		
		SockSend(sock, csHead, csHead.GetLength(), err);
		
		if (SockReadable(sock, REGULAR_CONN_TIMEOUT * 2)) {
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "HTTP/1.1 206")  && strstr(bBuf, "\r\n\r\n")){
					char *p = strstr(bBuf, "Content-Range: bytes ");
					if (p) {
						p += strlen("Content-Range: bytes 0-1/");
						sscanf(p, "%d", &FileSize);
						result = true;
					}
				} else {
					//LOG_ERR("GetUrlFileSize Error:  obj=%s recvsize=%d.",  obj, cbRead);
				}
			} else {
				//LOG_ERR("GetUrlFileSize recv Error:  obj=%s",  obj);
			}			
		}
		closesocket(sock);
	} else {
		//LOG_ERR("GetUrlFileSize sockconnect error.");
	}
	
	return result;
}

DownloadRangeStruct DownloadRange;
bool MDownloadUrl(LPCSTR host, USHORT port, LPCSTR objPath, unsigned long& fsize,  const char *saveToFile, int nSetBlocks, CString cookie)
{
	int i;
	int nBlocks = nSetBlocks; //if 0, not set
	char sTempFile[MAX_PATH];
	strcpy(sTempFile, saveToFile);
	strcat(sTempFile, ".part");

//If I do not know fsize, that is if it is 0, I should get it first
	if(fsize == 0){
		if(!GetUrlFileSize(host, port, objPath, fsize, cookie)){
			return false;
		}
		if(fsize == 0){
			return false;
		}
	}	
	
	memset((char*)&DownloadRange.DownloadRangeFileInfo, 0, sizeof(DownloadRange.DownloadRangeFileInfo));
	strcpy(DownloadRange.host, host);
	DownloadRange.port = port;
	strcpy(DownloadRange.obj, objPath);

	if(!GetDownloadHeader(sTempFile, fsize, DownloadRange.DownloadRangeFileInfo)){
		DeleteFile(sTempFile);
		CFile f(sTempFile, CFile::typeBinary|CFile::modeReadWrite|CFile::modeCreate);
		f.SetLength(fsize + sizeof(DownloadRangeFileInfoStruct));
		f.Close();

		//if not set a value
		if(nBlocks == 0){
			nBlocks = fsize / MINBLOCKSIZE + 1;
		}
		if(nBlocks > MAXBLOCKNUMBER){
			nBlocks = MAXBLOCKNUMBER;
		}

		int nBlockSize = fsize / nBlocks;


		DownloadRange.DownloadRangeFileInfo.nFileLength = fsize;
		DownloadRange.DownloadRangeFileInfo.nBlockCount = nBlocks;
		for(i = 0; i<nBlocks; i++){
			DownloadRange.DownloadRangeFileInfo.nStartPos[i] = i * nBlockSize;
			DownloadRange.DownloadRangeFileInfo.nEndPos[i] = (i + 1) * nBlockSize -1;
			DownloadRange.DownloadRangeFileInfo.nBlockLength[i] = nBlockSize;
			DownloadRange.DownloadRangeFileInfo.nFinishedLength[i] = 0;

			//the last block
			if(i == nBlocks - 1){
				DownloadRange.DownloadRangeFileInfo.nEndPos[i] = 0;
				DownloadRange.DownloadRangeFileInfo.nBlockLength[i] = fsize - i * nBlockSize;
			}
		}

		if(!UpdateDownloadFileInfo(sTempFile, fsize, DownloadRange.DownloadRangeFileInfo)){
			return false;
		}
	} else {
		//use the old one, can not use a new one
		nBlocks = DownloadRange.DownloadRangeFileInfo.nBlockCount;
	}

	DownloadRange.filepointer = fopen(sTempFile, "r+b");
	if(!DownloadRange.filepointer){
		//error
		return false;
	}

	if(cookie != ""){
		DownloadRange.cookie = cookie;
	}

	//now, I have the DownloadRange, 
//	nThreadCount = 0;
	for(i=0; i<nBlocks; i++){
		DownloadRange.bFinished[i] = TRUE;
		DownloadRange.nBlockIndex = i;
		AfxBeginThread(_DownloadUrlRange, (LPVOID)&DownloadRange);
		Sleep(200);
	}

	//wait for all thread finished
	while(1){
		int nAllFinished = 0;
		for(i=0; i<nBlocks; i++){
			if(DownloadRange.bFinished[i]){
				nAllFinished ++;
			}
		}

		if(nAllFinished == nBlocks){
			break;
		} else {
			Sleep(100);
		}
	}

	fclose(DownloadRange.filepointer); 

	BOOL bError = FALSE;
	CFileStatus status;

	//check for error
	for(i=0; i<nBlocks; i++){
		if(DownloadRange.DownloadRangeFileInfo.nFinishedLength[i] != DownloadRange.DownloadRangeFileInfo.nBlockLength[i]
			|| !CFile::GetStatus(sTempFile, status) || (unsigned int)status.m_size != DownloadRange.DownloadRangeFileInfo.nFileLength + sizeof(DownloadRangeFileInfoStruct)){
			bError = TRUE;
			break;
		}
	}

	if(!bError){

		//remove the info data
		CFile f(sTempFile, CFile::typeBinary|CFile::modeReadWrite);
		f.SetLength(DownloadRange.DownloadRangeFileInfo.nFileLength);
		f.Close();


		//Check MD5
		char digest[33];
		Md5File (sTempFile , digest);
		if (memcmp(digest, option.file_hash, 32) == 0){
			DeleteFile(saveToFile);
			MoveFileEx(sTempFile, saveToFile, false);
		} else {
			DeleteFile(sTempFile);
			bError = TRUE;
		}
	}

	return !bError;
}

BOOL GetNetworkTime()
{
	SOCKADDR_IN sin;
	SOCKET sock = INVALID_SOCKET;
	CString csHead, tmp;
	CBuffer reqBuf, respBuf;
	int err;//, len = 0;
	char bBuf[MAX_BUFFER_SIZE] = {0};
	int cbBuf = MAX_BUFFER_SIZE;
	int cbRead = 0;
	BOOL re = false;
//nTodayDay=10;return TRUE;
	SockGetAddr(option.time_server, 80, sin);
	sock = INVALID_SOCKET;
	sock = SockConnect((LPSOCKADDR)&sin, REGULAR_CONN_TIMEOUT);
	if (sock != INVALID_SOCKET) {
		csHead.Format(HTTP_HEADER, "/", option.time_server, tmp, USER_AGENT, option.time_server, "", "");
		SockSend(sock, csHead, csHead.GetLength(), err);
		
		if (SockReadable(sock, REGULAR_CONN_TIMEOUT * 2)) {
			//get the 1st recv buf
			memset(bBuf, 0, cbBuf);
			cbRead = recv(sock, bBuf, cbBuf, 0);
			if(cbRead > 0) {
				if(strstr(bBuf, "\r\n\r\n")){
					if(strstr(bBuf, "Date:")) {
						char *sDate = strstr(bBuf, "Date: ") + 10;
						if(sDate) {
							sscanf(sDate, "%d", &nTodayDay);
							nTodayDay = nTodayDay % option.link_count;
							re = true;
						}
					}
				}
			}
			
		}
		closesocket(sock);
	} 

	return re;
}

CString GetLink(int nIndex)
{
 	CString str = "";
	rc4_key key;
	sPassword[16] = (unsigned char)(nIndex * 5);
	prepare_key(sPassword, 17, &key);

	char *sBuf = (char*)malloc(option.link_length);
	memcpy(sBuf, sLinksBuf + nIndex * option.link_length, option.link_length);
	rc4((unsigned char *)sBuf, option.link_length, &key);
	memset(&key, 0, sizeof(rc4_key ));
	str = sBuf;
	free(sBuf);
	return str;
}

int GetURLType(CString csURL)
{
	CString url = csURL;
	url.MakeLower();
	if(url.Find("wikifortio.com") != -1){
		return 	URL_TYPE_WIKIFORTIO;
	} else if(url.Find("live.com") != -1){
		return	URL_TYPE_LIVE;
	}
	else {
		return -1;
	}
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFginstDlg dialog

CFginstDlg::CFginstDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFginstDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFginstDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFginstDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFginstDlg)
	DDX_Control(pDX, IDC_PROGRESS1, m_Progress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFginstDlg, CDialog)
	//{{AFX_MSG_MAP(CFginstDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, OnButtonPause)
	ON_WM_TIMER()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFginstDlg message handlers
BOOL CFginstDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWindowText(WINDOWS_TITLE);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetTimer(1, 1000, NULL);
	m_Progress.SetRange(0,100);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFginstDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFginstDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CFginstDlg::OnOK() 
{

}

void CFginstDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CFginstDlg::OnButtonPause() 
{
	// TODO: Add your control notification handler code here
	
}

BOOL bRunning = false;
UINT _TestAllDownloadThread(LPVOID p)
{
	bRunning = true;
	CFginstDlg *pDlg = (CFginstDlg*)p;
	CString csLink, csHost, csObj, csFile, csName;
	unsigned short nPort = 80;
	TCHAR szPath[MAX_PATH];
	char sRe[3000] = {0};
	CString str;
	DWORD service;
	int urlType;

	for(int i=0; i<option.link_count; i++){
		str.Format("正在下载#%d", i+1);
		pDlg->ShowText(str);
		pDlg->SetTimer(9, 100, NULL);
		
		
		csLink = GetLink(i);
		urlType = GetURLType(csLink);
		if (urlType == -1){
			str.Format("(X)#%d失败，地址%s不支持。\r\n", i+1, GetLink(i));
			strcat(sRe, str);
			pDlg->ShowText(str);
			continue;
		}
		if (!AfxParseURL(csLink, service, csHost, csObj, nPort)){
			continue;
		}
		csName = csObj.Mid(csObj.ReverseFind('/') + 1);

		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szPath))) 
		{
			if(szPath[strlen(szPath) - 1] != '\\'){
				strcat(szPath, "\\");
			}
		} else {
			strcpy(szPath, "c:\\");
		}
		
		int sufIndex = i%strlen(option.filename_base);
		sprintf(sFilePath, "%s%s%s%s%d.exe", szPath, option.filename_base, FILE_NAME_MID, option.filename_group, sufIndex);
		
		CString csNewURL, csCookie;
		if (urlType == URL_TYPE_WIKIFORTIO){
			if(!DownloadUrl_wiki(csHost, nPort, csObj, csName, csNewURL, csCookie)){
				str.Format("(X)#%d失败，地址%s查询路径失败。\r\n", i+1, GetLink(i));
				strcat(sRe, str);
				pDlg->ShowText(str);
				continue;
			}
		} else if(urlType == URL_TYPE_LIVE) {
			if(!DownloadUrl_live(csHost, nPort, csObj, csName, csNewURL)){
				str.Format("(X)#%d失败，地址%s查询路径失败。\r\n", i+1, GetLink(i));
				strcat(sRe, str);
				pDlg->ShowText(str);
				continue;
			} else {
				csCookie == "";
			}
		}
		
		if (!AfxParseURL(csNewURL, service, csHost, csObj, nPort)){
			str.Format("(X)#%d失败，地址%s，查询路径为%s，解析失败.\r\n", i+1, GetLink(i), csNewURL);
			strcat(sRe, str);
			pDlg->ShowText(str);
			continue;
		}

		if(csCookie != "") {
			csCookie += "\r\n";
		}

		unsigned long fsize = 0;  //we should know the size
		BOOL re = MDownloadUrl(csHost, nPort, csObj, fsize,  sFilePath, 5, csCookie);
		if(re){
			str.Format("   #%d成功\r\n", i+1);
			strcat(sRe, str);
			DeleteFile(sFilePath);
		} else {
			str.Format("(X)#%d失败，地址%s\r\n", i+1, GetLink(i));
			strcat(sRe, str);
		}
		pDlg->ShowText(str);
		Sleep(400);
	}
	
	pDlg->ShowText("测试完成.");
	bRunning = false;
	::MessageBox(0, sRe, "测试结果", 0);
	return 0;
}


UINT _DownloadThread(LPVOID p)
{
	bRunning = true;
	CFginstDlg *pDlg = (CFginstDlg*)p;
	CString csLink, csHost, csObj, csFile, csName;
	unsigned short nPort = 80;
	DWORD service;
	int urlType = -1;
	csLink = GetLink(nTodayDay);
	urlType = GetURLType(csLink);
	if (urlType == -1){
		pDlg->SetTimer(4, 100, NULL);
		bRunning = false;
		return 3;
	}

// 	csObj = csHost.Mid(csHost.Find('/'));
// 	csHost = csHost.Left(csHost.Find('/'));
// 
// 	if (csHost.Find(':') != -1){
// 		nPort = atoi(csHost.Mid(csHost.Find(':') + 1));
// 		csHost = csHost.Left(csHost.Find(':'));
// 	}
// 
// 	csName = csObj.Mid(csObj.Find('/') + 1);

	if (!AfxParseURL(csLink, service, csHost, csObj, nPort)){
		pDlg->SetTimer(4, 100, NULL);
		bRunning = false;
		return 2;
	}
	csName = csObj.Mid(csObj.ReverseFind('/') + 1);

//	char sCurPath[MAX_PATH];
//	GetCurrentDirectory(MAX_PATH, sCurPath);
//	if(sCurPath[strlen(sCurPath) - 1] != '\\'){
//		strcat(sCurPath, "\\");
//	}
	TCHAR szPath[MAX_PATH];
	if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szPath))) 
	{
		if(szPath[strlen(szPath) - 1] != '\\'){
			strcat(szPath, "\\");
		}
	} else {
		strcpy(szPath, "c:\\");
	}

	sprintf(sFilePath, "%s%s%s%s%d.exe", szPath, option.filename_base, FILE_NAME_MID, option.filename_group, nTodayDay);


	CString csNewURL, csCookie;
	if (urlType == URL_TYPE_WIKIFORTIO){
		if(!DownloadUrl_wiki(csHost, nPort, csObj, csName, csNewURL, csCookie)){
			pDlg->SetTimer(4, 100, NULL);
			bRunning = false;
			return 1;
		}
	} else if(urlType == URL_TYPE_LIVE) {
		if(!DownloadUrl_live(csHost, nPort, csObj, csName, csNewURL)){
			pDlg->SetTimer(4, 100, NULL);
			bRunning = false;
			return 1;
		} else {
			csCookie == "";
		}
	}

	if (!AfxParseURL(csNewURL, service, csHost, csObj, nPort)){
		pDlg->SetTimer(4, 100, NULL);
		bRunning = false;
		return 2;
	}


	if(csCookie != "") {
		csCookie += "\r\n";
	}


	unsigned long fsize = 0;  //we should know the size
	BOOL re = MDownloadUrl(csHost, nPort, csObj, fsize,  sFilePath, 5, csCookie);
	if(re){
		pDlg->SetTimer(3, 100, NULL);
	} else {
		pDlg->SetTimer(4, 100, NULL);
	}
	Sleep(400);
	pDlg->KillTimer(2);
	bRunning = false;
	return 0;
}

void CFginstDlg::OnTimer(UINT nIDEvent) 
{
#ifdef _DEBUG
// 	if(101 == nIDEvent){
// 		//Test network
// 		KillTimer(nIDEvent);
// 
// 		CString config_host;
// 		INTERNET_PORT config_port;
// 		CString config_obj;
// 		DWORD service;
// 		if (!AfxParseURL("http://wikifortio.com/414870/", service, config_host, config_obj, config_port))
// 			return ;
// 		
// 		char* buf;
// 		int bufLen;
// 		DWORD code;
// 		unsigned long fsize = 0;
// 
// 		CString csNewURL = "";
// 		if (!DownloadUrl_wiki(config_host, config_port, config_obj, csNewURL, FileDownloadCookie)) {
// 			FileDownloadCookie = "";
// 			return ;
// 		}
// 
// 		if (!AfxParseURL(csNewURL, service, config_host, config_obj, config_port))
// 			return ;
// 
// 		fsize = 0;
// 		BOOL re = MDownloadUrl(config_host, config_port, config_obj, fsize,  "c:\\111.zip", 5);
// 	
// 
// 	}
#endif

	if(1 == nIDEvent){
		//Test network
		KillTimer(nIDEvent);
		if(GetNetworkTime()){
			ShowText("下载中，请稍候...");
			AfxBeginThread(_DownloadThread, this);	
			SetTimer(2, 200, NULL);
		} else {
			ShowText("网络错误，请稍后重试。");
		}
	}

	if (2 == nIDEvent){
		long FielLength = DownloadRange.DownloadRangeFileInfo.nFileLength;
		if(FielLength > 0){
			long FinshedLength = 0;
			for(int i=0; i<MAXBLOCKNUMBER; i++){
				FinshedLength += DownloadRange.DownloadRangeFileInfo.nFinishedLength[i];
			}

			CString str;
			str.Format("%d%%", FinshedLength * 100 / FielLength);
			SetDlgItemText(IDC_PROGRESS_TEXT, str);
			m_Progress.SetPos(FinshedLength * 100 / FielLength);
			if(FinshedLength  == FielLength){
				KillTimer(2);
				ShowText("下载完成，文件处理中...");
			}
		}
	}

	if(3 == nIDEvent){ // OK
		KillTimer(nIDEvent);
		ShowText("下载成功！文件在你的桌面上。");
		CString str;
		str.Format("下载成功，文件位置为: %s。请问立即执行该软件吗？", sFilePath);
		if(MessageBox(str, "提示", MB_ICONQUESTION|MB_YESNO) == IDYES){
			WinExec(sFilePath, SW_SHOW);
		}
	}	
	
	if(4 == nIDEvent){ //FAIL
		KillTimer(nIDEvent);
		ShowText("下载失败，请重试。");
	}

	if (5 == nIDEvent){
		long FielLength = DownloadRange.DownloadRangeFileInfo.nFileLength;
		if(FielLength > 0){
			long FinshedLength = 0;
			for(int i=0; i<MAXBLOCKNUMBER; i++){
				FinshedLength += DownloadRange.DownloadRangeFileInfo.nFinishedLength[i];
			}
			
			CString str;
			str.Format("%d%%", FinshedLength * 100 / FielLength);
			SetDlgItemText(IDC_PROGRESS_TEXT, str);
			m_Progress.SetPos(FinshedLength * 100 / FielLength);
		}
	}

	if(9 == nIDEvent){
		KillTimer(nIDEvent);
		DownloadRange.DownloadRangeFileInfo.nFileLength = 0;
		SetDlgItemText(IDC_PROGRESS_TEXT, "0%");
		m_Progress.SetPos(0);
	}

	CDialog::OnTimer(nIDEvent);
}

void CFginstDlg::ShowText(LPCTSTR lpText)
{
	SetDlgItemText(IDC_NOTE_TEXT, lpText);

}

void CFginstDlg::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	if(_access("config.ini", 0) != -1){
		if(0 > GetKeyState(VK_SHIFT)) {
			if(bRunning){
				MessageBox("请等待操作完成再试。", "提示", 0);
			} else {
				AfxBeginThread(_TestAllDownloadThread, this);	
				ShowText("测试下载全部连接。");
				SetTimer(5, 400, NULL);
			}
		}
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}
