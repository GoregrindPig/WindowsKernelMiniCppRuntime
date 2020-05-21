#pragma once

#include "CommonDefinitions.h"

#pragma warning(disable: 4522)

template<typename Type> class KAutoPtr
{
	mutable Type* m_obj;

	template<typename OtherType> friend class KAutoPtr;

private:
	void Cleanup()
	{
		delete m_obj;
		m_obj = NULL;
	}

public:
	explicit KAutoPtr(Type* obj = NULL) : m_obj(obj) {}

	KAutoPtr(KAutoPtr& other)
		: m_obj(other.Release())
	{
	}

	template<typename OtherType> KAutoPtr(KAutoPtr<OtherType>& other)
		: m_obj(other.Release())
	{
	}

	KAutoPtr& operator=(const KAutoPtr& other)
	{
		return operator = (const_cast< KAutoPtr& >(other));
	}

	KAutoPtr& operator=(KAutoPtr& other)
	{
		if (reinterpret_cast<void*>(&other) != reinterpret_cast<void*>(this))
		{
			Cleanup();
			m_obj = other.Release();
		}

		return *this;
	}

	template<typename OtherType> KAutoPtr& operator=(const KAutoPtr<OtherType>& other)
	{
		return operator = (const_cast< KAutoPtr<OtherType>& >(other));
	}

	template<typename OtherType> KAutoPtr& operator=(KAutoPtr<OtherType>& other)
	{
		if (reinterpret_cast<void*>(&other) != reinterpret_cast<void*>(this))
		{
			Cleanup();
			m_obj = other.Release();
		}

		return *this;
	}

	~KAutoPtr()
	{ 
		Cleanup();
	}

	Type& operator*()  const { return *m_obj; }

	Type* operator->() const { return m_obj; }

	operator bool() const { return (m_obj != NULL); }

	Type* Get() const { return m_obj; }

	Type* Release() const
	{
		Type* obj = m_obj;
		m_obj = NULL;
		return obj;
	}

	void Reset(Type* obj)
	{
		if(m_obj != obj)
			Cleanup();
		m_obj = obj;
	}
};
