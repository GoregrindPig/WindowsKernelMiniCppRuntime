#pragma once

#include "CommonDefinitions.h"
#include "Allocator.h"

template < typename T, typename Alloc > class KList
{
	CLASS_NO_COPY(KList)
public:
	typedef typename Alloc::Val_t Val_t;
	typedef typename Alloc::Ref_t Ref_t;
	typedef typename Alloc::CRef_t CRef_t;
	typedef typename Alloc::Ptr_t Ptr_t;
	typedef typename Alloc::CPtr_t CPtr_t;
	typedef ptrdiff_t Dif_t;
	typedef size_t Size_t;

	struct Item_t
	{
		LIST_ENTRY link;
		T object;

		Item_t() 
			: link()
			, object()
		{

		}

		Item_t(const Item_t& other)
			: link(other.link)
			, object(other.object)
		{

		}

		~Item_t() {}

		Item_t& operator = (const Item_t& other)
		{
			if (this != &other)
			{
				link = other.link;
				object = other.object;
			}

			return *this;
		}
	};

	typedef Item_t* ItemPtr_t;

	template <typename Derived> class IterBase_t
	{
	protected:
		PLIST_ENTRY m_anchor;
		PLIST_ENTRY m_current;

	public:
		IterBase_t(PLIST_ENTRY anchor, PLIST_ENTRY current)
			: m_anchor(anchor)
			, m_current(current)
		{
		}

		IterBase_t(const IterBase_t& other)
			: m_anchor(other.m_anchor)
			, m_current(other.m_current)
		{
		}

		Derived& operator = (const IterBase_t& other)
		{
			if (this != &other)
			{
				m_anchor = other.m_anchor;
				m_current = other.m_current;
			}

			return static_cast<Derived&>(*this);
		}

		bool operator == (const IterBase_t& other) const
		{
			return (m_anchor == other.m_anchor) && (m_current == other.m_current);
		}

		bool operator != (const IterBase_t& other) const
		{
			return (m_anchor != other.m_anchor) || (m_current != other.m_current);
		}

		Ref_t operator * ()
		{
			ItemPtr_t item = CONTAINING_RECORD(m_current, Item_t, link);
			return item->object;
		}

		Ptr_t operator -> ()
		{
			ItemPtr_t item = CONTAINING_RECORD(m_current, Item_t, link);
			return &item->object;
		}

		Derived& Advance(Dif_t n)
		{
			ASSERT(m_current);
			PLIST_ENTRY tmp = m_current;
			ASSERT(tmp != m_anchor);
			for (Dif_t i = 0; i < n; i++)
			{
				tmp = tmp->Flink;
				if (tmp == m_anchor)
					break;
			}
			m_current = tmp;

			return static_cast<Derived&>(*this);
		}

		Derived& Retreat(Dif_t n)
		{
			ASSERT(m_current);
			PLIST_ENTRY tmp = m_current;
			ASSERT(tmp != m_anchor);
			for (Dif_t i = 0; i < n; i++)
			{
				tmp = tmp->Blink;
				if (tmp != m_anchor)
					break;
			}
			m_current = tmp;

			return static_cast<Derived&>(*this);
		}

		DEFINE_INCDEC_BOTH(Derived);
	};

	class Iter_t : public IterBase_t<Iter_t>
	{
		friend class KList;
	public:
		Iter_t(PLIST_ENTRY anchor, PLIST_ENTRY current)
			: IterBase_t(anchor, current)
		{
		}

		Iter_t(const Iter_t& other)
			: IterBase_t(other)
		{
		}

		Iter_t& operator = (const Iter_t& other)
		{
			return IterBase_t::operator= (other);
		}

		Iter_t& operator++()
		{
			ASSERT(m_current);
			m_current = m_current->Flink;
			return *this;
		}

		Iter_t operator++(int)
		{
			Iter_t tmp(m_current);
			operator++();
			return tmp;
		}
	};

	ITER_TYPEDEF(Iter);
	ITER_INC_DEC(Iter_t, Dif_t, false);

	class RevIter_t : public IterBase_t<RevIter_t>
	{
		friend class KList;
	public:
		RevIter_t(PLIST_ENTRY anchor, PLIST_ENTRY current)
			: IterBase_t(anchor, current)
		{
		}

		RevIter_t(const RevIter_t& other)
			: IterBase_t(other)
		{
		}

		RevIter_t& operator = (const RevIter_t& other)
		{
			return IterBase_t::operator= (other);
		}

		RevIter_t& operator--()
		{
			ASSERT(m_current);
			m_current = m_current->Blink;
			return *this;
		}

		RevIter_t operator--(int)
		{
			RevIter_t tmp(m_current);
			operator--();
			return tmp;
		}
	};

	ITER_TYPEDEF(RevIter);
	ITER_INC_DEC(RevIter_t, Dif_t, true);

	explicit KList()
	{
		InitializeListHead(&m_anchor);
	}

	~KList()
	{
		Cleanup();
	}

	Iter_t Begin()
	{
		Iter_t iterator(&m_anchor, m_anchor.Flink);
		return iterator;
	}

	Iter_t End()
	{
		Iter_t iterator(&m_anchor, &m_anchor);
		return iterator;
	}

	RevIter_t RBegin()
	{
		RevIter_t iterator(&m_anchor, m_anchor.Blink);
		return iterator;
	}

	RevIter_t REnd()
	{
		RevIter_t iterator(&m_anchor, &m_anchor);
		return iterator;
	}

	Size_t GetSize()
	{
		Size_t res = 0;
		for (Iter_t it = Begin(); it != End(); ++it)
			res++;

		return res;
	}

	bool IsEmpty()
	{
		return IsListEmpty(&m_anchor) == TRUE;
	}

	void Cleanup()
	{
		while (!IsEmpty())
			RemoveFirst();
	}

	void InsertFirst(CRef_t obj)
	{
		Item_t* item = Prepare(obj);
		InsertHeadList(&m_anchor, &item->link);
	}

	void InsertLast(CRef_t obj)
	{
		Item_t* item = Prepare(obj);
		InsertTailList(&m_anchor, &item->link);
	}

	void InsertBefore(const Iter_t& it, CRef_t obj)
	{
		Item_t* item = Prepare(obj);
		InsertHeadList(it.m_current, &item->link);
	}

	void InsertAfter(const Iter_t& it, CRef_t obj)
	{
		Item_t* item = Prepare(obj);
		InsertTailList(it.m_current, &item->link);
	}

	Val_t RemoveFirst()
	{
		PLIST_ENTRY entry = RemoveHeadList(&m_anchor);
		return RemoveAt(entry);
	}

	Val_t RemoveLast()
	{
		PLIST_ENTRY entry = RemoveTailList(&m_anchor);
		return RemoveAt(entry);
	}

	bool Remove(CRef_t val)
	{
		PLIST_ENTRY target = NULL;

		for (Iter_t it = Begin(); it != End(); ++it)
		{
			Ref_t obj = *it;
			if (obj == val)
			{
				target = it.m_current;
				break;
			}
		}

		ASSERT(target);
		RemoveEntryList(target);
		RemoveAt(target);

		return target != NULL;
	}

	Val_t Erase(CIterRef_t it)
	{
		return RemoveEntry(it.m_current);
	}

	Val_t Erase(CRevIterRef_t it)
	{
		return RemoveEntry(it.m_current);
	}

private:
	ItemPtr_t Prepare(CRef_t obj)
	{
		Size_t num = sizeof(Item_t);
		ItemPtr_t item = m_allocator.Allocate(num);
		ASSERT(item);

		if (!item)
			return NULL;

		m_allocator.Construct(item);
		item->object = obj;

		return item;
	}

	Val_t RemoveAt(PLIST_ENTRY entry)
	{
		ASSERT(entry);
		ItemPtr_t item = CONTAINING_RECORD(entry, Item_t, link);
		Val_t val = item->object;
		m_allocator.Destroy(item);
		m_allocator.Deallocate(item);

		return val;
	}

	Val_t RemoveEntry(PLIST_ENTRY entry)
	{
		if (!entry)
			return Val_t();
		
		RemoveEntryList(entry);
		return RemoveAt(entry);
	}

private:
	typedef typename Alloc::template Rebind_t<Item_t>::Other_t ItemAlloc_t;

private:
	ItemAlloc_t m_allocator;
	LIST_ENTRY m_anchor;
};

template <typename T> struct KPagedPoolList
{
	typedef KList< T, typename KPagedPoolAllocator< T >::Type > Type;
};

template <typename T> struct KNonPagedPoolList
{
	typedef KList< T, typename KNonPagedPoolAllocator< T >::Type > Type;
};

template <typename T, ULONG Tag> struct KPagedLookasideList
{
	typedef KList< T, KPagedLookasideAllocator< T, Tag > > Type;
};

template <typename T, ULONG Tag> struct KNonPagedLookasideList
{
	typedef KList< T, KNonPagedLookasideAllocator< T, Tag > > Type;
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolList
{
	typedef KList< T, typename KTaggedPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolList
{
	typedef KList< T, typename KTaggedNonPagedPoolAllocator< T, Tag >::Type > Type;
};
