#pragma once

template <typename T1, typename T2> struct KPair
{
	typedef T1 First_t;
	typedef T2 Second_t;

	First_t first;
	Second_t second;

	KPair()
		: first()
		, second()
	{

	}

	~KPair() {}

	KPair(const KPair& other) 
		: first(other.first)
		, second(other.second)
	{
	}

	KPair(const First_t& a, const Second_t& b)
		: first(a)
		, second(b)
	{
	}

	template<typename U, typename V> KPair(const KPair<U, V>& other)
		: first(other.first)
		, second(other.second)
	{
	}

	KPair& operator = (const KPair& other)
	{
		if (this != &other)
		{
			first = other.first;
			second = other.second;
		}
		
		return *this;
	}

	template<typename U, typename V> KPair& operator = (const KPair<U, V>& other)
	{
		if (this != &other)
		{
			first = other.first;
			second = other.second;
		}

		return *this;
	}
};

template <typename T1, typename T2> bool operator == (const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return (lhs.first == rhs.first) && (lhs.second == rhs.second);
}

template <typename T1, typename T2> bool operator != (const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return !(lhs == rhs);
}

template <typename T1, typename T2> bool operator <  (const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return (lhs.first < rhs.first) || (!(rhs.first < lhs.first) && lhs.second < rhs.second);
}

template <typename T1, typename T2> bool operator <= (const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return !(rhs < lhs);
}

template <typename T1, typename T2> bool operator >(const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return rhs < lhs;
}

template <typename T1, typename T2> bool operator >= (const KPair<T1, T2>& lhs, const KPair<T1, T2>& rhs)
{
	return !(lhs < rhs);
}

template <typename T1, typename T2> KPair<T1, T2> MakePair(T1 x, T2 y)
{
	return (KPair<T1, T2>(x, y));
}

template <typename T> void Swap(T& source, T& dest)
{
	T temp(source);
	source = dest;
	dest = temp;
}

template <ULONG Tag, POOL_TYPE Pool, typename Type> struct KDefaultNew
{
	Type* operator()()
	{
		return new (Tag, Pool) Type;
	}
};

template <typename Type> struct KDefaultDelete
{
	void operator()(Type* obj)
	{
		delete obj;
	}
};
