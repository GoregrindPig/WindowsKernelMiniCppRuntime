#pragma once

#include "AvlTree.h"

template <typename ConcreteSet, typename T, typename Lock, typename Alloc> class KSet
: public KAvlTree< ConcreteSet, Lock, Alloc >
{
	CLASS_NO_COPY(KSet)
public:
	explicit KSet() {}
	~KSet() {}

protected:
	__checkReturn
	RTL_GENERIC_COMPARE_RESULTS OnCompare(__in CRef_t x, __in CRef_t y) const
	{
		if (x == y)
			return GenericEqual;

		if (x < y)
			return GenericLessThan;
		else
			return GenericGreaterThan;
	}
};

template <typename T, typename Lock, typename Alloc> class KPoolSet
: public KSet< KPoolSet<T, Lock, Alloc>, T, Lock, Alloc >
, public KAvlTreePoolEventSink<Alloc>
{
	CLASS_NO_COPY(KPoolSet)

	friend class KAvlTree< KPoolSet<T, Lock, Alloc>, Lock, Alloc >;
public:
	explicit KPoolSet() : KAvlTreePoolEventSink(m_allocator)
	{}

	~KPoolSet()
	{
		Cleanup();
	}

private:
	Alloc_t m_allocator;
};

template <typename T, typename Lock, typename Alloc> class KLookasideSet
: public KSet< KLookasideSet<T, Lock, Alloc>, T, Lock, Alloc >
, public KAvlTreeLookasideEventSink<Alloc>
{
	CLASS_NO_COPY(KLookasideSet)

	friend class KAvlTree< KLookasideSet<T, Lock, Alloc>, Lock, Alloc >;
public:
	explicit KLookasideSet()
	{
		Reset(&m_allocator);
	}

	~KLookasideSet()
	{
		Cleanup();
	}

private:
	typedef typename Alloc::template Rebind_t<Item_t>::Other_t ItemAlloc_t;

private:
	ItemAlloc_t m_allocator;
};

template <typename T> struct KPagedPoolSet
{
	typedef KPoolSet< T, KGuardedMutex, typename KPagedPoolAllocator< T >::Type > Type;
};

template <typename T> struct KNonPagedPoolSet
{
	typedef KPoolSet< T, KSpinLock, typename KNonPagedPoolAllocator< T >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolSet
{
	typedef KPoolSet< T, KGuardedMutex, typename KTaggedPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolSet
{
	typedef KPoolSet< T, KSpinLock, typename KTaggedNonPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KPagedLookasideSet
{
	typedef KLookasideSet< T, KGuardedMutex, KPagedLookasideAllocator< T, Tag > > Type;
};

template <typename T, ULONG Tag> struct KNonPagedLookasideSet
{
	typedef KLookasideSet< T, KSpinLock, KNonPagedLookasideAllocator< T, Tag > > Type;
};