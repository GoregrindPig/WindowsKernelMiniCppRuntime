#include <ntifs.h>

#include "atexit.h"
#include "KernelNew.h"

struct atexit_entry 
{
	atexit_entry* next;
	ONEXIT_CALLBACK_TYPE function;
	PVOID arg;
};

atexit_entry* __atexit_ptr = NULL;
	
void STDMETHODCALLTYPE cpp_cleanup(void)
{
	atexit_entry* entry = __atexit_ptr;
	__atexit_ptr = NULL; // to prevent infinite loops

	while(entry)
	{
		(entry->function)(entry->arg);
		atexit_entry* next = entry->next;
		delete entry;
		entry = next;
	}
}

__checkReturn
__drv_allocatesMem(atexit_entry)
ONEXIT_CALLBACK_TYPE  STDMETHODVCALLTYPE _onexit(__in ONEXIT_CALLBACK_TYPE callback, __in_opt PVOID param)
{
	if(!callback)
		return NULL;

	atexit_entry* entry = new ('xEnO', NonPagedPool) atexit_entry;
	if(!entry)
		return NULL;

	entry->arg = param;
	entry->next = __atexit_ptr;
	entry->function = callback;
	__atexit_ptr = entry;

	return callback;
}

int STDMETHODVCALLTYPE atexit(__in ATEXIT_CALLBACK_TYPE callback, __in_opt PVOID param)
{
	ONEXIT_CALLBACK_TYPE onExitCallback = reinterpret_cast<ONEXIT_CALLBACK_TYPE>(callback);
	return (_onexit(onExitCallback, param) == onExitCallback) ? 0 : -1;
}

int STDMETHODVCALLTYPE _atexit(__in _ATEXIT_CALLBACK_TYPE callback)
{
	ONEXIT_CALLBACK_TYPE onExitCallback = reinterpret_cast<ONEXIT_CALLBACK_TYPE>(callback);
	return (_onexit(onExitCallback, NULL) == onExitCallback) ? 0 : -1;
}
