#ifndef __REGISTRY_H__
#define __REGISTRY_H__
//
// Registry.h
//   - Helper functions registering and unregistering a component.
//

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer(HMODULE hModule, 
                       REFCLSID rCLSID, 
                       const char* szFriendlyName,
                       const char* szVerIndProgID,
                       const char* szProgID);

// This function will unregister a component.  Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(REFCLSID rCLSID,
                         const char* szVerIndProgID,
                         const char* szProgID);

#endif // __REGISTRY_H__