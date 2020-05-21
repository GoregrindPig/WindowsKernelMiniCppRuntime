#pragma once

#include "AvlTree.h"
#include "Utility.h"

template <typename ConcreteMap, typename Key, typename T, typename Lock, typename Alloc> class KMap
: public KAvlTree< ConcreteMap, Lock, Alloc >
{
	CLASS_NO_COPY(KMap)

	typedef KAvlTree< ConcreteMap, Lock, Alloc > Base_t;
public:
	typedef Key Key_t;
	typedef T Mapped_t;

	explicit KMap() {}
	~KMap() {}

	__checkReturn_opt
	bool Erase(__in const Key_t& key)
	{
		Val_t val(key, Mapped_t());
		return Base_t::Erase(val);
	}

	__checkReturn
	Iter_t Find(__in const Key_t& key)
	{
		Val_t val(key, Mapped_t());
		return Base_t::Find(val);
	}

	__checkReturn_opt
	__drv_mustHold(Lock)
	Mapped_t& operator[] (__in const Key_t& key)
	{
		Val_t val(key, Mapped_t());

		m_lock.Lock();
		Ptr_t item = Lookup(val);
		if (item)
		{
			m_lock.Unlock();
			return item->second;
		}
		else
		{
			m_lock.Unlock();

			Ptr_t res = NULL;
			if (Insert(val, &res))
				return res->second;
			else
				return m_nullObj.second;
		}
	}

protected:
	__checkReturn
	RTL_GENERIC_COMPARE_RESULTS OnCompare(__in CRef_t x, __in CRef_t y) const
	{
		if (x.first == y.first)
			return GenericEqual;

		if (x.first < y.first)
			return GenericLessThan; 
		else
			return GenericGreaterThan;
	}

protected:
	Val_t m_nullObj;
};

template <typename Key, typename T, typename Lock, typename Alloc> class KPoolMap
: public KMap< KPoolMap<Key, T, Lock, Alloc>, Key, T, Lock, Alloc >
, public KAvlTreePoolEventSink<Alloc>
{
	CLASS_NO_COPY(KPoolMap)

	friend class KAvlTree< KPoolMap<Key, T, Lock, Alloc>, Lock, Alloc >;
public:
	explicit KPoolMap() : KAvlTreePoolEventSink(m_allocator)
	{}

	~KPoolMap()
	{
		Cleanup();
	}

private:
	Alloc_t m_allocator;
};
	
template <typename Key, typename T, typename Lock, typename Alloc> class KLookasideMap
: public KMap< KLookasideMap<Key, T, Lock, Alloc>, Key, T, Lock, Alloc >
, public KAvlTreeLookasideEventSink<Alloc>
{
	CLASS_NO_COPY(KLookasideMap)

	friend class KAvlTree< KLookasideMap<Key, T, Lock, Alloc>, Lock, Alloc >;
public:
	explicit KLookasideMap()
	{
		Reset(&m_allocator);
	}

	~KLookasideMap()
	{
		Cleanup();
	}

private:
	typedef typename Alloc::template Rebind_t<Item_t>::Other_t ItemAlloc_t;

private:
	ItemAlloc_t m_allocator;
};

template <typename K, typename T> struct KPagedPoolMap
{
	typedef KPoolMap< K, T, KGuardedMutex, typename KPagedPoolAllocator< KPair<K, T> >::Type > Type;
};

template <typename K, typename T> struct KNonPagedPoolMap
{
	typedef KPoolMap< K, T, KSpinLock, typename KNonPagedPoolAllocator< KPair<K, T> >::Type > Type;
};

template <typename K, typename T, ULONG Tag> struct KTaggedPagedPoolMap
{
	typedef KPoolMap< K, T, KGuardedMutex, typename KTaggedPagedPoolAllocator< KPair<K, T>, Tag >::Type > Type;
};

template <typename K, typename T, ULONG Tag> struct KTaggedNonPagedPoolMap
{
	typedef KPoolMap< K, T, KSpinLock, typename KTaggedNonPagedPoolAllocator< KPair<K, T>, Tag >::Type > Type;
};

template <typename K, typename T, ULONG Tag> struct KPagedLookasideMap
{
	typedef KLookasideMap< K, T, KGuardedMutex, KPagedLookasideAllocator< KPair<K, T>, Tag > > Type;
};

template <typename K, typename T, ULONG Tag> struct KNonPagedLookasideMap
{
	typedef KLookasideMap< K, T, KSpinLock, KNonPagedLookasideAllocator< KPair<K, T>, Tag > > Type;
};