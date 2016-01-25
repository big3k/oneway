///////////////////////////////////////////////////////////////////////////////
// Module Name: Verify.h
// Written By: J.Liu
// Purpose: 断言和效验宏定义.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_VERIFY_H__
#define __CMN_LIB_VERIFY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include <cassert>

//
// Assert Macro
//
#define CMN_LIB_ASSERT(_Condition)	assert((_Condition))

//
// Verify Macro
//
#ifdef _DEBUG
# define CMN_LIB_VERIFY(_Condition)	CMN_LIB_ASSERT(_Condition)
#else
# define CMN_LIB_VERIFY(_Condition) __noop
#endif // CMN_LIB_USE_GUARD

#endif // __CMN_LIB_VERIFY_H__