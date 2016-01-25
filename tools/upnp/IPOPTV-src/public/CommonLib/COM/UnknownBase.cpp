///////////////////////////////////////////////////////////
//
// CUnknown.cpp 
//
// Implementation of IUnknown Base class
//
#include "UnknownBase.h"
#include "FactoryBase.h"

///////////////////////////////////////////////////////////
//
// Count of active objects
//   - Use to determine if we can unload the DLL.
//
LONG CUnknown::s_cActiveComponents = 0 ;


///////////////////////////////////////////////////////////
//
// Constructor
//
CUnknown::CUnknown(IUnknown* pUnknownOuter)
	: m_cRef(1)
{
	// Set m_pUnknownOuter pointer.
	if (pUnknownOuter == NULL)
	{		
		m_pUnknownOuter = reinterpret_cast<IUnknown*>
		                     (static_cast<INondelegatingUnknown*>
		                     (this));  // notice cast
	}
	else
	{		
		m_pUnknownOuter = pUnknownOuter;
	}

	// Increment count of active components.
	::InterlockedIncrement(&s_cActiveComponents);
}

//
// Destructor
//
CUnknown::~CUnknown()
{
	::InterlockedDecrement(&s_cActiveComponents);

	// If this is an EXE server, shut it down.
	CFactory::CloseExe();
}

//
// FinalRelease - called by Release before it deletes the component
//
void CUnknown::FinalRelease()
{	
	m_cRef = 1;
}

//
// Nondelegating IUnknown
//   - Override to handle custom interfaces.
//
STDMETHODIMP CUnknown::NondelegatingQueryInterface(REFIID riid, void** ppVoid)
{
	// CUnknown supports only IUnknown.
	if (riid == IID_IUnknown)
	{
		return (FinishQI(reinterpret_cast<IUnknown*>
						(static_cast<INondelegatingUnknown*>(this)),
						 ppVoid));
	}	
	*ppVoid = NULL;
	return (E_NOINTERFACE);	
}

//
// AddRef
//
STDMETHODIMP_(ULONG) CUnknown::NondelegatingAddRef()
{
	return (InterlockedIncrement(&m_cRef));
}

//
// Release
//
STDMETHODIMP_(ULONG) CUnknown::NondelegatingRelease()
{	
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		FinalRelease();
		delete this;
		return (0);
	}
	return (m_cRef);
}

//
// FinishQI
//   - Helper function to simplify overriding
//     NondelegatingQueryInterface
//
HRESULT CUnknown::FinishQI(IUnknown* pUnknown, void** ppVoid) 
{
	*ppVoid = pUnknown ;
	pUnknown->AddRef() ;
	return (S_OK);
}
