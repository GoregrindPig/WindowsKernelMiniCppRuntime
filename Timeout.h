#pragma once

#include "CommonDefinitions.h"

class KTimeout
{
	mutable PLARGE_INTEGER m_val;
public:
	KTimeout()
		: m_val(NULL)
	{}

	KTimeout(__in PLARGE_INTEGER val)
		: m_val(val)
	{}

	PLARGE_INTEGER Get() const
	{
		return m_val;
	}
};