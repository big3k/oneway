#include "Utility.h"
#include <tchar.h>

namespace CommonLib {
	void AppendPath(CmnLibStringT& szNewPath, LPCTSTR lpszPath, LPCTSTR lpszFileName) {
		TCHAR szBuffer[MAX_PATH] = { 0};
		int iLen = _stprintf(szBuffer, _T("%s\\%s"), lpszPath, lpszFileName);
		szNewPath.assign(szBuffer, iLen);
	}

	BOOL IsChildDir(WIN32_FIND_DATA& fd) {
		return((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			   (lstrcmp(fd.cFileName, _T("."))  != 0) &&
			   (lstrcmp(fd.cFileName, _T("..")) != 0));
	}

	BOOL CopyFilesRecurse(const CmnLibStringT& szSrcPath,
						  const CmnLibStringT& szDstPath,
						  BOOL bFailIfExists) {
		if (!CreateDirectory(szDstPath.c_str(), NULL)) {
			DWORD dwLastError = GetLastError();
			if (dwLastError == ERROR_PATH_NOT_FOUND) {
				return(FALSE);
			}
		}

		WIN32_FIND_DATA fd;
		CmnLibStringT szTemp = szSrcPath + _T("\\*.*");
		HANDLE hFind = FindFirstFile(szTemp.c_str(), &fd);
		BOOL bOk = (hFind != INVALID_HANDLE_VALUE);
		while (bOk) {
			BOOL bIsDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			if (bIsDir) {
				if (IsChildDir(fd)) {
					CmnLibStringT szSrc = szSrcPath + "\\" + fd.cFileName;
					CmnLibStringT szDst = szDstPath + "\\" + fd.cFileName;
					if (!CopyFilesRecurse(szSrc, szDst, bFailIfExists)) {
						FindClose(hFind);
						return(FALSE);
					}
				}
			} else { // Is File?
				CmnLibStringT szExistingFile;
				CmnLibStringT szNewFile;
				szExistingFile = szSrcPath + "\\" + fd.cFileName;
				szNewFile = szDstPath + "\\" + fd.cFileName;
				if (!CopyFile(szExistingFile.c_str(), szNewFile.c_str(), bFailIfExists)) {
					FindClose(hFind);
					return(FALSE);
				}
			}
			bOk = FindNextFile(hFind, &fd);
		}    
	    FindClose(hFind);
		return(TRUE);
	}

	BOOL DeleteFiles(const CmnLibStringT& szSourcePath) {
		TCHAR szSource[MAX_PATH] = { 0};
		_tcsncpy(szSource, szSourcePath.c_str(), szSourcePath.size());
		szSource[szSourcePath.size() + 1] = _T('\0');

		SHFILEOPSTRUCT  fs;
		ZeroMemory(&fs, sizeof (SHFILEOPSTRUCT));
		fs.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SILENT;
		fs.hwnd   = NULL;
		fs.pFrom  = szSource;    
		fs.wFunc  = FO_COPY;

		if (::SHFileOperation(&fs) != 0) {
			return(FALSE);
		}
		return(TRUE);
	}
}

