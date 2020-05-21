#pragma once

#include "CommonDefinitions.h"
#include "Threading.h"
#include "List.h"

class KThreadPool  : public KThreadingBase<KThreadPool>
{
	CLASS_NO_COPY(KThreadPool);
public:
	KThreadPool(ULONG num, PKSTART_ROUTINE threadFun, PVOID ctx);
	~KThreadPool();

	bool IsValid();

	__drv_neverHold(KSPIN_LOCK)
	__drv_maxIRQL(APC_LEVEL)
	void Wait(__in const KTimeout& timeout);

	ULONG GetThreadCount() const;

private:
	typedef KTaggedNonPagedPoolList<PETHREAD, s_tag>::Type ThreadList_t;

private:
	mutable ThreadList_t m_threads;
};
