///////////////////////////////////////////////////////////////////////////////
// Module Name: CharConverter.h
// Written By: J.Liu
// Purpose: ½«¿í×Ö·û×Ö·û´®×ª»»Îª¶à×Ö½Ú×Ö·û´®»ò½«¶à×Ö½Ú×Ö·û´®×ª»»Îª¿í×Ö·û´®.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CHAR_CONVERTER_H__
#define __CHAR_CONVERTER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include "CommonDef.h"

namespace CommonLib {

	class CharConverter
	{
	public:
		// Category

		enum Category { Ascii, Unicode } ;	

	public:
		CharConverter();	
		explicit CharConverter(LPCSTR lpString);
		explicit CharConverter(LPCWSTR lpString);		
		CharConverter(const CharConverter& c);
		CharConverter& operator =(const CharConverter& c);

	public:
		// Operations

		LPSTR GetAscii();
		LPWSTR GetUnicode();

		// Ascii version
		// Notice: nLen is excluding the null terminate Character
		BOOL Copy(LPCSTR lpString) { return (Copy(lpString, strlen(lpString))); }
		BOOL Copy(LPCSTR lpString, size_t nLen);

		// Unicode version
		// Notice: nLen is excluding the null terminate Character
		BOOL Copy(LPCWSTR lpString) { return (Copy(lpString, wcslen(lpString))); }
		BOOL Copy(LPCWSTR lpString, size_t nLen);

	public:
		// Attributes

		static size_t GetMaxBufferSize() { return (MAX_BUFFER_SIZE); };

		size_t GetSize() const { return (m_nBufferSize); }

		BOOL IsEmpty() const { return (m_nBufferSize == 0); }
		Category KindOf() const { return (m_eCate); }

	private:
		static const size_t MAX_BUFFER_SIZE = MAX_PATH * 2;

		void ZeroBuffer() { ZeroMemory(m_pBuffer, MAX_BUFFER_SIZE); }
		BYTE* GetBuffer() const { return ((BYTE*)m_pBuffer); }

		BOOL ConvertToAscii();
		BOOL ConvertToUnicode();

	private:
		Category	m_eCate;
		BYTE		m_pBuffer[MAX_BUFFER_SIZE];
		size_t		m_nBufferSize;
	};

}

#endif // __CHAR_CONVERTER_H__

