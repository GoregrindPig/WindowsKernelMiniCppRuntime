#include "KernelNew.h"

__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new(__in size_t size, __in ULONG tag, __in POOL_TYPE pool)
{
	bool correctPoolSpec = ((pool == NonPagedPool) || (pool == PagedPool));

	ASSERT(correctPoolSpec);
	if(!correctPoolSpec)
		return NULL;
	
	PVOID buffer = ExAllocatePoolWithTag(pool, size, tag);
	if(buffer)
		RtlSecureZeroMemory(buffer, size);

	return buffer;
}

__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new(__in size_t size, __in POOL_TYPE pool)
{
	bool correctPoolSpec = ((pool == NonPagedPool) || (pool == PagedPool));

	ASSERT(correctPoolSpec);
	if(!correctPoolSpec)
		return NULL;
	
	PVOID buffer = ExAllocatePoolWithTag(pool, size, 'meMX');
	if(buffer)
		RtlSecureZeroMemory(buffer, size);

	return buffer;
}

PVOID STDMETHODVCALLTYPE operator new(__in size_t count, __inout PVOID object)
{
	UNREFERENCED_PARAMETER(count);

	ASSERT(object);
	return object;
}

__drv_freesMem(PVOID)
VOID STDMETHODVCALLTYPE operator delete(__in PVOID object)
{
	if (object && MmIsAddressValid(object))
		ExFreePool(object);
}

__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new[](__in size_t size, __in ULONG tag, __in POOL_TYPE pool)
{
	return operator new(size, tag, pool);
}

__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new[](__in size_t size, __in POOL_TYPE pool)
{
	return operator new(size, pool);
}

__drv_freesMem(PVOID)
VOID STDMETHODVCALLTYPE operator delete[](__in PVOID object)
{
	operator delete(object);
}
