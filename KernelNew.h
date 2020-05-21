#pragma once

#include "CommonDefinitions.h"

// Allocation pool with tag.
__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new(__in size_t size, __in ULONG tag, __in POOL_TYPE pool);

// Allocation pool without tag specified.
__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new(__in size_t size, __in POOL_TYPE pool);

// Placement new.
PVOID STDMETHODVCALLTYPE operator new(__in size_t count, __inout PVOID object);

// Freeing pool.
__drv_freesMem(PVOID)
VOID STDMETHODVCALLTYPE operator delete(__in PVOID object);

// Allocation pool with tag for array.
__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new[](__in size_t size, __in ULONG tag, __in POOL_TYPE pool);

// Allocation pool without tag for array.
__checkReturn
__drv_allocatesMem(PVOID)
PVOID STDMETHODVCALLTYPE operator new[](__in size_t size, __in POOL_TYPE pool);

// Freeing pool for array.
__drv_freesMem(PVOID)
VOID STDMETHODVCALLTYPE operator delete[](__in PVOID object);