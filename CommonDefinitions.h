#pragma once

#ifdef _DEBUG
#define	DBGPRINT(_A_)		DbgPrint _A_
#else 
#define	DBGPRINT(_A_)		
#endif // _DEBUG

#pragma warning(disable: 4127)

#if defined (USER_MODE_TEST)

#include <Windows.h>

#pragma warning(disable: 4291)

typedef int POOL_TYPE;

#else

#pragma warning(disable: 4510)
#pragma warning(disable: 4512)
#pragma warning(disable: 4610)

#include <ntifs.h>
#include <fltkernel.h>

#endif // USER_MODE_TEST

#define CLASS_NO_COPY(type)				\
	type(const type&){}					\
	type& operator = (const type&) { return *this; }

#define ITER_TYPEDEF(typeBase)						\
	typedef const typeBase##_t& C##typeBase##Ref_t;	\
	typedef typeBase##_t& typeBase##Ref_t;

#define ITER_INC(iterType, incType, rev)				\
	friend												\
	iterType operator + (iterType it, incType n)		\
	{													\
		return it.OrderedIncrement(n, rev);				\
	}													\
	friend												\
	iterType operator + (incType n, iterType it)		\
	{													\
		return it.OrderedIncrement(n, rev);				\
	}

#define ITER_INC_DEC(iterType, incdecType, rev)			\
	ITER_INC(iterType, incdecType, rev)					\
	friend												\
	iterType operator - (iterType it, incdecType n)		\
	{													\
		return it.OrderedDecrement(n, rev);				\
	}													\
	friend												\
	iterType operator - (incdecType n, iterType it)		\
	{													\
		return it.OrderedDecrement(n, rev);				\
	}

// Miscellaneous functions used by iterators.
// Increment mease advance for the forward iterator.
// Decrement means retreat for the forward iterator.
// Increment mease retreat for the backward iterator.
// Decrement means advance for the backward iterator.

#define DEFINE_INCDEC_FORWARD(type)				\
	type& OrderedIncrement(Dif_t n, bool)		\
	{											\
		return Advance(n);						\
	}											\
	type& OrderedDecrement(Dif_t n, bool)		\
	{											\
		return Retreat(n);						\
	}

#define DEFINE_INCDEC_BOTH(type)				\
	type& OrderedIncrement(Dif_t n, bool rev)	\
	{											\
		if (rev) return Retreat(n);				\
		else return Advance(n);					\
	}											\
	type& OrderedDecrement(Dif_t n, bool rev)	\
	{											\
		if(rev) return Advance(n);				\
		else return Retreat(n);					\
	}
