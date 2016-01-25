///////////////////////////////////////////////////////////////////////////////
// Module Name: CLookasideList.h
// Written By: J.Liu
// Purpose: A memory pool class that is a Big-Buffer list.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_MEMORY_BAND_H__
#define __CMN_LIB_MEMORY_BAND_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MemoryDef.h"
#include <new>

///////////////////////////////////////////////////////////////////////////////

namespace CommonLib {

	//
	// CLASS CLookasideList
	//
	template <typename _Ty, typename _AutoLocker>
		class CLookasideList
		{
			// Typedefs

			union Node
			{
				union Node* _NextNode;
				char		_Value[1];
			};

			typedef Node* NodePtr;
			typedef typename _AutoLocker MyAutoLocker;
			typedef typename _AutoLocker::LockTraits MyLocker;			

		public:			

			typedef _Ty	value_type;
			typedef value_type*	pointer;
			typedef const value_type* const_pointer;
			typedef value_type&	reference;
			typedef const value_type& const_reference;
			typedef size_t size_type;

		public:
			// Constructor / Destructor / Copy Constructor / Assign Operater

			explicit CLookasideList(size_type nReserve = 0)
				: m_pMyHead(0)
				{
				while (nReserve > 0)
					{
					CMN_LIB_TRY_BEGIN
					NodePtr _Ptr = AllocNode(sizeof (_Ty));
					Link(&m_pMyHead, &_Ptr);
					CMN_LIB_CATCH_ALL
					Free(m_pMyHead);
					CMN_LIB_RETHROW
					CMN_LIB_CATCH_END
					--nReserve;
					}
				}
			~CLookasideList()
				{
				Free(m_pMyHead);
				}
			CLookasideList(const CLookasideList&)
				: m_pMyHead(0)
				{			
				}
			CLookasideList& operator =(const CLookasideList&)
				{
				return (*this);
				}

		public:

			pointer Allocate()
				{
				MyAutoLocker locker(m_locker);
				pointer pResult = (pointer)m_pMyHead;
				if (pResult == 0)
					{
					pResult = (pointer)AllocNode(sizeof (_Ty));
					}
				else
					{
					m_pMyHead = m_pMyHead->_NextNode;
					}              
				return (pResult);
				}
			void Deallocate(pointer _Ptr)
				{
				MyAutoLocker locker(m_locker);
				Link(&m_pMyHead, (NodePtr*)&_Ptr);				
				}
			static void Construct(pointer _Ptr)
				{
				new (_Ptr) _Ty;
				}
			static void Destroy(pointer _Ptr)
				{
				_Ptr->~_Ty();
				}

		private:

			static NodePtr AllocNode(size_type _Size)
				{
				NodePtr pNode = 0;
				CMN_LIB_TRY_BEGIN
				pNode = (NodePtr)CMN_LIB_MEMORY_ALLOC(_Size);
				if (pNode == 0)
					{
					CMN_LIB_THROW(std::bad_alloc());
					}
				CMN_LIB_CATCH_ALL
				CMN_LIB_THROW("Out of memory!");
				CMN_LIB_CATCH_END
				return (pNode);	
				}
			static void Link(NodePtr* _Front, NodePtr* _Back)
				{
				(*_Back)->_NextNode = *_Front; *_Front = *_Back;				
				}		
			static void FreeNode(NodePtr _Ptr)
				{
				CMN_LIB_MEMORY_FREE(_Ptr);
				}
			static void Free(NodePtr _Head)
				{
				while (_Head)
					{
					NodePtr _Next = _Head->_NextNode;
					FreeNode(_Head);
					_Head = _Next;
					}
				}

		private:						
	
			MyLocker m_locker;	// 锁定器
			NodePtr	 m_pMyHead;	// 指向头结点
		};

	//
	// CLASS CLookasideList<char>
	//
	#define _CHAR_TYPE BYTE
	template <typename _AutoLocker>
		class CLookasideList<_CHAR_TYPE, typename _AutoLocker>
		{
			union Node
			{
				union Node* _NextNode;
				char		_Value[1];
			};

			typedef Node* NodePtr;
			typedef typename _AutoLocker MyAutoLocker;
			typedef typename _AutoLocker::LockTraits MyLocker;

		public:
			// Typedefs

			typedef _CHAR_TYPE value_type;
			typedef value_type*	pointer;
			typedef const value_type* const_pointer;
			typedef value_type&	reference;
			typedef const value_type& const_reference;
			typedef size_t size_type;

		public:
			// Constructor / Destructor / Coye Constructor / Assign Operater

			CLookasideList(size_type nLength, size_type nReserve = 0)
				: m_pMyHead(0), m_nLength(nLength)
				{
				while (nReserve > 0)
					{				
					CMN_LIB_TRY_BEGIN
					NodePtr _Ptr = AllocNode(nLength * sizeof (_CHAR_TYPE));
					Link(&m_pMyHead, &_Ptr);
					CMN_LIB_CATCH_ALL
					Free(m_pMyHead);
					CMN_LIB_RETHROW
					CMN_LIB_CATCH_END
					--nReserve;
					}
				}
			~CLookasideList()
				{
				Free(m_pMyHead);
				}
			CLookasideList(const CLookasideList& l)
				: m_pMyHead(0), m_nLength(l.m_nLength)
				{			
				}
			CLookasideList& operator =(const CLookasideList&)
				{
				return (*this);
				}

		public:

			pointer Allocate()
				{
				MyAutoLocker locker(m_locker);
				pointer pResult = (pointer)m_pMyHead;
				if (pResult == 0)
					{
					pResult = (pointer)AllocNode(m_nLength * sizeof (_CHAR_TYPE));
					}
				else
					{
					m_pMyHead = m_pMyHead->_NextNode;
					}
				return (pResult);
				}
			void Deallocate(pointer _Ptr)
				{
				MyAutoLocker locker(m_locker);
				Link(&m_pMyHead, (NodePtr*)&_Ptr);				
				}
			size_type Length() const 
				{
				return (m_nLength); 
				}

		private:

			static NodePtr AllocNode(size_type _Size)
				{
				NodePtr pNode = 0;
				CMN_LIB_TRY_BEGIN				
				pNode = (NodePtr)CMN_LIB_MEMORY_ALLOC(_Size);
				if (pNode == 0)
					{
					CMN_LIB_THROW(std::bad_alloc());
					}
				CMN_LIB_CATCH_ALL
				CMN_LIB_RETHROW
				CMN_LIB_CATCH_END
				return (pNode);	
				}
			static void Link(NodePtr* _Front, NodePtr* _Back)
				{
				(*_Back)->_NextNode = *_Front;
				*_Front = *_Back;
				}		
			static void FreeNode(NodePtr _Ptr)
				{
				CMN_LIB_MEMORY_FREE(_Ptr);
				}
			static void Free(NodePtr _Head)
				{
				while (_Head)
					{
					NodePtr _Next = _Head->_NextNode;
					FreeNode(_Head);
					_Head = _Next;
					}
				}

		private:			

			MyLocker	m_locker;
			NodePtr		m_pMyHead;	// 指向头结点
			size_type	m_nLength;	// 每个块的单位长度
		};

    #define LookasideListBYTE_ST   CLookasideList<_CHAR_TYPE, CommonLib::AutoLockerST>
    #define LookasideListBYTE_MT   CLookasideList<_CHAR_TYPE, CommonLib::AutoLockerMT>

	#define LookasideListST(_Ty) CLookasideList<_Ty, CommonLib::AutoLockerST>
	#define LookasideListMT(_Ty) CLookasideList<_Ty, CommonLib::AutoLockerMT>

	#ifdef CMN_LIB_USE_IN_MT
	#define LookasideList(_Ty)  LookasideListST(_Ty)
    #define LookasideListBYTE   LookasideListBYTE_ST
	#else
	#define LookasideList(_Ty) LookasideListMT(_Ty)
    #define LookasideListBYTE   LookasideListBYTE_MT
	#endif
	
}

#endif // __CMN_LIB_MEMORY_BAND_H__