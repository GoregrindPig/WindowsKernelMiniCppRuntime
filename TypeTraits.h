#pragma once

#include "CommonDefinitions.h"

struct SfinaeTypes
{
	typedef char One;
	typedef struct
	{
		char arr[2];
	} Two;
};

template <typename Tp, Tp v> struct IntegralConstant
{
   static const Tp value = v;
   typedef Tp value_type;
   typedef IntegralConstant<Tp, v> type;
   operator value_type() { return value; }
 };

typedef IntegralConstant<bool, true> true_type;

typedef IntegralConstant<bool, false> false_type;

template <typename Tp, Tp v>
const Tp IntegralConstant<Tp, v>::value;

// Forward declaration.
template <typename> struct RemoveCV;

template<typename Tp> struct IsFunction;

template <typename> struct IsMemberFunctionPointerHelper : public false_type
{};

template <typename Tp> struct IsMemberFunctionPointer
: public IntegralConstant<bool, (IsMemberFunctionPointerHelper<typename RemoveCV<Tp>::type>::value)>{ };

/// const-volatile modifications [4.7.1].
template <typename Tp> struct RemoveConst
{
	typedef Tp type;
};

template <typename Tp> struct RemoveConst<Tp const>
{
	typedef Tp type;
};

template <typename Tp> struct RemoveVolatile
{
	typedef Tp type;
};

template <typename Tp> struct RemoveVolatile<Tp volatile>
{
	typedef Tp type;
};

template <typename Tp> struct RemoveCV
{
	typedef typename RemoveConst<typename RemoveVolatile<Tp>::type>::type type;
};

template <bool Cond, typename T = void> struct EnableIf
{};

template <typename T> struct EnableIf<true, T>
{
	typedef T type;
};

template <bool Cond, typename IfTrue, typename IfFalse>
struct Conditional
{
	typedef IfTrue type;
};

template <typename IfTrue, typename IfFalse>
struct Conditional<false, IfTrue, IfFalse>
{
	typedef IfFalse type;
};
