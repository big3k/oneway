#ifndef __CMN_LIB_COM_UTILITY_H__
#define __CMN_LIB_COM_UTILITY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000]

#define CMN_LIB_HR(_Exp) { HRESULT hr = _Exp; if (FAILED(hr)) { return (hr);} }
#define CMN_LIB_HR_TRUE(_Exp, _Code) { BOOL b = _Exp; if (b) { return (_Code);} }	
#define CMN_LIB_HR_FALSE(_Exp, _Code) { CMN_LIB_HR_TRUE(!(_Exp), _Code); }

#define CMN_LIB_CHK_POINTER(_Ptr) { if (_Ptr == NULL) { return (E_POINTER); } }
#define CMN_LIB_SAFE_ASSIGN(_Ptr, _Value) { CMN_LIB_CHK_POINTER(_Ptr); *_Ptr = _Value; return (S_OK); }
#define CMN_LIB_SAFE_RELEASE(_Ptr) { if (_Ptr) { _Ptr->Release(); _Ptr = NULL; } }
#define CMN_LIB_SAFE_PTR_ASSIGN(_Left, _Right) { if (_Right) { _Right->AddRef(); } if (_Left) { _Left->Release(); }	_Left = _Right;	}

#endif // __CMN_LIB_COM_UTILITY_H__