#include "Thread.h"

KThread::KThread(PKSTART_ROUTINE threadFun, PVOID ctx)
: KThreadingBase(threadFun, ctx)
{
	CreateThread(m_object);
}

KThread::~KThread()
{
	if (m_object)
		ObDereferenceObject(m_object);
}

bool KThread::IsValid()
{
	return m_object != NULL;
}

PETHREAD KThread::Get() const
{
	return m_object;
}

void KThread::Wait(__in const KTimeout& timeout)
{
	KeWaitForSingleObject(m_object, Executive, KernelMode, FALSE, timeout.Get());
}
