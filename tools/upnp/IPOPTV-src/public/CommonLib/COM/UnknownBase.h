#ifndef __UNKNOWN_BASE_H__
#define __UNKNOWN_BASE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <objbase.h>

///////////////////////////////////////////////////////////
//
// Nondelegating IUnknown interface
//   - Nondelegating version of IUnknown
//
interface INondelegatingUnknown
{
	STDMETHOD(NondelegatingQueryInterface(REFIID riid, void** ppVoid)) PURE;
	STDMETHOD_(ULONG, NondelegatingAddRef()) PURE;
	STDMETHOD_(ULONG, NondelegatingRelease()) PURE;
} ;


///////////////////////////////////////////////////////////
//
// Declaration of CUnknown 
//   - Base class for implementing IUnknown
//

class CUnknown : public INondelegatingUnknown
{
public:
	// Nondelegating IUnknown implementation
	STDMETHOD(NondelegatingQueryInterface(REFIID riid, void** ppVoid));
	STDMETHOD_(ULONG, NondelegatingAddRef());
	STDMETHOD_(ULONG, NondelegatingRelease());

	// Constructor
	CUnknown(IUnknown* pUnknownOuter) ;

	// Destructor
	virtual ~CUnknown() ;

	// Initialization (especially for aggregates)
	virtual HRESULT Init() { return (S_OK); }

	// Notification to derived classes that we are releasing
	virtual void FinalRelease() ;

	// Count of currently active components
	static long ActiveComponents() { return (s_cActiveComponents); }
	
	// Helper function
	HRESULT FinishQI(IUnknown* pUnknown, void** ppVoid) ;

protected:
	// Support for delegation
	IUnknown* GetOuterUnknown() const { return (m_pUnknownOuter); }

private:
	// Reference count for this object
	LONG m_cRef;
	
	// Pointer to (external) outer IUnknown
	IUnknown* m_pUnknownOuter;

	// Count of all active instances
	static LONG s_cActiveComponents; 
};


///////////////////////////////////////////////////////////
//
// Delegating IUnknown
//   - Delegates to the nondelegating IUnknown, or to the
//     outer IUnknown if the component is aggregated.
//
#define DECLARE_IUNKNOWN		                             \
	virtual HRESULT __stdcall	                             \
		QueryInterface(const IID& iid, void** ppv)           \
	{	                                                     \
		return GetOuterUnknown()->QueryInterface(iid,ppv) ;  \
	};	                                                     \
	virtual ULONG __stdcall AddRef()	                     \
	{	                                                     \
		return GetOuterUnknown()->AddRef() ;                 \
	};	                                                     \
	virtual ULONG __stdcall Release()	                     \
	{	                                                     \
		return GetOuterUnknown()->Release() ;                \
	};


///////////////////////////////////////////////////////////


#endif // __UNKNOWN_BASE_H__
