#pragma once

#include "CommonDefinitions.h"
#include "Synch.h"
#include "Allocator.h"

template < typename T, typename Alloc > class KForwardList
{
	CLASS_NO_COPY(KForwardList)
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
		SINGLE_LIST_ENTRY link;
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
		PSINGLE_LIST_ENTRY m_current;
	public:
		IterBase_t(PSINGLE_LIST_ENTRY current)
			: m_current(current)
		{
		}

		IterBase_t(const IterBase_t& other)
			: m_current(other.m_current)
		{
		}

		Derived& operator = (const IterBase_t& other)
		{
			if (this != &other)
				m_current = other.m_current;
			return static_cast<Derived&>(*this);
		}

		bool operator == (const IterBase_t& other) const
		{
			return m_current == other.m_current;
		}

		bool operator != (const IterBase_t& other) const
		{
			return m_current != other.m_current;
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
			PSINGLE_LIST_ENTRY tmp = m_current;
			for (Dif_t i = 0; i < n; i++)
			{
				tmp = tmp->Next;
				if (!tmp)
					break;
			}
			m_current = tmp;

			return static_cast<Derived&>(*this);
		}

		Derived& Retreat(Dif_t n)
		{
			return static_cast<Derived&>(*this);
		}

		DEFINE_INCDEC_FORWARD(Derived);
	};

	class Iter_t : public IterBase_t<Iter_t>
	{
		friend class KForwardList;
	public:
		Iter_t(PSINGLE_LIST_ENTRY current)
			: IterBase_t(current)
		{
		}

		Iter_t(const Iter_t& other)
			: IterBase_t(other.m_current)
		{
		}

		Iter_t& operator = (const Iter_t& other)
		{
			return IterBase_t::operator= (other);
		}

		Iter_t& operator++()
		{
			ASSERT(m_current);
			m_current = m_current->Next;
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
	ITER_INC(IterRef_t, Dif_t, false);

	explicit KForwardList()
	{
		m_anchor.Next = NULL;
	}

	~KForwardList()
	{
		Cleanup();
	}

	Iter_t Begin()
	{
		Iter_t iterator(m_anchor.Next);
		return iterator;
	}

	Iter_t End()
	{
		Iter_t iterator(NULL);
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
		return m_anchor.Next == NULL;
	}

	void Cleanup()
	{
		while (!IsEmpty())
			Pop();
	}

	void Push(const T& obj)
	{
		Item_t* item = Prepare(obj);
		PushEntryList(&m_anchor, &item->link);
	}

	Val_t Pop()
	{
		PSINGLE_LIST_ENTRY entry = PopEntryList(&m_anchor);
		return Delete(entry);
	}

	Iter_t EraseAfter(CIterRef_t pos)
	{
		ItemPtr_t posItem = CONTAINING_RECORD(pos.m_current, Item_t, link);
		PSINGLE_LIST_ENTRY prevEntry = &m_anchor;
		PSINGLE_LIST_ENTRY entry = m_anchor.Next;

		for (; entry; prevEntry = prevEntry->Next, entry = entry->Next)
		{
			ItemPtr_t curItem = CONTAINING_RECORD(entry, Item_t, link);
			if ((entry == &posItem->link) && (curItem->object == posItem->object))
			{
				PSINGLE_LIST_ENTRY nextEntry = entry->Next;
				prevEntry->Next = nextEntry;
				Delete(entry);

				return Iter_t(nextEntry);
			}
		}

		return Iter_t(NULL);
	}

private:
	ItemPtr_t Prepare(const T& obj)
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

	Val_t Delete(PSINGLE_LIST_ENTRY entry)
	{
		ASSERT(entry);
		ItemPtr_t item = CONTAINING_RECORD(entry, Item_t, link);
		Val_t val = item->object;
		m_allocator.Destroy(item);
		m_allocator.Deallocate(item);

		return val;
	}

private:
	typedef typename Alloc::template Rebind_t<Item_t>::Other_t ItemAlloc_t;

private:
	ItemAlloc_t m_allocator;
	SINGLE_LIST_ENTRY m_anchor;
};

template <typename T> struct KPagedPoolForwardList
{
	typedef KForwardList< T, typename KPagedPoolAllocator< T >::Type > Type;
};

template <typename T> struct KNonPagedPoolForwardList
{
	typedef KForwardList< T, typename KNonPagedPoolAllocator< T >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolForwardList
{
	typedef KForwardList< T, typename KTaggedPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolForwardList
{
	typedef KForwardList< T, typename KTaggedNonPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KPagedLookasideForwardList
{
	typedef KForwardList< T, typename KPagedLookasideAllocator< T, Tag > > Type;
};

template <typename T, ULONG Tag> struct KNonPagedLookasideForwardList
{
	typedef KForwardList< T,  KNonPagedLookasideAllocator< T, Tag > > Type;
};
