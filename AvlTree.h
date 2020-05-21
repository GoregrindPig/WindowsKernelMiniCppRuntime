#pragma once

#include "CommonDefinitions.h"
#include "Synch.h"
#include "Allocator.h"

template <typename ConcreteTree, typename Lock, typename Alloc> class KAvlTree : public RTL_AVL_TABLE
{
	CLASS_NO_COPY(KAvlTree)
public:
	typedef Alloc Alloc_t;
	typedef typename Alloc::Val_t Val_t;
	typedef typename Alloc::Ref_t Ref_t;
	typedef typename Alloc::CRef_t CRef_t;
	typedef typename Alloc::Ptr_t Ptr_t;
	typedef typename Alloc::CPtr_t CPtr_t;
	typedef ptrdiff_t Dif_t;
	typedef size_t Size_t;

	class Iter_t
	{
		friend class KAvlTree;

		KAvlTree* m_target;
		Ptr_t m_current;

	public:
		Iter_t(KAvlTree* target)
			: m_target(target)
			, m_current(NULL)
		{
		}

		Iter_t(const Iter_t& other)
			: m_target(other.m_target)
			, m_current(other.m_current)
		{
		}

		Iter_t& operator++()
		{
			m_current = reinterpret_cast<Ptr_t>(RtlEnumerateGenericTableAvl(m_target, m_current == NULL));
			return *this;
		}

		Iter_t operator++(int)
		{
			Iter_t tmp(*this);
			operator++();
			return tmp;
		}

		bool operator == (const Iter_t& other) const
		{
			return m_current == other.m_current;
		}

		bool operator != (const Iter_t& other) const
		{
			return m_current != other.m_current;
		}

		Ref_t operator * ()
		{
			return *m_current;
		}

		Ptr_t operator -> ()
		{
			return m_current;
		}
	};

	explicit KAvlTree()
	{
		RtlInitializeGenericTableAvl(this, &KAvlTree::CompareRoutine, &KAvlTree::AllocateRoutine, &KAvlTree::FreeRoutine, this);
	}

	~KAvlTree() {}

	__drv_mustHold(Lock)
	void Cleanup()
	{
		KLocker<Lock> locker(m_lock);
		for (PVOID p = RtlEnumerateGenericTableAvl(this, TRUE); p;
			p = RtlEnumerateGenericTableAvl(this, FALSE))
		{
			RtlDeleteElementGenericTableAvl(this, p);
		}
	}

	__drv_mustHold(Lock)
	Size_t GetSize()
	{
		KLocker<Lock> locker(m_lock);
		ULONG len = RtlNumberGenericTableElementsAvl(this);
		return len;
	}

	bool IsEmpty()
	{
		return GetSize() == 0;
	}

	__checkReturn_opt
	__drv_mustHold(Lock)
	bool Insert(__in CRef_t val, __in Ptr_t* res = NULL)
	{
		KLocker<Lock> locker(m_lock);
		BOOLEAN inserted = FALSE;
		PVOID raw = RtlInsertElementGenericTableAvl(this, reinterpret_cast<PVOID>(const_cast<Ptr_t>(&val)), sizeof(val), &inserted);
		if (!raw)
			return false;

		// Since we deal with C style function accepting flat buffer no guarantee that new item initialized properly.
		// Thus apply copy constructor to fulfill new item's initialization.
		if (inserted)
			static_cast<ConcreteTree*>(this)->OnInsert(raw, val);
		else
			return false;

		// Return new item if the caller interested therein.
		if (res)
			*res = reinterpret_cast<Ptr_t>(raw);

		return true;
	}

	__checkReturn_opt
	__drv_mustHold(Lock)
	bool Erase(__in CRef_t val)
	{
		KLocker<Lock> locker(m_lock);
		BOOLEAN deleted = RtlDeleteElementGenericTableAvl(this, reinterpret_cast<PVOID>(const_cast<Ptr_t>(&val)));

		return deleted == TRUE;
	}

	__checkReturn
	Iter_t Find(__in CRef_t val)
	{
		Ptr_t item = Lookup(val);
		Iter_t iterator(this);
		iterator.m_current = (item) ? item : NULL;

		return iterator;
	}

	Iter_t Begin()
	{
		Iter_t iterator(this);
		iterator.m_current = reinterpret_cast<Ptr_t>(RtlEnumerateGenericTableAvl(this, TRUE));
		return iterator;
	}

	Iter_t End()
	{
		Iter_t iterator(this);
		return iterator;
	}

	Lock& GetLock()
	{
		return m_lock;
	}

protected:
	__checkReturn
	Ptr_t Lookup(__in CRef_t val)
	{
		Ptr_t item = reinterpret_cast<Ptr_t>(RtlLookupElementGenericTableAvl(this, reinterpret_cast<PVOID>(const_cast<Ptr_t>(&val))));
		return item;
	}

	__checkReturn
	static RTL_GENERIC_COMPARE_RESULTS CompareRoutine(__in PRTL_AVL_TABLE self, __in PVOID first, __in PVOID second)
	{
		return static_cast<ConcreteTree*>(self)->OnCompare(*reinterpret_cast<Ptr_t>(first),
			*reinterpret_cast<Ptr_t>(second));
	}

	__checkReturn
	__drv_allocatesMem(PVOID)
	static PVOID AllocateRoutine(__in PRTL_AVL_TABLE self, __in CLONG byteSize)
	{
		if (!byteSize)
			return NULL;

		return static_cast<ConcreteTree*>(self)->OnAllocate(byteSize);
	}

	__drv_freesMem(PVOID)
	static VOID FreeRoutine(__in PRTL_AVL_TABLE self, __in PVOID buf)
	{
		if (!buf)
			return;

		static_cast<ConcreteTree*>(self)->OnFree(buf);
	}

protected:
	Lock m_lock;
};

template <typename Alloc> class KAvlTreePoolEventSink
{
	CLASS_NO_COPY(KAvlTreePoolEventSink)
public:
	typedef typename Alloc::Val_t Val_t;
	typedef typename Alloc::Ref_t Ref_t;
	typedef typename Alloc::CRef_t CRef_t;
	typedef typename Alloc::Ptr_t Ptr_t;
	typedef typename Alloc::CPtr_t CPtr_t;
	typedef size_t Size_t;

	KAvlTreePoolEventSink(Alloc& allocator) : m_allocator(allocator) {}
	~KAvlTreePoolEventSink() {}

	__checkReturn
	__drv_allocatesMem(PVOID)
	PVOID OnAllocate(__in Size_t num)
	{
		PVOID buf = m_allocator.Allocate(num);
		if (!buf)
			return NULL;

		// Advance the pointer to access payload leaving behind AVL tree's internal structures.
		Ptr_t p = reinterpret_cast<Ptr_t>(reinterpret_cast<PUCHAR>(buf) + sizeof(RTL_BALANCED_LINKS));
		m_allocator.Construct(p);

		return buf;
	}

	__drv_freesMem(PVOID)
	void OnFree(__in PVOID buf)
	{
		Ptr_t p = reinterpret_cast<Ptr_t>(reinterpret_cast<PUCHAR>(buf) + sizeof(RTL_BALANCED_LINKS));
		m_allocator.Destroy(p);
		m_allocator.Deallocate(reinterpret_cast<Ptr_t>(buf));
	}

	void OnInsert(__in PVOID raw, __in CRef_t val)
	{
		m_allocator.Construct(raw);
		Ref_t inst = *reinterpret_cast<Ptr_t>(raw);
		inst = val;
	}

protected:
	Alloc& m_allocator;
};

template <typename Alloc> class KAvlTreeLookasideEventSink
{
	CLASS_NO_COPY(KAvlTreeLookasideEventSink)
public:
	typedef typename Alloc::Val_t Val_t;
	typedef typename Alloc::Ref_t Ref_t;
	typedef typename Alloc::CRef_t CRef_t;
	typedef typename Alloc::Ptr_t Ptr_t;
	typedef typename Alloc::CPtr_t CPtr_t;
	typedef size_t Size_t;

	KAvlTreeLookasideEventSink()
		: m_allocator(NULL) {}
	~KAvlTreeLookasideEventSink() {}

	struct Item_t
	{
		RTL_BALANCED_LINKS link;
		Val_t object;

		Item_t()
			: link()
			, object()
		{

		}

		~Item_t() {}

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
	typedef typename Alloc::template Rebind_t<Item_t>::Other_t* ItemAlloc_ptr;

	__checkReturn
	__drv_allocatesMem(PVOID)
	PVOID OnAllocate(__in Size_t num)
	{
		ItemPtr_t p = m_allocator->Allocate(num);
		return p;
	}

	__drv_freesMem(PVOID)
	void OnFree(__in PVOID buf)
	{
		ItemPtr_t p = reinterpret_cast<ItemPtr_t>(buf);
		m_allocator->Destroy(p);
		m_allocator->Deallocate(p);
	}

	void OnInsert(__in PVOID raw, __in CRef_t val)
	{
		Ptr_t p = new (raw)Val_t();
		Ref_t inst = *p;
		inst = val;
	}

protected:
	void Reset(typename Alloc::template Rebind_t<Item_t>::Other_t* allocator)
	{
		ASSERT(allocator);
		m_allocator = allocator;
	}

private:
	ItemAlloc_ptr m_allocator;
};