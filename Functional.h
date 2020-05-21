#pragma once

#include "CommonDefinitions.h"
#include "KernelNew.h"
#include "Utility.h"
#include "AutoPtr.h"
#include "TypeTraits.h"
#include "Utility.h"

template <typename Arg, typename Result>
struct UnaryFunction
{
	typedef Arg Argument_t;
	typedef Result Result_t;
};

template <typename Arg1, typename Arg2, typename Result>
struct BinaryFunction
{
	typedef Arg1 FirstArgument_t;
	typedef Arg2 SecondArgument_t;
	typedef Result Result_t;
};

template <typename Tp, bool> struct MemFnConstOrNon
{
	typedef const Tp& type;
};

template <typename Tp> struct MemFnConstOrNon<Tp, false>
{
	typedef Tp& type;
};

template <typename MemberPointer> class MemFn_t;

template <typename Tp> struct IsPlaceholder
{
	static const int value = 0;
};

template <typename Tp> const int IsPlaceholder<Tp>::value;
