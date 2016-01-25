#ifndef __CMN_LIB_MEMORY_DEF_H__
#define __CMN_LIB_MEMORY_DEF_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Win32\AutoLocker.h"

//
// Raw Memory Allocate Mothed
//
#define CMN_LIB_MEMORY_ALLOC(n)			::malloc(n)
#define CMN_LIB_MEMORY_REALLOC(p, n)	::realloc(p, n)
#define CMN_LIB_MEMORY_FREE(p)			::free(p)

#endif // __CMN_LIB_MEMORY_DEF_H__