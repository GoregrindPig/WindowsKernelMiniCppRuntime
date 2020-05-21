#include "ThreadPool.h"

#pragma warning(disable: 4189)

KThreadPool::KThreadPool(ULONG num, PKSTART_ROUTINE threadFun, PVOID ctx)
: KThreadingBase(threadFun, ctx)
{
	for (ULONG i = 0; i < num; i++)
	{
		PETHREAD threadObject = NULL;
		if (!CreateThread(threadObject))
			continue;

		m_threads.InsertLast(threadObject);
	}
}

KThreadPool::~KThreadPool()
{
	for (ThreadList_t::RevIter_t it = m_threads.RBegin(); it != m_threads.REnd(); --it)
	{
		PVOID obj = *it;
		if (obj)
			ObDereferenceObject(obj);
	}
}

bool KThreadPool::IsValid()
{
	return m_threads.GetSize() > 0;
}

__drv_neverHold(KSPIN_LOCK)
__drv_maxIRQL(APC_LEVEL)
void KThreadPool::Wait(__in const KTimeout& timeout)
{
	KAutoPtr<PVOID> objects;
	KAutoPtr<KWAIT_BLOCK> waitBlock;
	PKWAIT_BLOCK pWaitBlock = NULL;
	ULONG num = GetThreadCount();

	objects.Reset(new (s_tag, NonPagedPool) PVOID [num]);
	PVOID* pObjects = objects.Get();
	ASSERT(pObjects);

	if (m_threads.GetSize() > THREAD_WAIT_OBJECTS)
	{
		waitBlock.Reset(new (s_tag, NonPagedPool) KWAIT_BLOCK[num]);
		pWaitBlock = waitBlock.Get();
		ASSERT(pWaitBlock);
	}

	ThreadList_t::Iter_t it = m_threads.Begin();
	for (ULONG i = 0; it != m_threads.End(); ++it, ++i)
		pObjects[i] = *it;

	NTSTATUS status = KeWaitForMultipleObjects(num, pObjects, WaitAll, 
		Executive, KernelMode, FALSE, timeout.Get(), pWaitBlock);
	ASSERT(status == STATUS_SUCCESS);
}

ULONG KThreadPool::GetThreadCount() const
{
	return static_cast<ULONG>(m_threads.GetSize());
}
