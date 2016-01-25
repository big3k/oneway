#ifndef __CMN_LIB_MD5_H__
#define __CMN_LIB_MD5_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"

namespace CommonLib {

	typedef union
	{
		BYTE	n[16];
		BYTE	b[16];
		DWORD	w[4];
	} MD4, MD5;
	
	inline bool operator <(const MD5& left, const MD5& right)
	{
		return (memcmp(&left, &right, sizeof (MD5)) < 0);
	}
	inline bool operator >(const MD5& left, const MD5& right)
	{
		return (right < left);
	}
	inline bool operator <=(const MD5& left, const MD5& right)
	{
		return (!(right < left));
	}
	inline bool operator >=(const MD5& left, const MD5& right)
	{
		return (!(left < right));
	}
	inline bool operator ==(const MD5& left, const MD5& right)
	{
		return (memcmp(&left, &right, sizeof (MD5)) == 0);
	}
	inline bool operator !=(const MD5& left, const MD5& right)
	{
		return (!(left == right));
	}

	//
	// MD5.h
	//
	class CMD5  
	{
	public:
		// Construction / Destructor

		CMD5();
		virtual ~CMD5();		
	
	public:
		// Attributes

		DWORD m_nCount[2];
		DWORD m_nState[4];
		BYTE  m_byBuffer[64];

	public:
		// Operations	

		void Reset();
		void Add(LPCVOID pData, DWORD nLength);
		void Finish();
		void GetHash(MD5* pHash);

	public:
		// Static Operations

		static VOID CreateMD5Hash(char szHash[33], BYTE* pData, DWORD dwLen);
		static VOID HashToString(char* szHash, const MD5* pHash, BOOL bURN = FALSE);	
		static BOOL HashFromString(LPCSTR pszHash, MD5* pMD5);
		static BOOL	HashFromURN(LPCSTR pszHash, MD5* pMD5);
	
	protected:
		// Implementation

		void Transform(BYTE* pBlock);
		void Encode(BYTE* pOutput, DWORD* pInput, DWORD nLength);
		void Decode(DWORD* pOutput, BYTE* pInput, DWORD nLength);
	};

}

#endif // __CMN_LIB_MD5_H__