#pragma once

#include "CommonDefinitions.h"
#include "KernelNew.h"

#pragma warning(disable: 4100)

// Generic allocator class to be base for various concrete allocators.
template <typename T, class ConcreteAllocator> class KAllocator
{
public:
	typedef size_t Size_t;
	typedef ptrdiff_t Dif_t;
	typedef T* Ptr_t;
	typedef const T* CPtr_t;
	typedef T& Ref_t;
	typedef const T& CRef_t;
	typedef T Val_t;

	KAllocator() {}
	KAllocator(const KAllocator&) {}

	~KAllocator() {}

	Ptr_t GetAddress(__in Ref_t val) const
	{
		return &val;
	}

	CPtr_t GetAddress(__in CRef_t val) const
	{
		return &val;
	}

	__checkReturn
	Ptr_t Allocate(__in Size_t num)
	{
		return static_cast<ConcreteAllocator*>(this)->Allocate(num);
	}

	void Construct(__in PVOID p)
	{
		new (p) T;
	}

	void Destroy(__in Ptr_t p)
	{
		p->~T();
	}

	void Deallocate(__in Ptr_t p)
	{
		static_cast<ConcreteAllocator*>(this)->Deallocate(p);
	}
};

template <typename T, POOL_TYPE Pool> class KPoolAllocator  : public KAllocator<T, KPoolAllocator<T, Pool>>
{
public:
	template <typename U> struct Rebind_t
	{
		typedef KPoolAllocator<U, Pool> Other_t;
	};

	KPoolAllocator() {}
	KPoolAllocator(const KPoolAllocator&) {}
	template <typename U, POOL_TYPE Pool> KPoolAllocator(const KPoolAllocator<U, Pool>&) {}
	~KPoolAllocator() {}

	__checkReturn
	__drv_allocatesMem(Ptr_t)
	Ptr_t Allocate(__in Size_t num) 
	{
		Ptr_t p = reinterpret_cast<Ptr_t>(ExAllocatePoolWithTag(Pool, num, 'meMX'));
		if (p)
			memset(p, 0, num);

		return p;
	}

	__drv_freesMem(Ptr_t)
	void Deallocate(__in Ptr_t p) 
	{
		if (p)
			ExFreePool(p);
	}
};

template <typename T> struct KPagedPoolAllocator
{
	typedef KPoolAllocator<T, PagedPool> Type;
};

template <typename T> struct KNonPagedPoolAllocator
{
	typedef KPoolAllocator<T, NonPagedPool> Type;
};

template <typename T, ULONG Tag, POOL_TYPE Pool> class KTaggedPoolAllocator  : public KAllocator<T, KTaggedPoolAllocator<T, Tag, Pool>>
{
public:
	template <typename U> struct Rebind_t
	{
		typedef KTaggedPoolAllocator<U, Tag, Pool> Other_t;
	};

	KTaggedPoolAllocator() {}
	KTaggedPoolAllocator(const KTaggedPoolAllocator&) {}
	template <typename U, ULONG Tag, POOL_TYPE Pool> KTaggedPoolAllocator(const KTaggedPoolAllocator<U, Tag, Pool>&) {}
	~KTaggedPoolAllocator() {}

	__checkReturn
	__drv_allocatesMem(Ptr_t)
	Ptr_t Allocate(__in Size_t num)
	{
		Ptr_t p = reinterpret_cast<Ptr_t>(ExAllocatePoolWithTag(Pool, num, Tag));
		if (p)
			memset(p, 0, num);

		return p;
	}

	__drv_freesMem(Ptr_t)
	void Deallocate(__in Ptr_t p)
	{
		if (p)
			ExFreePoolWithTag(p, Tag);
	}
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolAllocator
{
	typedef KTaggedPoolAllocator<T, Tag, PagedPool> Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolAllocator
{
	typedef KTaggedPoolAllocator<T, Tag, NonPagedPool> Type;
};

template <typename T, ULONG Tag> class KPagedLookasideAllocator  : public KAllocator<T, KPagedLookasideAllocator<T, Tag>>
{
private:
	PPAGED_LOOKASIDE_LIST m_handle;

private:
	KPagedLookasideAllocator(const KPagedLookasideAllocator&){}
	template <typename U, ULONG Tag> KPagedLookasideAllocator(const KPagedLookasideAllocator<U, Tag>&){}

public:
	template <typename U> struct Rebind_t
	{
		typedef KPagedLookasideAllocator<U, Tag> Other_t;
	};

	__drv_maxIRQL(APC_LEVEL)
	__drv_allocatesMem(PPAGED_LOOKASIDE_LIST)
	KPagedLookasideAllocator() : m_handle(NULL)
	{
		m_handle = reinterpret_cast<PPAGED_LOOKASIDE_LIST>(ExAllocatePoolWithTag(PagedPool, sizeof(PAGED_LOOKASIDE_LIST), Tag));
		if (m_handle)
			ExInitializePagedLookasideList(m_handle, NULL, NULL, 0, sizeof(T), Tag, 0);
	}

	__drv_maxIRQL(APC_LEVEL)
	__drv_freesMem(PPAGED_LOOKASIDE_LIST)
	~KPagedLookasideAllocator()
	{
		if (IsValid())
		{
			ExDeletePagedLookasideList(m_handle);
			ExFreePoolWithTag(m_handle, Tag);
		}
		
	}

	bool IsValid() const
	{
		return m_handle != NULL;
	}

	__checkReturn
	__drv_maxIRQL(APC_LEVEL)
	Ptr_t Allocate(__in Size_t num)
	{
		UNREFERENCED_PARAMETER(num);

		if (!IsValid())
			return NULL;

		Ptr_t p = reinterpret_cast<Ptr_t>(ExAllocateFromPagedLookasideList(m_handle));
		if (p)
			memset(p, 0, sizeof(T));

		return p;
	}

	__drv_maxIRQL(APC_LEVEL)
	void Deallocate(__in Ptr_t p)
	{
		ExFreeToPagedLookasideList(m_handle, p);
	}
};

template <typename T, ULONG Tag> class KNonPagedLookasideAllocator : public KAllocator<T, KNonPagedLookasideAllocator<T, Tag>>
{
private:
	PNPAGED_LOOKASIDE_LIST m_handle;

private:
	KNonPagedLookasideAllocator(const KNonPagedLookasideAllocator&){}
	template <class U, ULONG Tag> KNonPagedLookasideAllocator(const KNonPagedLookasideAllocator<U, Tag>&){}

public:
	template <typename U> struct Rebind_t
	{
		typedef KNonPagedLookasideAllocator<U, Tag> Other_t;
	};

	__drv_maxIRQL(DISPATCH_LEVEL)
	__drv_allocatesMem(PNPAGED_LOOKASIDE_LIST)
	KNonPagedLookasideAllocator() : m_handle(NULL)
	{
		m_handle = reinterpret_cast<PNPAGED_LOOKASIDE_LIST>(ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), Tag));
		if (m_handle)
			ExInitializeNPagedLookasideList(m_handle, NULL, NULL, 0, sizeof(T), Tag, 0);
	}

	__drv_maxIRQL(DISPATCH_LEVEL)
	__drv_freesMem(PNPAGED_LOOKASIDE_LIST)
	~KNonPagedLookasideAllocator()
	{
		if (IsValid())
		{
			ExDeleteNPagedLookasideList(m_handle);
			ExFreePoolWithTag(m_handle, Tag);
		}

	}

	bool IsValid() const
	{
		return m_handle != NULL;
	}

	__checkReturn
	__drv_maxIRQL(DISPATCH_LEVEL)
	Ptr_t Allocate(__in Size_t num)
	{
		UNREFERENCED_PARAMETER(num);

		if (!IsValid())
			return NULL;

		Ptr_t p = reinterpret_cast<Ptr_t>(ExAllocateFromNPagedLookasideList(m_handle));
		if (p)
			memset(p, 0, sizeof(T));

		return p;
	}

	__drv_maxIRQL(DISPATCH_LEVEL)
	void Deallocate(__in Ptr_t p)
	{
		ExFreeToNPagedLookasideList(m_handle, p);
	}
};