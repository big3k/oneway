///////////////////////////////////////////////////////////////////////////////
// Module Name: CircuitQueue.h
// Written By: J.Liu
// Purpose: 循环队列类.
///////////////////////////////////////////////////////////////////////////////

#ifndef __CMN_LIB_CIRCUIT_QUEUE_H__
#define __CMN_LIB_CIRCUIT_QUEUE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include <memory>

///////////////////////////////////////////////////////////////////////////////

namespace CommonLib {

	template <typename _Ty, typename _Alloc>
		class CCircuitQueue_Val
		{	// Base class for CCircuitQueue to hold allocator _Alval
		protected:
			CCircuitQueue_Val(_Alloc _Al = _Alloc())
				: Alval(_Al)
				{	// Construct allocator from _Al
				}
			typedef typename _Alloc::template rebind<_Ty>::other Alty;
			Alty Alval;	// Allocator object for values
		};

	template <typename _Ty, typename _Ax = std::allocator<_Ty> >
		class CCircuitQueue 
			: public CCircuitQueue_Val<_Ty, _Ax>
		{
		public:
			// Typedefs

			typedef CCircuitQueue<_Ty, _Ax>				Myt;
			typedef CCircuitQueue_Val<_Ty, _Ax>			Mybase;
			typedef typename Mybase::Alty				Alloc;
			typedef Alloc								allocator_type;
			typedef typename Alloc::value_type			value_type;
			typedef typename Alloc::size_type			size_type;
			typedef typename Alloc::difference_type		difference_type;		
			typedef typename Alloc::pointer				pointer;
			typedef typename Alloc::const_pointer		const_pointer;
			typedef typename Alloc::reference			reference;
			typedef typename Alloc::const_reference		const_reference;
			
			static const difference_type EOQ = -1;		// End of the queue

		public:
			// Constructor / Destructor

			CCircuitQueue() 
				:	Mybase(),
					_MyQueue(0), 
					_MyFirst(0), 
					_MyLast(0), 
					_MyCapacity(0), 
					_MySize(0)
				{
				}
			CCircuitQueue(const Alloc& _Al) 
				:	Mybase(_Al),
					_MyQueue(0), 
					_MyFirst(0), 
					_MyLast(0), 
					_MyCapacity(0), 
					_MySize(0)
				{
				}
			~CCircuitQueue()
				{
				this->Alval.deallocate(_MyQueue, Capacity());
				}	

		public:
			// Operations

			BOOL Create(size_type _Capacity)
				{
				Clear();
				if (Capacity() != _Capacity)
					{
					this->_Alval.deallocate(_MyQueue, Capacity());
					_MyQueue = this->_Alval.allocate(_Capacity);				
					if (_MyQueue == NULL)
						{
						return (FALSE);
						}
					_MyCapacity = _Capacity;
					}			
				return (_MyQueue != NULL);
				}
			BOOL Enqueue(pointer pSrc, size_type _Size)
				{
				if (pSrc == NULL || _Size == 0)
					{ // 允许0个元素入队	
					return (TRUE);
					}
				if ((Capacity() - Size()) >= _Size)
					{ // 队列中的空间够容纳_Size个元素				
					pointer _Last = _MyQueue + _MyLast;
					if (_MyFirst > _MyLast)
						{
						_UinitCopy(pSrc, pSrc + _Size, _Last);
						}
					else // HeadPos() <= TailPos()
						{
						size_type _BackSpace = Capacity() - _MyLast;
						if (_BackSpace >= _Size)
							{
							_UinitCopy(pSrc, pSrc + _Size, _Last);
							}
						else
							{
							_UinitCopy(pSrc, pSrc + _BackSpace, _Last);
							_UinitCopy(pSrc + _BackSpace, pSrc + nSize, _MyQueue);
							}
						}
					_MySize	+= _Size;		// 递增队列长度	
					_MyLast	+= _Size;		// 递增Tail长度，表示有元素入队
					_MyLast	%= Capacity();	// 确定Tail在队列中的索引(模Capacity是为了实现循环(Circuit))				
					return (TRUE);
					}
				return (FALSE);
				}
			BOOL Dequeue(pointer pTar, size_type _Size)
				{				
				if (Peek(pTar, _Size))
					{
					Dequeue(_Size);
					return (TRUE);
					}
				return (FALSE);
				}
			BOOL Peek(pointer pTar, size_type _Size)
				{				
				// 判断队列中的元素个数是否大于等于需要Peek的个数
				if (Size() >= _Size)
					{					
					if (pTar != NULL)
						{
						pointer _First = _MyQueue + _MyFirst;
						if (_MyFirst < _MyLast)
							{
							_Copy(_First, _First + _Size, _Tar);
							}
						else // HeadPos() >= TailPos()
							{
							size_type _BackSpace = Capacity() - _MyFirst;
							if (_BackSpace >= _Size)
								{
								_Copy(_First, _First + _Size, pTar);
								}
							else
								{
								_Copy(_First, _First + _BackSpace, pTar);
								_Copy(_MyQueue, _MyQueue + _Size - _BackSpace, pTar + _BackSpace);
								}
							}					
						}
					return (TRUE);
					}
				return (FALSE);
				}
			void Clear()
				{
				Dequeue(Size());
				}

		public:
			// Attributes

			size_type Capacity() const 
				{
				return (_MyCapacity); 
				}
			size_type Size() const
				{
				return (_MySize);
				}
			BOOL Empty()
				{
				return (Size() != 0);
				}
		public:
			// Element Access

			difference_type GetHeadPosition() const
				{ // Returns the position of the head element of the queue
				return (!Empty() ? _MyFirst : EOQ);
				}
			difference_type GetNextPosition(difference_type _Pos)
				{ // Gets the next position for iterating
				difference_type _NewPos = nPos;
				if (++_NewPos == _MyLast)
					{
					_NewPos = EOQ;
					}
				else
					{
					_NewPos %= Capacity();
					}			
				return (_NewPos);
				}		
			const_reference Front() const
				{
				return (At(_MyFirst));
				}
			reference Front()
				{
				return (At(_MyFirst));
				}
			const_reference At(difference_type _Pos) const
				{
				return (*(_MyQueue + _Pos));
				}
			reference At(difference_type _Pos)
				{
				return (*(_MyQueue + _Pos));
				}

		protected:
			// Internal Operations

			void _Dequeue(UINT _Size) 
				{
				pointer _First = _MyQueue + _MyFirst;					
				if (_MyFirst < _MyLast)
					{
					_Destroy(_First, _First + _Size);
					}
				else // HeadPos() >= TailPos()
					{
					size_type _BackSpace = Capacity() - _MyFirst;
					if (_BackSpace >= _Size)
						{
						_Destroy(_First, _First + _Size);
						}
					else
						{
						_Destroy(_First, _First + _BackSpace);
						_Destroy(_MyQueue, _MyQueue + _Size - _BackSpace);
						}
					}
				_MySize	 -= _Size;		// 递减队列长度
				_MyFirst += _Size;		// 递增Head长度，表示有元素出队
				_MyFirst %= _MyCapacity;// 确定Head在队列中的索引(模Capacity是为了实现循环(Circuit))			
				}
			static void _Copy(pointer _First, pointer _Last, pointer _Ptr)
				{
				std::copy(_First, _Last, _Ptr);
				}
			static void _UinitCopy(pointer _First, pointer _Last, pointer _Ptr)
				{ // Copy initializing [_First, _Last), using allocator			
				std::_Uninitialized_copy(_First, _Last, _Ptr, this->_Alval);		
				}
			static void _Destroy(pointer _First, pointer _Last)
				{ // Destroy [_First, _Last) using allocator
				std::_Destroy_range(_First, _Last, this->_Alval);
				}

		protected:

			pointer			_MyQueue;
			difference_type	_MyFirst;
			difference_type	_MyLast;		
			size_type		_MyCapacity;			
			size_type		_MySize;		
		};

}

#endif // __CMN_LIB_CIRCUIT_QUEUE_H__