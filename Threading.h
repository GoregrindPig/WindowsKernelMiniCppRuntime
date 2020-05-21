#pragma once

#include "CommonDefinitions.h"
#include "Timeout.h"
#include "Functional.h"
#include "AutoPtr.h"

template <typename Derived, ULONG Tag = 'drTX'> class KThreadingBase
{
public:
	KThreadingBase()
		: m_threadCallback(NULL)
		, m_ctx(NULL)
	{
	}

	KThreadingBase(PKSTART_ROUTINE threadCallback, PVOID ctx)
		: m_threadCallback(threadCallback)
		, m_ctx(ctx)
	{
	}

	bool IsValid()
	{
		return static_cast<Derived*>(this)->IsValid();
	}

	__drv_neverHold(KSPIN_LOCK)
	__drv_maxIRQL(APC_LEVEL)
	void Wait(__in const KTimeout& timeout)
	{
		return static_cast<Derived*>(this)->Wait(timeout);
	}

protected:
	bool CreateThread(PETHREAD& threadObject)
	{
		bool res = false;

		do
		{
			HANDLE threadHandle = NULL;
			OBJECT_ATTRIBUTES oa = { 0 };
			InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
			NTSTATUS status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, &oa, NULL, NULL,
				m_threadCallback, m_ctx);
			if (!NT_SUCCESS(status))
				break;

			status = ObReferenceObjectByHandle(threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode,
				reinterpret_cast<PVOID*>(&threadObject), NULL);
			if (threadHandle)
				ZwClose(threadHandle);

			res = NT_SUCCESS(status);
		} while (0);

		return res;
	}

protected:
	static const ULONG s_tag = Tag;
	PKSTART_ROUTINE m_threadCallback;
	PVOID m_ctx;
};