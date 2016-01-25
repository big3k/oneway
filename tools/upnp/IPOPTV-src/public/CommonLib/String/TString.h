///////////////////////////////////////////////////////////////////////////////
// Module Name: TString.h
// Written By: J.Liu
// Purpose: 字符串定义类型。For Unicode and Ascii complier.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_TSTRING_H__
#define __CMN_LIB_TSTRING_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include <string>

namespace CommonLib {

#ifdef _UNICODE
typedef std::wstring CmnLibTString;
#else
typedef std::string	 CmnLibTString;
#endif // _UNICODE

}

#endif // __CMN_LIB_TSTRING_H__