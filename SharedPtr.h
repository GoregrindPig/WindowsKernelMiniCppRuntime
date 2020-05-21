#pragma once

#include "CommonDefinitions.h"
#include "KernelNew.h"
#include "Utility.h"

static const ULONG sharedPtrCounterTag = 'CPhS';

template
<
	typename Type,
	POOL_TYPE Pool,
	typename Deleter = KDefaultDelete<Type>,
	typename Allocator = KDefaultNew<sharedPtrCounterTag, Pool, Type>
> class KSharedPtr
{
private:
	class Counter_t
	{
	public:
		volatile LONG m_count;

		Counter_t() : m_count(1)
		{
		}

		Counter_t(const Counter_t&) {}
		~Counter_t() {}

		Counter_t& operator = (const Counter_t&) {};

		void Reference() 
		{
			InterlockedIncrement(&m_count);
		}

		// True means that owning object must be deleted.
		bool Dereference()
		{
			return InterlockedDecrement(&m_count) == 0;
		}
	};

private:
	bool m_valid;
	Counter_t* m_counter;
	Type* m_obj;
	Deleter m_deleter;
	Allocator m_allocator;

private:
	template<typename OtherType, POOL_TYPE Pool, typename Deleter, typename Allocator> friend class KSharedPtr;

private:
	// True means the owning object has been deleted.
	__drv_freesMem(Counter_t)
	bool Cleanup()
	{
		bool deleted = false;

		if (m_counter && m_counter->Dereference())
		{
			m_deleter(m_obj);
			delete m_counter;
			deleted = true;
		}

		ZeroOut();

		return deleted; 
	}

	void ZeroOut()
	{
		InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_obj), NULL);
		InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_counter), NULL);
		m_valid = false;
	}

	__drv_allocatesMem(Counter_t)
	void Init(__in Type* obj)
	{
		m_counter = new (sharedPtrCounterTag, Pool) Counter_t;
		ASSERT(m_counter);

		// Despite the assertion on mamory allocation failure we set object state invalid and return control.
		// This is because of "Low Resource Simulation" option of Driver Verifier. Once the option set and
		// an allocation fails no other operations can be performed with this object.
		// We don't throw an exception because this is kernel mode code which can be run at high IRQL.
		// Nothe that current class doesn't verify validity exactly delegating it to the caller.
		// Such behaviour is because we don't want to eventually cover an error, yet to catch it by occasion.
		if (!m_counter)
		{
			ZeroOut();
			return;
		}
	
		m_obj = obj;
		m_valid = true;
	}

	template<typename OtherType, POOL_TYPE Pool, typename Deleter, typename Allocator>
	void Init(__in const KSharedPtr<OtherType, Pool, Deleter, Allocator>& other)
	{
		if (other.m_counter)
		{
			InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_counter), other.m_counter);
		}
		else
		{
			m_counter = new (sharedPtrCounterTag, Pool) Counter_t;
			ASSERT(m_counter);

			// Additional verification, for more details see comment to function overload above.
			if (!m_counter)
			{
				ZeroOut();
				return;
			}
		}

		InterlockedExchangePointer(reinterpret_cast<volatile PVOID*>(&m_obj), other.m_obj);
		m_counter->Reference();
		m_valid = true;
	}

	void Init(__in const KSharedPtr& other)
	{
		Init<Type, Pool, Deleter, Allocator>(other);
	}

	template<typename OtherType, POOL_TYPE Pool, typename Deleter, typename Allocator>
	void Reset(__in const KSharedPtr<OtherType, Pool, Deleter, Allocator>& other)
	{
		Cleanup();
		Init(other);
	}

	void Reset(__in const KSharedPtr& other)
	{
		Cleanup();
		Init<Type, Pool, Deleter, Allocator>(other);
	}

public:
	explicit KSharedPtr(Type* obj = NULL)
		: m_valid(true)
		, m_counter(NULL)
		, m_obj(NULL)
	{
		Init(obj);
	}

	KSharedPtr(const KSharedPtr& other)
	{
		Init(other);
	}

	KSharedPtr& operator = (const KSharedPtr& other)
	{
		if (this != &other)
			Reset(other);

		return *this;
	}

	explicit KSharedPtr(const Deleter& del, Type* obj = NULL)
		: m_valid(true)
		, m_counter(NULL)
		, m_obj(NULL)
	{
		m_deleter = del;
		Init(obj);
	}

	explicit KSharedPtr(const Deleter& del, const Allocator& alloc)
		: m_valid(true)
		, m_counter(NULL)
		, m_obj(NULL)
	{
		m_deleter = del;
		m_allocator = alloc;

		Type* obj = m_allocator();
		ASSERT(obj);
		Init(obj);
	}

	template<typename OtherType, POOL_TYPE Pool, typename Deleter, typename Allocator>
	KSharedPtr(const KSharedPtr<OtherType, Pool, Deleter, Allocator>& other)
	{
		Init<OtherType, Pool, Deleter, Allocator>(other);
	}

	template<typename OtherType, POOL_TYPE Pool, typename Deleter, typename Allocator>
	KSharedPtr& operator = (const KSharedPtr<OtherType, Pool, Deleter, Allocator>& other)
	{
		if(reinterpret_cast<const KSharedPtr*>(&other) != this) 
			Reset<OtherType, Pool, Deleter, Allocator>(other);

		return *this;
	}

	~KSharedPtr()
	{ 
		Cleanup();
	}

	bool IsValid() const { return m_valid; }

	Type& operator*()  const { return *m_obj; }

	Type* operator->() const { return m_obj; }

	operator bool() const { return m_obj != NULL; }

	Type* Get() const { return m_obj; }

	Type* Release()
	{ 
		Type* obj = m_obj;
		return (Cleanup()) ? NULL : obj;
	}

	void Reset(__in Type* obj)
	{
		if(m_obj == obj)
			return;
		
		Cleanup();
		Init(obj);
	}

	LONG GetRefCount()
	{
		return m_counter->m_count;
	}
};

template <typename T, typename U, POOL_TYPE Pool, typename Deleter, typename Allocator>
KSharedPtr<T, Pool> StaticPointerCast(__in const KSharedPtr<U, Pool, Deleter, Allocator>& sp)
{
	return static_cast<T*>(sp.Get());
}