#pragma once

#include "CommonDefinitions.h"

typedef int (STDMETHODVCALLTYPE* ONEXIT_CALLBACK_TYPE)(__in_opt PVOID);
typedef void (STDMETHODVCALLTYPE* ATEXIT_CALLBACK_TYPE)(__in_opt PVOID);
typedef void (STDMETHODVCALLTYPE* _ATEXIT_CALLBACK_TYPE)();

void STDMETHODCALLTYPE cpp_cleanup(void);

__checkReturn
__drv_allocatesMem(atexit_entry)
ONEXIT_CALLBACK_TYPE  STDMETHODVCALLTYPE _onexit(__in ONEXIT_CALLBACK_TYPE callback, __in_opt PVOID param);

int STDMETHODVCALLTYPE atexit(__in ATEXIT_CALLBACK_TYPE callback, __in_opt PVOID param);
int STDMETHODVCALLTYPE _atexit(__in _ATEXIT_CALLBACK_TYPE callback);