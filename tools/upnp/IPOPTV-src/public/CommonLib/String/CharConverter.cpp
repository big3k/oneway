///////////////////////////////////////////////////////////////////////////////
// Module Name: CharConverter.cpp
// Written By: J.Liu
// Purpose: Implementation for CharConverter class.
///////////////////////////////////////////////////////////////////////////////

#include "CharConverter.h"

namespace CommonLib {

	CharConverter::CharConverter()
		:	m_eCate(Ascii), m_nBufferSize(0)
	{
		ZeroBuffer();
	}

	CharConverter::CharConverter(LPCSTR lpString)
		:	m_eCate(Ascii), m_nBufferSize(0)
	{
		Copy(lpString);
	}

	CharConverter::CharConverter(LPCWSTR lpString)	
		:	m_eCate(Unicode), m_nBufferSize(0)
	{	
		Copy(lpString);
	}

	CharConverter::CharConverter(const CharConverter& c)
		:	m_eCate(c.m_eCate), m_nBufferSize(0)
	{
		ZeroBuffer();
		if (c.KindOf() == Unicode)
		{
			Copy((LPCWSTR)c.GetBuffer());
		}
		else
		{
			Copy((LPCSTR)c.GetBuffer());
		}	
	}

	CharConverter& CharConverter::operator =(const CharConverter& c)
	{
		if (this != &c)
		{
			if (c.KindOf() == Unicode)
			{
				Copy((LPCWSTR)c.GetBuffer());
			}
			else
			{
				Copy((LPCSTR)c.GetBuffer());
			}
		}
		return (*this);
	}

	LPSTR CharConverter::GetAscii()
	{
		if (this->KindOf() != Ascii)
		{
			if (!ConvertToAscii())
			{
				return (NULL);
			}
		}
		return ((LPSTR)m_pBuffer);
	}

	LPWSTR CharConverter::GetUnicode()
	{
		if (this->KindOf() != Unicode)
		{
			if (!ConvertToUnicode())
			{
				return (NULL);
			}
		}
		return ((LPWSTR)m_pBuffer);
	}

	BOOL CharConverter::Copy(LPCSTR lpString, size_t nLen)
	{
		BOOL bCopy = FALSE;
		if (nLen < GetMaxBufferSize())
		{
			ZeroBuffer();
			if (nLen != 0)
			{	
				m_nBufferSize = nLen;
				strncpy((LPSTR)m_pBuffer, lpString, m_nBufferSize);
			}
			m_eCate = Ascii;
			bCopy = TRUE;
		}
		return (bCopy);
	}

	BOOL CharConverter::Copy(LPCWSTR lpString, size_t nLen)
	{
		BOOL bCopy = FALSE;
		if (nLen < GetMaxBufferSize())
		{
			ZeroBuffer();
			if (nLen != 0)
			{
				m_nBufferSize = nLen;
				wcsncpy((LPWSTR)m_pBuffer, lpString, m_nBufferSize);		
			}
			m_eCate = Unicode;
			bCopy = TRUE;
		}
		return (bCopy);
	}

	BOOL CharConverter::ConvertToAscii()
	{
		BOOL bSuccess = TRUE;
		if (m_eCate != Ascii)
		{
			if (!IsEmpty())
			{
				// 宽字符转换到多字节
				BYTE tmpBuffer[MAX_BUFFER_SIZE] = { 0 };		
				int cbMultiByte = WideCharToMultiByte(CP_ACP, 0, GetUnicode(), -1, 
											(LPSTR)tmpBuffer, MAX_BUFFER_SIZE, NULL, NULL);
				if (cbMultiByte != 0)
				{
					// To succeed
					Copy((LPCSTR)tmpBuffer, cbMultiByte - 1);
				}
				else
				{
					bSuccess = FALSE;
				}				
			}	
			m_eCate = Ascii;		
		}
		return (bSuccess);
	}

	BOOL CharConverter::ConvertToUnicode()
	{
		BOOL bSuccess = TRUE;
		if (m_eCate != Unicode)
		{
			if (!IsEmpty())
			{
				// 宽字符转换到多字节
				BYTE tmpBuffer[MAX_BUFFER_SIZE] = { 0 };			
				int cchWideChar = MultiByteToWideChar(CP_ACP, 0, GetAscii(), -1, 
											(LPWSTR)tmpBuffer, MAX_BUFFER_SIZE);
				if (cchWideChar != 0)
				{
					// To succeed
					Copy((LPCWSTR)tmpBuffer, cchWideChar - 1);
				}		
				else
				{
					bSuccess = FALSE;
				}
			}
			m_eCate = Unicode;
		}
		return (TRUE);
	}

}