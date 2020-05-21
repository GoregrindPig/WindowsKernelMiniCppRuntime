#pragma once

#include "CommonDefinitions.h"
#include "Threading.h"

class KThread : public KThreadingBase<KThread>
{
	CLASS_NO_COPY(KThread);
public:
	KThread(PKSTART_ROUTINE threadFun, PVOID ctx);
	~KThread();

	bool IsValid();
	PETHREAD Get() const;

	void Wait(__in const KTimeout& timeout);

private:
	PETHREAD m_object;
};
