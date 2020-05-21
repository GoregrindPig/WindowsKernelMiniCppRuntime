#pragma once

#include "CommonDefinitions.h"
#include "Allocator.h"

#pragma warning(disable: 4100)

template < typename T, typename Alloc > class KVector
{
	CLASS_NO_COPY(KVector)
public:
	typedef typename Alloc::Val_t Val_t;
	typedef typename Alloc::Ref_t Ref_t;
	typedef typename Alloc::CRef_t CRef_t;
	typedef typename Alloc::Ptr_t Ptr_t;
	typedef typename Alloc::CPtr_t CPtr_t;
	typedef ptrdiff_t Dif_t;
	typedef size_t Size_t;

	template <typename Derived> class IterBase_t
	{
	protected:
		T* m_data;
		Size_t m_index;
		Size_t m_size;

	public:
		IterBase_t(T* data, Size_t index, Size_t size)
			: m_data(data)
			, m_index(index)
			, m_size(size)
		{
		}

		IterBase_t(const IterBase_t& other)
			: m_data(other.m_data)
			, m_index(other.m_index)
			, m_size(other.m_size)
		{
		}

		Derived& operator = (const IterBase_t& other)
		{
			if (this != &other)
			{
				m_data = other.m_data;
				m_index = other.m_index;
				m_size = other.m_size;
			}

			return static_cast<Derived&>(*this);
		}

		bool operator == (const IterBase_t& other) const
		{
			return (m_size == other.m_size) &&
				(m_index == other.m_index);
		}

		bool operator != (const IterBase_t& other) const
		{
			return !operator == (other);
		}

		Ref_t operator * ()
		{
			return m_data[m_index];
		}

		Ptr_t operator -> ()
		{
			return &m_data[m_index];
		}

		Derived& Advance(Dif_t n)
		{
			ASSERT(m_size >= m_index + static_cast<Size_t>(n));
			m_index += n;
			return static_cast<Derived&>(*this);
		}

		Derived& Retreat(Dif_t n)
		{
			ASSERT(m_size - m_index >= static_cast<Size_t>(n));
			m_index -= n;
			return static_cast<Derived&>(*this);
		}

		DEFINE_INCDEC_BOTH(Derived);
	};

	class Iter_t : public IterBase_t<Iter_t>
	{
		friend class KVector;
	public:
		Iter_t(T* data, Size_t size, Size_t index = 0)
			: IterBase_t(data, index, size)
		{
		}

		Iter_t(const Iter_t& other)
			: IterBase_t(other)
		{
		}

		Iter_t& operator = (const Iter_t& other)
		{
			return IterBase_t::operator = (other);
		}

		Iter_t& operator++()
		{
			++m_index;
			ASSERT(m_index <= m_size);
			return *this;
		}

		Iter_t operator++(int)
		{
			Iter_t tmp(m_data, m_index, m_size);
			operator++();
			return tmp;
		}
	};

	ITER_TYPEDEF(Iter);
	ITER_INC_DEC(Iter_t, Dif_t, false);

	class RevIter_t : public IterBase_t<RevIter_t>
	{
		friend class KVector;
	public:
		RevIter_t(T* data, int index, Size_t size)
			: IterBase_t(data, index, size)
		{
		}

		RevIter_t(T* data, Size_t size)
			: IterBase_t(data, static_cast<int>(size)-1, size)
		{
		}

		RevIter_t(const Iter_t& other)
			: IterBase_t(other)
		{
		}

		RevIter_t& operator = (const Iter_t& other)
		{
			return IterBase_t::operator = (other);
		}

		RevIter_t& operator--()
		{
			--m_index;
			return *this;
		}

		RevIter_t operator--(int)
		{
			RevIter_t tmp(m_data, m_index, m_size);
			operator--();
			return tmp;
		}
	};

	ITER_TYPEDEF(RevIter);
	ITER_INC_DEC(RevIter_t, Dif_t, true);

	explicit KVector()
	{
		Setup();
		Grow(m_size, false);
	}

	~KVector()
	{
		Cleanup();
	}

	Iter_t Begin()
	{
		Iter_t iterator(m_data, m_size);
		return iterator;
	}

	Iter_t End()
	{
		// Points to the memory straigh after the end of data buffer.
		Iter_t iterator(m_data, m_size, m_size);
		return iterator;
	}

	RevIter_t RBegin()
	{
		RevIter_t iterator(m_data, m_size);
		return iterator;
	}

	RevIter_t REnd()
	{
		// Points to the memory straigh before the beginning of data buffer.
		RevIter_t iterator(m_data, -1, m_size);
		return iterator;
	}

	Size_t GetCapacity() const
	{
		return m_capacity;
	}

	Size_t GetSize() const
	{
		return m_size;
	}

	Size_t GetMaxSize() const
	{
		return 0x7FFFFFFF;
	}

	bool IsEmpty() const
	{
		return GetSize() == 0;
	}

	void Cleanup()
	{
		for (Size_t i = 0; i < m_size; i++)
			m_allocator.Destroy(&m_data[i]);
		m_allocator.Deallocate(m_data);

		Setup();
	}

	void Reserve(Size_t newCapacity)
	{
		if (newCapacity > m_capacity)
			Grow(newCapacity);
	}

	void Resize(Size_t newSize, Val_t val = Val_t())
	{
		if (ShouldShrink(newSize))
		{
			Shrink(newSize, val);
		}
		else if (ShouldStandstill(newSize))
		{
			Append(m_data, newSize, val);
			m_size = newSize;
		}
		else if (ShouldGrow(newSize))
		{
			Grow(newSize, true, val);
		}
	}

	T& At(Size_t index)
	{
		ASSERT(IsInRange(index));
		return m_data[index];
	}

	T& operator[] (Size_t index)
	{
		return m_data[index];
	}

	T& Front()
	{
		ASSERT(!IsEmpty())
		return m_data[0];
	}

	T& Back()
	{
		ASSERT(!IsEmpty())
		return m_data[m_size - 1];
	}

	void Assign(Iter_t first, Iter_t last)
	{
		Size_t newSize = static_cast<Size_t>(Distance(first, last));
		Resize(newSize);

		Iter_t it = first;
		for (Size_t i = 0; ((i < m_size) && (it != last)); ++i, ++it)
			m_data[i] = *it;
	}

	void Assign(Size_t newSize, CRef_t val)
	{
		Resize(newSize, val);
	}

	void PushBack(CRef_t val)
	{
		Size_t pos = m_size;
		Size_t newSize = m_size + 1;
		Resize(newSize);

		m_data[pos] = val;
	}

	void PopBack()
	{
		ASSERT(!IsEmpty());
		Size_t newSize;
		Size_t lastIndex;
		newSize = lastIndex = m_size - 1;

		m_allocator.Destroy(m_data[lastIndex]);
		Resize(newSize);
	}

	Iter_t Insert(Iter_t pos, CRef_t val)
	{
		Size_t newSize = m_size + 1;
		Resize(newSize);

		Size_t posIndex = pos.m_index;
		MoveRight(posIndex);
		m_data[posIndex] = val;

		return Iter_t(m_data, m_size, posIndex);
	}

	Iter_t Insert(Iter_t pos, Iter_t first, Iter_t last)
	{
		Size_t count = Distance(first, last);
		Size_t newSize = m_size + count;
		Resize(newSize);

		Size_t lowerBound = pos.m_index;
		Size_t upperBound = lowerBound + count;

		MoveRightRange(lowerBound, upperBound);

		Iter_t it = first;
		for (Size_t i = lowerBound; ((i <= upperBound) && (it != last)); ++i, ++it)
			m_data[i] = *it;

		return Iter_t(m_data, m_size, lowerBound);
	}

	void Insert(Iter_t pos, Size_t count, CRef_t val)
	{
		Size_t newSize = m_size + count;
		Resize(newSize);

		Size_t lowerBound = pos.m_index;
		Size_t upperBound = lowerBound + count;

		MoveRightRange(lowerBound, upperBound);

		for (Size_t i = lowerBound; i < upperBound; ++i)
			m_data[i] = val;
	}

	Iter_t Erase(Iter_t pos)
	{
		Size_t posIndex = pos.m_index;
		MoveLeft(posIndex);

		Size_t newSize = m_size - 1;
		Resize(newSize);

		if (IsEmpty())
			return End();
		else
			return Iter_t(m_data, m_size, posIndex);
	}

	Iter_t Erase(Iter_t first, Iter_t last)
	{
		Size_t count = Distance(first, last);
		if (!count)
			return End();

		Size_t lowerBound = first.m_index;
		Size_t upperBound = lowerBound + count;

		MoveLeftRange(lowerBound, upperBound);

		Size_t newSize = m_size - count;
		Resize(newSize);

		if (IsEmpty())
			return End();
		else
			return Iter_t(m_data, m_size, lowerBound);
	}

	void Swap(KVector& other)
	{
		KVector tmp;

		tmp.Assign(Begin(), End());
		Assign(other.Begin(), other.End());
		other.Assign(tmp.Begin(), tmp.End());
	}

protected:
	inline void Setup()
	{
		m_data = NULL;
		m_size = 0;
		m_capacity = 1;
	}

	inline bool ShouldShrink(Size_t newSize) const
	{
		return newSize < m_size;
	}

	inline bool ShouldGrow(Size_t newSize) const
	{
		return newSize >= m_capacity;
	}

	inline bool ShouldStandstill(Size_t newSize) const
	{
		return (newSize >= m_size) && (newSize < m_capacity);
	}

	inline bool IsInRange(Size_t index) const
	{
		return (index >= 0) && (index < m_size);
	}

	void Grow(Size_t incrementSize, bool increaseCapacity, Val_t val = Val_t())
	{
		Size_t newSize = incrementSize;
		if (increaseCapacity && (incrementSize >= m_size))
		{
			// Doubling the capacity.
			m_capacity = (newSize << 1);
		}
		Ptr_t newData = m_allocator.Allocate(m_capacity * sizeof(T));
		ASSERT(newData);

		if (!newData)
			return;

		if (increaseCapacity)
			Move(newData);
		Append(newData, incrementSize, val);

		m_data = newData;
		m_size = newSize;
	}

	void Grow(Size_t newCapacity)
	{
		m_capacity = newCapacity;
		Ptr_t newData = m_allocator.Allocate(m_capacity * sizeof(T));
		ASSERT(newData);

		if (!newData)
			return;

		Move(newData);
		m_data = newData;
	}

	void Shrink(Size_t newSize, Val_t val = Val_t())
	{
		bool invalidate = true;

		if (newSize == 0)
		{
			m_capacity = 1;
		}
		else if ((newSize < m_size) && (newSize % 2))
		{
			Size_t capacity = m_capacity >> 1;
			if (capacity > newSize)
				m_capacity = capacity;
			else
				invalidate = false;
		}

		// Data array of smaller capacity needed only when the half of previous capacity exceeds new size.
		// Otherwise we've got enough space to retain data in the same array.
		if (invalidate)
		{
			Ptr_t newData = m_allocator.Allocate(m_capacity * sizeof(T));
			ASSERT(newData);

			Trim(newData, newSize);

			m_data = newData;
		}


		m_size = newSize;
	}

	void Replace(Ptr_t dst, Ptr_t src, Size_t pos, Size_t len)
	{
		// Sometimes there is no previously allocated buffer thus we should just return.
		// Otherwise we get access violation by referencing it.
		if (!src)
			return;

		for (Size_t i = pos; i < len; i++)
		{
			dst[i] = src[i];
			m_allocator.Destroy(&src[i]);
		}

		m_allocator.Deallocate(src);
	}

	void Move(Ptr_t dst)
	{
		Replace(dst, m_data, 0, m_size);
	}

	void MoveRight(Size_t index)
	{
		Size_t pos = index;
		// Shifting each subsequent entry right to a position in the array. 
		Size_t newIndex = m_size - 1;
		index = newIndex - 1;
		for (; index >= pos; index--, newIndex--)
			m_data[newIndex] = m_data[index];

		// Delete entry at the position to the right of which data has been moved.
		m_allocator.Destroy(&m_data[pos]);
	}

	void MoveRightRange(Size_t lowerBound, Size_t upperBound)
	{
		// Shifting each subsequent entry to a position in the array, starting just right of given range upper bound.
		Size_t count = (upperBound > lowerBound) ? upperBound - lowerBound : 0;
		if (!count)
			return;

		Size_t pos = lowerBound;
		Size_t newIndex = m_size - 1;
		Size_t index = newIndex - count;
		for (; index >= pos; index--, newIndex--)
			m_data[newIndex] = m_data[index];

		// Delete entries in the initial range.
		for (index = lowerBound; index <= upperBound; index++)
			m_allocator.Destroy(&m_data[index]);
	}

	void MoveLeft(Size_t index)
	{
		Size_t pos = index;

		// Delete entry at the position to the left of which data has been moved.
		m_allocator.Destroy(&m_data[pos]);

		// Shifting each subsequent entry left to a position in the array. 
		Size_t newIndex = index + 1;
		for (; newIndex < m_size; index++, newIndex++)
			m_data[index] = m_data[newIndex];
	}

	void MoveLeftRange(Size_t lowerBound, Size_t upperBound)
	{
		// Delete entries in the specified range.
		for (Size_t index = lowerBound; index <= upperBound; index++)
			m_allocator.Destroy(&m_data[index]);

		// Shifting each subsequent entry to a position in the array, starting just left of given range lower bound.
		Size_t index = lowerBound;
		Size_t newIndex = upperBound;
		for (; newIndex < m_size; index++, newIndex++)
			m_data[index] = m_data[newIndex];
	}

	void Trim(Ptr_t newData, Size_t newSize)
	{
		Replace(newData, m_data, 0, newSize);
	}

	void Populate(Ptr_t data, Size_t pos, Size_t len, CRef_t entry)
	{
		for (Size_t i = pos; i < len; i++)
		{
			m_allocator.Construct(&data[i]);
			data[i] = entry;
		}
	}

	void Create(Ptr_t data, Size_t len, CRef_t entry)
	{
		Populate(data, 0, len, entry);
	}

	void Append(Ptr_t data, Size_t len, CRef_t entry)
	{
		Populate(data, m_size, len, entry);
	}

	Dif_t Distance(CIterRef_t first, CIterRef_t last)
	{
		ASSERT(last.m_index >= first.m_index);
		return static_cast<Dif_t>(last.m_index - first.m_index);
	}

private:
	Alloc m_allocator;
	T* m_data;
	Size_t m_size;
	Size_t m_capacity;
};

template <typename T> struct KPagedPoolVector
{
	typedef KVector< T, typename KPagedPoolAllocator< T >::Type > Type;
};

template <typename T> struct KNonPagedPoolVector
{
	typedef KVector< T, typename KNonPagedPoolAllocator< T >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedPagedPoolVector
{
	typedef KVector< T, typename KTaggedPagedPoolAllocator< T, Tag >::Type > Type;
};

template <typename T, ULONG Tag> struct KTaggedNonPagedPoolVector
{
	typedef KVector< T, typename KTaggedNonPagedPoolAllocator< T, Tag >::Type > Type;
};