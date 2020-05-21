#pragma once

#include "CommonDefinitions.h"
#include "List.h"

template < typename T, typename Holder > class KQueue
{
	CLASS_NO_COPY(KQueue)
public:
	typedef typename Holder::Val_t Val_t;
	typedef typename Holder::Ref_t Ref_t;
	typedef typename Holder::CRef_t CRef_t;
	typedef typename Holder::Ptr_t Ptr_t;
	typedef typename Holder::CPtr_t CPtr_t;
	typedef ptrdiff_t Dif_t;
	typedef size_t Size_t;

	KQueue()
	{}

	void Cleanup()
	{
		m_holder.Cleanup();
	}

	Size_t GetSize()
	{
		return m_holder.GetSize();
	}

	bool IsEmpty()
	{
		return m_holder.IsEmpty();
	}

	void Push(CRef_t entry)
	{
		m_holder.InsertLast(entry);
	}

	Val_t Pop()
	{
		return m_holder.RemoveFirst();
	}

private:
	Holder m_holder;
};

template <typename T> struct KPagedPoolListQueue
{
	typedef KQueue< T, KList< T, typename KPagedPoolAllocator< T >::Type > > Type;
};

template <typename T> struct KNonPagedPoolListQueue
{
	typedef KQueue< T, KList< T, typename KNonPagedPoolAllocator< T >::Type > > Type;
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolListQueue
{
	typedef KQueue< T, KList< T, typename KTaggedPagedPoolAllocator< T, Tag >::Type > > Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolListQueue
{
	typedef KQueue< T, KList< T, typename KTaggedNonPagedPoolAllocator< T, Tag >::Type > > Type;
};

template <typename T, ULONG Tag> struct KPagedLookasideListQueue
{
	typedef KQueue< T, KList< T, KPagedLookasideAllocator< T, Tag > > > Type;
};

template <typename T, ULONG Tag> struct KNonPagedLookasideListQueue
{
	typedef KQueue< T, KList< T, KNonPagedLookasideAllocator< T, Tag > > > Type;
};
