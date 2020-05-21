#pragma once

#include "CommonDefinitions.h"
#include "Timeout.h"

#pragma warning(disable:28103 28104 28107 28167)

template <typename ConcreteLock> class KLock
{
	CLASS_NO_COPY(KLock)
public:
	KLock() {}
	~KLock() {}

	void Lock()
	{
		static_cast<ConcreteLock*>(this)->Lock();
	}

	void Unlock()
	{
		static_cast<ConcreteLock*>(this)->Unlock();
	}
};

template <typename T = KLock> class KLocker
{
	CLASS_NO_COPY(KLocker)
public:
	KLocker(T& lock) : m_lock(lock)
	{
		m_lock.Lock();
	}

	~KLocker()
	{
		m_lock.Unlock();
	}

private:
	T& m_lock;
};

template <typename T = KLock, typename C = bool> class KConditionLocker
{
	CLASS_NO_COPY(KConditionLocker)
public:
	KConditionLocker(T& lock, C cond)
		: m_lock(lock)
		, m_cond(cond)
	{
		if (m_cond)
			m_lock.Lock();
	}

	~KConditionLocker()
	{
		if (m_cond)
			m_lock.Unlock();
	}

private:
	T& m_lock;
	C m_cond;
};

template <typename ConcreteRWLock> class KRWLock
{
	CLASS_NO_COPY(KRWLock)
public:
	KRWLock() {}
	~KRWLock() {}

	void LockShared()
	{
		static_cast<ConcreteRWLock*>(this)->LockShared();
	}

	void LockExclusive()
	{
		static_cast<ConcreteRWLock*>(this)->LockExclusive();
	}

	void Unock()
	{
		static_cast<ConcreteRWLock*>(this)->Unlock();
	}
};

template <typename T = KRWLock> class KRWLocker
{
	CLASS_NO_COPY(KRWLocker)
public:
	KRWLocker(T& lock, bool exclusive) : m_lock(lock)
	{
		if (exclusive)
			m_lock.LockExclusive();
		else
			m_lock.LockShared();
	}

	~KRWLocker()
	{
		m_lock.Unlock();
	}

private:
	T& m_lock;
};

class KSpinLock : public KLock<KSpinLock>
{
	CLASS_NO_COPY(KSpinLock)
public:
	KSpinLock() : m_irql(0)
	{
		KeInitializeSpinLock(&m_lock);
	}

	~KSpinLock() {}

	void Lock()
	{
		KeAcquireSpinLock(&m_lock, &m_irql);
	}

	void Unlock()
	{
		KeReleaseSpinLock(&m_lock, m_irql);
	}

	PKSPIN_LOCK Get()
	{
		return &m_lock;
	}

private:
	KSPIN_LOCK m_lock;
	KIRQL m_irql;
};

class KQueuedLock : public KLock<KQueuedLock>
{
	CLASS_NO_COPY(KQueuedLock)
public:
	KQueuedLock()
	{
		RtlZeroMemory(&m_handle, sizeof(KLOCK_QUEUE_HANDLE));
		KeInitializeSpinLock(&m_lock);
	}

	~KQueuedLock() {}

	void Lock()
	{
		KeAcquireInStackQueuedSpinLock(&m_lock, &m_handle);
	}

	void Unlock()
	{
		KeReleaseInStackQueuedSpinLock(&m_handle);
	}

private:
	KSPIN_LOCK m_lock;
	KLOCK_QUEUE_HANDLE m_handle;
};

class KMutex : public KLock<KMutex>
{
	CLASS_NO_COPY(KMutex)
public:
	KMutex()
	{
		KeInitializeMutex(&m_mutex, 0);
	}

	~KMutex() {}

	void Lock()
	{
		KeWaitForMutexObject(&m_mutex, Executive, KernelMode, FALSE, NULL);
	}

	void Unlock()
	{
		KeReleaseMutex(&m_mutex, FALSE);
	}

private:
	KMUTEX m_mutex;
};

class KFastMutex : public KLock<KFastMutex>
{
	CLASS_NO_COPY(KFastMutex)
public:
	KFastMutex()
	{
		ExInitializeFastMutex(&m_mutex);
	}

	~KFastMutex() {}

	void Lock()
	{
		ExAcquireFastMutex(&m_mutex);
	}

	void Unlock()
	{
		ExReleaseFastMutex(&m_mutex);
	}

private:
	FAST_MUTEX m_mutex;
};

class KGuardedMutex : public KLock<KGuardedMutex>
{
	CLASS_NO_COPY(KGuardedMutex)
public:
	KGuardedMutex()
	{
		KeInitializeGuardedMutex(&m_mutex);
	}

	~KGuardedMutex() {}

	void Lock()
	{
		KeAcquireGuardedMutex(&m_mutex);
	}

	void Unlock()
	{
		KeReleaseGuardedMutex(&m_mutex);
	}

private:
	KGUARDED_MUTEX m_mutex;
};

class KResource : public KRWLock<KResource>
{
	CLASS_NO_COPY(KResource)
public:
	KResource()
	{
		ExInitializeResourceLite(&m_res);
	}

	~KResource()
	{
		ExDeleteResourceLite(&m_res);
	}

	__drv_acquiresCriticalRegion
	__drv_acquiresExclusiveResource(ERESOURCE)
	void LockExclusive()
	{
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&m_res, true);
	}

	__drv_acquiresCriticalRegion
	void LockShared()
	{
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&m_res, true);
	}

	__drv_releasesResource(ERESOURCE)
	__drv_releasesCriticalRegion
	void Unlock()
	{
		ExReleaseResourceLite(&m_res);
		KeLeaveCriticalRegion();
	}

	void Reset()
	{
		ExReinitializeResourceLite(&m_res);
	}

private:
	ERESOURCE m_res;
};

class KPushLock : public KRWLock<KPushLock>
{
	CLASS_NO_COPY(KPushLock)
public:
	KPushLock()
	{
		FltInitializePushLock(&m_pushLock);
	}

	~KPushLock()
	{
		FltDeletePushLock(&m_pushLock);
	}

	void LockExclusive()
	{
		FltAcquirePushLockExclusive(&m_pushLock);
	}

	void LockShared()
	{
		FltAcquirePushLockShared(&m_pushLock);
	}

	void Unlock()
	{
		FltReleasePushLock(&m_pushLock);
	}

private:
	EX_PUSH_LOCK m_pushLock;
};

class KSemaphore
{
	CLASS_NO_COPY(KSemaphore)
public:
	KSemaphore(LONG count = 0, LONG limit = MAXLONG)
	{
		Reset(count, limit);
	}

	~KSemaphore() {}

	void Reset(LONG nCount = 0, LONG nLimit = MAXLONG)
	{
		KeInitializeSemaphore(&m_sem, nCount, nLimit);
	}

	LONG GetState()
	{
		return KeReadStateSemaphore(&m_sem);
	}

	void Signal()
	{
		KeReleaseSemaphore(&m_sem, IO_NO_INCREMENT, 1, FALSE);
	}

	NTSTATUS Wait(__in const KTimeout& timeout = KTimeout())
	{
		return KeWaitForSingleObject(&m_sem, Executive, KernelMode, FALSE, timeout.Get());
	}

	PKSEMAPHORE Get() const
	{
		return &m_sem;
	}

private:
	mutable KSEMAPHORE m_sem;
};

class KEvent
{
	CLASS_NO_COPY(KEvent)
public:
	KEvent(bool autoReset = true, bool state = false)
	{
		KeInitializeEvent(&m_event, (autoReset) ? SynchronizationEvent : NotificationEvent, state);
	}

	~KEvent() {}

	void Clear()
	{
		KeClearEvent(&m_event);
	}

	bool GetState()
	{
		return KeReadStateEvent(&m_event) > 0;
	}

	bool Reset()
	{
		return KeResetEvent(&m_event) > 0;
	}

	void Signal()
	{
		KeSetEvent(&m_event, IO_NO_INCREMENT, FALSE);
	}

	NTSTATUS Wait(__in const KTimeout& timeout = KTimeout())
	{
		return KeWaitForSingleObject(&m_event, Executive, KernelMode, FALSE, timeout.Get());
	}

	PKEVENT Get() const
	{
		return &m_event;
	}

private:
	mutable KEVENT m_event;
};