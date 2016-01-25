#ifndef __CMN_LIB_UTILITY_H__
#define __CMN_LIB_UTILITY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include <string>

#ifdef _UNICODE
typedef std::wstring   CmnLibStringT;
#else
typedef std::string    CmnLibStringT;
#endif // _UNICODE


namespace CommonLib
{
    BOOL CopyFilesRecurse(const CmnLibStringT& szSrcPath, const CmnLibStringT& szDstPath, BOOL bFailIfExists);    
    BOOL DeleteFiles(const CmnLibStringT& szSourcePath);        
} 

#endif // __CMN_LIB_UTILITY_H__