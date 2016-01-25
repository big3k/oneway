///////////////////////////////////////////////////////////////////////////////
// Module Name: MemoryPool.h
// Written By: J.Liu
// Purpose: A memory pool class that is instead of behavior of 
//			default new or malloc.
//			1.	If the client request an object of size > _MAX_BYTES, 
//				the resulting object will be obtained directly from malloc.
//			2.	In all other cases, we allocate an object of size exactly 
//				alloc_round_up(requested_size). Thus the client has enough size
//				information that we can return the object to the proper free
//				list without permanently losting part of the object.
//			3.	The template parameter specifies whether more than one thread may 
//				be use the allocator. it is safe to allocate an object from one 
//				instance of a CAllocator and deallocate it with another	one. This 
//				effectively transfers its ownership to other one. This may have 
//				undesirable effects on reference locality. 
// Reference: For help, you can see SGI STL __default_alloc_template class.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_MEMORY_POOL_H__
#define __CMN_LIB_MEMORY_POOL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MemoryDef.h"

///////////////////////////////////////////////////////////////////////////////

namespace CommonLib {

	template <typename _AutoLocker>
		class CMemoryPool
		{
			// Typedefs

			typedef typename _AutoLocker MyAutoLocker;
			typedef typename _AutoLocker::LockTraits MyLocker;
			typedef size_t size_type;
		
			// Static / Constant members

			static const size_type ALIGN		= 8;
			static const size_type FREE_LISTS	= 16;
			static const size_type MAX_BYTES	= 128;			

			union Obj
			{
				union Obj*	pNextObj;
				char		data[1];
			};			

			// 自由链表的句柄数组，它的形状如下：
			// ------------------------------------
			// |#1|#2|#3|#4|#5|#6|#7|#8|#9|...|#16|
			// ------------------------------------
			//  8  16 24 32 40 48 56 64 72 ... 128
			// 每一个元素都指向一个自由链表.
			// s_HLists: static member that is array of handles of free list.
			static Obj* CMN_LIB_VOLATILE s_HLists[FREE_LISTS];			

		public:
			// Operations

			static void* allocate(size_type n)
				{
				void* p = 0;
				if (n > MAX_BYTES)
					{					
					p = CMN_LIB_MEMORY_ALLOC(n); // 大于128字节我们使用CRT函数malloc来分配
					}
				else
					{					
					// 找到尺寸大小与n相等的自由链表
					Obj* CMN_LIB_VOLATILE* pFreeList = s_HLists + free_list_index(n);					
					Obj* pResult = *pFreeList;
					MyAutoLocker locker(s_locker);
					if (pResult == 0)
						{							
						p = refill(round_up(n)); // 自由链表为空，从内存池中分配
						}
					else
						{							
						*pFreeList = pResult->pNextObj; // 自由链表非空，从该链表中分配
						p = pResult;
						}
					}			
				return (p);
				}

			// _p may not be 0
			static void deallocate(void* p, size_type n)
				{				
				if (n > MAX_BYTES)
					{
					CMN_LIB_MEMORY_FREE(p);
					}
				else
					{
					Obj* CMN_LIB_VOLATILE* pFreeList = s_HLists + free_list_index(n);
					Obj* q = (Obj*)p;
					MyAutoLocker locker(s_locker);
					q->pNextObj = *pFreeList; // 将q指向的节点加入pFreeList指向的自由链表中
					*pFreeList = q;					
					}			
				}

			static void* reallocate(void* p, size_type nOld, size_type nNew)
				{			
				void* pResult = 0;
				MyAutoLocker locker(s_locker);
				if (old_sz > MAX_BYTES && new_sz > MAX_BYTES)
					{
					pResult = CMN_LIB_MEMORY_REALLOC(p, new_sz);
					}
				else if (round_up(old_sz) == round_up(new_sz))
					{
					pResult = p;
					}
				else
					{
					pResult = allocate(new_sz);
					size_t copy_sz = new_sz > old_sz ? old_sz : new_sz;
					memcpy(pResult, p, copy_sz);
					deallocate(p, old_sz);
					}				
				return (pResult);
				}

		private:
			// Internal Operations

			static size_t round_up(size_type nBytes)
				{
				return ((nBytes + ALIGN - 1) & (~(ALIGN - 1)));
				}

			static size_t free_list_index(size_type nBytes)
				{
				return (((nBytes + ALIGN - 1) / ALIGN) - 1);
				}

			// Return an object of size _n, and optionally add to size _n free list.
			static void* refill(size_type n)
				{
				size_type nObjs = 20;
				char* pChunk = (char*)alloc_chunk(n, nObjs);
				Obj* pResult = 0;	

				if (nObjs == 1)
					{
					pResult = (Obj*)pChunk;
					}
				else
					{
					Obj* CMN_LIB_VOLATILE* pFreeList = s_HLists + free_list_index(n);

					// 第一块数据返回给客户，其余的添加到自由链表中
					pResult = (Obj*)(pChunk);
					Obj* pNextObj = (Obj*)(pChunk + n); 
					*pFreeList = pNextObj; // 让自由链表指向刚分配的块
					for (int i = 1; ; i++)
						{
						Obj* pCurrentObj = pNextObj;	// 保存指向pNextObj的指针
						pNextObj = (Obj*)((char*)pNextObj + n); // pNextObj指向下一个Obj

						if ((nObjs - 1) == i)
							{
							pCurrentObj->pNextObj = 0; // pCurrentObj指向最后一个Obj
							break;
							}
						else
							{
							pCurrentObj->pNextObj = pNextObj; // 将Obj串起来
							}
						}
					}
				return (pResult);
				}

			// Allocate a chunk for _nobjs(number of objects) of size _size. nobjs 
			// may be reduced if it is inconvenient to allocate the requested number.
			static void* alloc_chunk(size_type nSize, size_type& nObjs)
				{
				char* pResult = 0;
				size_type nTotalBytes = nSize * nObjs; // 计算需要分配多少字节
				size_type nLeftBytes = s_pPoolEnd - s_pPoolBeg; // 计算内存池中剩下的字节数

				if (nLeftBytes >= nTotalBytes)
					{	
					// 剩下的字节数完全够用，不需要特殊处理直接返回给客户
					pResult = s_pPoolBeg;
					s_pPoolBeg += nTotalBytes;
					}
				else if (nLeftBytes >= nSize)
					{
					// 不能全部满足要求，但也许可以分配1个nSize大小的Obj
					nObjs = (size_type)(nLeftBytes / nSize); // 计算可能分配的Obj的个数
					nTotalBytes = nSize * nObjs;
					pResult = s_pPoolBeg;
					s_pPoolBeg += nTotalBytes;
					}
				else
					{
					// 内存池已经干涸（连1个nSize大小的Obj对象都不能分配了）
					// 这次分配的尺寸是以前的2倍，在加上一个增量
					size_type nAllocBytes = 2 * nTotalBytes + round_up(s_nHeapSize >> 4);

					// 如果以前的池子中还有部分空间，那么我们将它们加入到比
					// 请求的nSize大小的Obj小的自由链表中去
					if (nLeftBytes > 0)
						{
						Obj* CMN_LIB_VOLATILE* pFreeList = s_HLists + free_list_index(nLeftBytes);
						((Obj*)s_pPoolBeg)->pNextObj = *pFreeList;
						*pFreeList = (Obj*)s_pPoolBeg;
						}

					// 调用malloc或其它分配方法分配一大块内存
					s_pPoolBeg = (char*)CMN_LIB_MEMORY_ALLOC(nAllocBytes);

					if (s_pPoolBeg != 0)
						{
						s_nHeapSize += nAllocBytes;
						s_pPoolEnd = s_pPoolBeg + nAllocBytes;
						pResult = (char*)alloc_chunk(nSize, nObjs);
						}
					else
						{
						// 加入异常处理代码, 例如:
						s_pPoolEnd = 0;	
						CMN_LIB_THROW(std::bad_alloc());
						}
					}
				return (pResult);
				}

		private:

			static MyLocker  s_locker;
			static char*	 s_pPoolBeg;
			static char*	 s_pPoolEnd;
			static size_t	 s_nHeapSize;
		};

		///////////////////////////////////////////////////////////////////////////

		template <typename _AutoLocker>
			typename CMemoryPool<_AutoLocker>::MyLocker CMemoryPool<_AutoLocker>::s_locker;

		template <typename _AutoLocker>
			typename CMemoryPool<_AutoLocker>::Obj* 
			CMN_LIB_VOLATILE CMemoryPool<_AutoLocker>::s_HLists[FREE_LISTS];			

		template <typename _AutoLocker>
			char* CMemoryPool<_AutoLocker>::s_pPoolBeg = 0;

		template <typename _AutoLocker>
			char* CMemoryPool<_AutoLocker>::s_pPoolEnd = 0;

		template <typename _AutoLocker>
			size_t CMemoryPool<_AutoLocker>::s_nHeapSize = 0;

		typedef CMemoryPool<AutoLockerST> MemoryPoolST;
		typedef CMemoryPool<AutoLockerMT> MemroyPoolMT;

	#ifdef CMN_LIB_USE_IN_MT
		typedef MemroyPoolMT CMN_LIB_MEMORY_POOL;
	#else
		typedef MemoryPoolST CMN_LIB_MEMORY_POOL;
	#endif

}

#endif // __CMN_LIB_MEMORY_POOL_H__