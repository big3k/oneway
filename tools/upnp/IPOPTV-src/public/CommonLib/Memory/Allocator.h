///////////////////////////////////////////////////////////////////////////////
// Module Name: Allocator.h
// Written By: J.Liu
// Purpose: An allocator template class that should be rougfly as fast as the
//			original STL class-specific allocators, but with less fragmentation.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_ALLOCATOR_H__
#define __CMN_LIB_ALLOCATOR_H__

#include "MemoryPool.h"

namespace CommonLib {

	//
	// TEMPLATE CLASS CAllocBase for generic allocators
	//
	template <typename _Ty>
		struct CAllocBase
		{	
		typedef _Ty value_type;
		};

	//
	// TEMPLATE CLASS CAllocBase<const _Ty>
	//
	template <typename _Ty>
		struct CAllocBase<const _Ty>
		{	// base class for generic allocators for const _Ty
		typedef _Ty value_type;
		};


	//
	// TEMPLATE CLASS CAllocator for objects of typename _Ty
	//
	template <typename _Ty>
		class CAllocator : public CAllocBase<_Ty>
		{
			typedef CMN_LIB_MEMORY_POOL _Alloc;	// The underlying allocator

		public:
			// Typedefs

			typedef CAllocBase<_Ty> _Mybase;
			typedef typename _Mybase::value_type value_type;
			typedef value_type* pointer;
			typedef value_type& reference;
			typedef const value_type* const_pointer;
			typedef const value_type& const_reference;
			typedef size_t size_type;
			typedef ptrdiff_t difference_type;

		template <typename _Other>
			struct rebind
				{	
				// Convert an CAllocator<_Ty> to an CAllocator <_Other>
				typedef CAllocator<_Other> other;
				};

		public:
			// Constructor / Destructor / Assign operator

			CAllocator()
				{	
				// Construct default CAllocator (do nothing)
				}

			CAllocator(const CAllocator<_Ty>&) 
				{	
				// Construct by copying (do nothing)
				}

		template <typename _Other>
			CAllocator(const CAllocator<_Other>&)
				{
				// Construct from a related CAllocator (do nothing)
				}

		template <typename _Other>
			CAllocator<_Ty>& operator =(const CAllocator<_Other>&)
				{
				// Assign from a related CAllocator (do nothing)
				return (*this);
				}

		public:
			// Operations

			pointer address(reference _Val) const
				{	
				// Return address of mutable _Val
				return (&_Val);
				}

			const_pointer address(const_reference _Val) const
				{	
				// Return address of nonmutable _Val
				return (&_Val);
				}

			pointer allocate(size_type _Cnt, const void* = 0)
				{	
				// Allocate array of _Count elements			
				return (_Cnt != 0 ? (pointer)_Alloc::allocate(_Cnt * sizeof (_Ty)) : 0);
				}

			void construct(pointer _Ptr)
				{
				new (_Ptr) _Ty;				
				}

			void construct(pointer _Ptr, const _Ty& _Val)
				{	
				// Construct object at _Ptr with value _Val
				new (_Ptr) _Ty(_Val);
				}

			void destroy(pointer _Ptr)
				{	
				// Destroy object at _Ptr
				_Ptr->~_Ty(); 
				}

			void deallocate(pointer _Ptr, size_type _Cnt)
				{
				_Alloc::deallocate(_Ptr, _Cnt * sizeof (_Ty));
				}

			size_t max_size() const
				{
				// Estimate maximum array size
				return ((size_t)(-1) / sizeof (_Ty));
				}
			};

	//
	// CAllocator TEMPLATE OPERATORS
	//
	template <typename _Ty, typename _Other> inline
		bool operator==(const CAllocator<_Ty>&, const CAllocator<_Other>&)
			{	
			// Test for CAllocator equality (always true)
			return (true);
			}

	template <typename _Ty, typename _Other> inline
		bool operator!=(const CAllocator<_Ty>&, const CAllocator<_Other>&)
			{	
			// Test for CAllocator inequality (always false)
			return (false);
			}

	//	
	// CLASS CAllocator<void> : Generic CAllocator for type void
	//
	template <> 
		class CAllocator<void>
		{		
		public:
			typedef void _Ty;
			typedef _Ty* pointer;
			typedef const _Ty* const_pointer;
			typedef _Ty value_type;

		template <typename _Other>
			struct rebind
				{
				// Convert an CAllocator<void> to an allocator <_Other>
				typedef CAllocator<_Other> other;
				};

			CAllocator()
				{	
				// Construct default CAllocator (do nothing)
				}

			CAllocator(const CAllocator<_Ty>&)
				{	
				// Construct by copying (do nothing)
				}

		template <typename _Other>
			CAllocator(const CAllocator<_Other>&)
			{
			// Construct from related CAllocator (do nothing)
			}

		template <typename _Other>
			CAllocator<_Ty>& operator=(const CAllocator<_Other>&)
			{
			// Assign from a related CAllocator (do nothing)
			return (*this);
			}
		};

}

#endif // __CMN_LIB_ALLOCATOR_H__