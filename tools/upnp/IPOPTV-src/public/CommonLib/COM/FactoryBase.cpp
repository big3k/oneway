///////////////////////////////////////////////////////////
//
// CFactory
//   - Base class for reusing a single class factory for
//     all components in a DLL
//
#include <objbase.h>

#include "Registry.h"
#include "FactoryBase.h"

///////////////////////////////////////////////////////////
//
// Static variables
//
LONG CFactory::s_cServerLocks = 0 ;    // Count of locks

HMODULE CFactory::s_hModule = NULL ;   // DLL module handle

#ifdef _OUTPROC_SERVER_
DWORD CFactory::s_dwThreadID = 0 ;
#endif

///////////////////////////////////////////////////////////
//
// CFactory implementation
//

CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
	m_pFactoryData = pFactoryData ;
}

//
// IUnknown implementation
//
STDMETHODIMP CFactory::QueryInterface(REFIID riid, void** ppVoid)
{ 	
	IUnknown* pI;
	if ((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
		pI = this; 
	}
	else
	{
	   *ppVoid = NULL;
		return (E_NOINTERFACE);
	}
	pI->AddRef();
	*ppVoid = pI;
	return (S_OK);
}

STDMETHODIMP_(ULONG) CFactory::AddRef() 
{ 
	return (InterlockedIncrement(&m_cRef)); 
}

STDMETHODIMP_(ULONG) CFactory::Release() 
{
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this; 
		return (0);
	}   
	return (m_cRef);
}

//
// IClassFactory implementation
//

STDMETHODIMP CFactory::CreateInstance(IUnknown* pUnknownOuter, REFIID riid, void** ppVoid) 
{

	// Aggregate only if the requested IID is IID_IUnknown.
	CMN_LIB_HR_TRUE((pUnknownOuter != NULL) && (riid != IID_IUnknown), CLASS_E_NOAGGREGATION);

	// Create the component.
	CUnknown* pNewComponent;
	HRESULT hr = m_pFactoryData->CreateInstance(pUnknownOuter, &pNewComponent);
	if (FAILED(hr))
	{
		return (hr);
	}

	// Initialize the component.
	hr = pNewComponent->Init();
	if (FAILED(hr))
	{
		// Initialization failed.  Release the component.
		pNewComponent->NondelegatingRelease();
		return (hr);
	}
	
	// Get the requested interface.
	hr = pNewComponent->NondelegatingQueryInterface(riid, ppVoid);

	// Release the reference held by the class factory.
	pNewComponent->NondelegatingRelease();
	return (hr);   
}

// LockServer
STDMETHODIMP CFactory::LockServer(BOOL bLock)
{
	if (bLock) 
	{
		::InterlockedIncrement(&s_cServerLocks); 
	}
	else
	{
		::InterlockedDecrement(&s_cServerLocks);
	}
	// If this is an out-of-proc server, check to see
	// whether we should shut down.
	CloseExe();  //@local

	return (S_OK);
}


///////////////////////////////////////////////////////////
//
// GetClassObject
//   - Create a class factory based on a CLSID.
//
HRESULT CFactory::GetClassObject(REFCLSID rCLSID, REFIID riid, void** ppVoid)
{
	CMN_LIB_HR_TRUE((riid != IID_IUnknown) && (riid != IID_IClassFactory), E_NOINTERFACE);

	// Traverse the array of data looking for this class ID.
	for (int i = 0; i < g_cFactoryDataEntries; i++)
	{
		const CFactoryData* pData = &g_FactoryDataArray[i] ;
		if (pData->IsClassID(rCLSID))
		{

			// Found the ClassID in the array of components we can
			// create. So create a class factory for this component.
			// Pass the CFactoryData structure to the class factory
			// so that it knows what kind of components to create.
			*ppVoid = (IUnknown*) new CFactory(pData) ;			
			CMN_LIB_HR_TRUE(*ppVoid == NULL, E_OUTOFMEMORY);
			return (NOERROR);
		}
	}
	return (CLASS_E_CLASSNOTAVAILABLE);
}

//
// Determine if the component can be unloaded.
//
HRESULT CFactory::CanUnloadNow()
{
	return (CUnknown::ActiveComponents() || IsLocked() ? S_FALSE : S_OK);	
}

//
// Register all components.
//
HRESULT CFactory::RegisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)
	{
		RegisterServer(s_hModule,
		               *(g_FactoryDataArray[i].m_pCLSID),
		               g_FactoryDataArray[i].m_RegistryName,
		               g_FactoryDataArray[i].m_szVerIndProgID, 
		               g_FactoryDataArray[i].m_szProgID) ;
	}
	return (S_OK);
}   
	
HRESULT CFactory::UnregisterAll()
{
	for(int i = 0 ; i < g_cFactoryDataEntries ; i++)   
	{
		UnregisterServer(*(g_FactoryDataArray[i].m_pCLSID),
		                 g_FactoryDataArray[i].m_szVerIndProgID, 
		                 g_FactoryDataArray[i].m_szProgID) ;
	}
	return (S_OK);
}

#ifndef _OUTPROC_SERVER_
//////////////////////////////////////////////////////////
//
// Exported functions
//

STDAPI DllCanUnloadNow()
{
	return (CFactory::CanUnloadNow()); 
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv) 
{
	return (CFactory::GetClassObject(clsid, iid, ppv));
}

//
// Server registration
//
STDAPI DllRegisterServer()
{
	return (CFactory::RegisterAll());
}


STDAPI DllUnregisterServer()
{
	return (CFactory::UnregisterAll());
}

///////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL APIENTRY DllMain(
					  HMODULE hModule, 
                      DWORD dwReason, 
                      void* lpReserved
					  )
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CFactory::s_hModule = hModule;
	}
	return (TRUE);
}

#else
//////////////////////////////////////////////////////////
//
// Out of process Server support
//

//
// Start factories
//
BOOL CFactory::StartFactories()
{
	CFactoryData* pStart = &g_FactoryDataArray[0];
	const CFactoryData* pEnd = &g_FactoryDataArray[g_cFactoryDataEntries - 1];

	for (CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
	{
		// Initialize the class factory pointer and cookie.
		pData->m_pIClassFactory = NULL;
		pData->m_dwRegister = NULL;

		// Create the class factory for this component.
		IClassFactory* pIFactory = new CFactory(pData);

		// Register the class factory.
		DWORD dwRegister;
		HRESULT hr = ::CoRegisterClassObject(
		                  *pData->m_pCLSID,
		                  static_cast<IUnknown*>(pIFactory),
		                  CLSCTX_LOCAL_SERVER,
		                  REGCLS_MULTIPLEUSE,
		                  // REGCLS_MULTI_SEPARATE, //@Multi
		                  &dwRegister);
		if (FAILED(hr))
		{
			pIFactory->Release();
			return (FALSE);
		}

		// Set the data.
		pData->m_pIClassFactory = pIFactory;
		pData->m_dwRegister = dwRegister;
	}
	return (TRUE);
}

//
// Stop factories
//
void CFactory::StopFactories()
{
	CFactoryData* pStart = &g_FactoryDataArray[0];
	const CFactoryData* pEnd = &g_FactoryDataArray[g_cFactoryDataEntries - 1];

	for (CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
	{
		// Get the magic cookie and stop the factory from running.
		DWORD dwRegister = pData->m_dwRegister;
		if (dwRegister != 0) 
		{
			::CoRevokeClassObject(dwRegister);
		}

		// Release the class factory.
		IClassFactory* pIFactory = pData->m_pIClassFactory;
		if (pIFactory != NULL) 
		{
			pIFactory->Release();
		}
	}
}

#endif //_OUTPROC_SERVER_
