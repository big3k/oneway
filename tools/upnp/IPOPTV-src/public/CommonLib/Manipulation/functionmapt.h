///////////////////////////////////////////////////////////////////////////////
// Module Name: FunctionMap.h
// Written By: J.Liu
// Purpose: 
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_FUNCTION_MAP_TEMPLATE_H__
#define __CMN_LIB_FUNCTION_MAP_TEMPLATE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER

#include <map>
#include <hash_map>

#pragma warning (disable : 4996)

namespace CommonLib {    
    
    
    // 
    // CLASS CTreeMap
    //
    template <typename TKey, typename TValue>
        class CTreeMap
        {
        public:
            // Typedefs

            typedef typename TKey keyClass;
            typedef typename TValue valueClass;
            typedef typename std::map<keyClass, valueClass> mapBase;

        public:
            // Operations

            BOOL Append(const TKey& key, TValue value)
            {
                return (m_mapBase.insert(map_base::value_type(key, value)).second);
            }
            BOOL Find(const TKey& key, TValue* value)
            {
                *value = NULL;
                map_base::iterator iter = m_mapBase.find(key);
                if (iter != m_mapBase.end())
                {
                    *value = iter->second;
                    return (TRUE);
                }
                return (FALSE);
            }
            BOOL Remove(const TKey& key)
            {
                return (m_mapBase.erase(key) > 0);
            }
            VOID Clear()
            {
                m_mapBase.clear();
            }

        protected:
            // Date Members

            mapBase m_mapBase;
        };

        // 
        // CLASS CMapBase
        //
        template <typename TKey, typename TValue>
            class CHashMap
            {
            public:
                // Typedefs

                typedef typename TKey keyClass;
                typedef typename TValue valueClass;
                typedef typename stdext::hash_map<keyClass, valueClass> mapBase;

            public:
                // Operations

                BOOL Append(const TKey& key, TValue value)
                {
                    return (m_mapBase.insert(mapBase::value_type(key, value)).second);
                }
                BOOL Find(const TKey& key, TValue* value)
                {
                    *value = NULL;
                    mapBase::iterator iter = m_mapBase.find(key);
                    if (iter != m_mapBase.end())
                    {
                        *value = iter->second;
                        return (TRUE);
                    }
                    return (FALSE);
                }
                BOOL Remove(const TKey& key)
                {
                    return (m_mapBase.erase(key) > 0);
                }
                VOID Clear()
                {
                    m_mapBase.clear();
                }

            protected:
                // Date Members

                mapBase m_mapBase;
            };

    // 
    // CLASS CFunctionMapT
    //
    template 
        <        
        typename TKey, 
        typename TFunction, 
        typename TBase
        >
        class CFunctionMap : public TBase
        {
        public:
            // Typedefs

            typedef typename TKey keyClass;
            typedef typename TFunction functionClass;
            typedef typename TBase baseClass;

        public:
            // Operations

            BOOL Register(const keyClass& key, functionClass function)
                {                
                    return (baseClass::Append(key, function));
                }
            BOOL Find(const keyClass& key, functionClass* function)
                {                
                    return (baseClass::Find(key, function));
                }
            BOOL Revoke(const keyClass& key)
                {
                    return (baseClass::Remove(key));
                }
            VOID Clear()
                {                
                    baseClass::Clear();
                }
        };    
}

#endif // __CMN_LIB_FUNCTION_MAP_TEMPLATE_H__