#pragma once

#include "CommonDefinitions.h"

class KFile
{
public:
	static const ULONG modeRead = (1 << 0);
	static const ULONG modeWrite = (1 << 1);

	static const ULONG shareRead = (1 << 2);
	static const ULONG shareWrite = (1 << 3);
	static const ULONG shareDelete = (1 << 4);

	static const ULONG modeCreate = (1 << 5);
	static const ULONG modeOpenIf = (1 << 6);
	static const ULONG modeOverwriteIf = (1 << 7);

	static const ULONG optNoBuffer = (1 << 8);
	static const ULONG optRandomAccess = (1 << 9);
	static const ULONG optSequentialScan = (1 << 10);
	static const ULONG optWriteThrough = (1 << 11);

	enum Position
	{
		current,
		begin,
		end
	};

public:
	KFile();
	KFile(const wchar_t* filePath, ULONG flags);

	~KFile();

	const NTSTATUS GetStatus() const;
	const bool IsOpen() const;

	__checkReturn
	__drv_maxIRQL(PASSIVE_LEVEL)
	static NTSTATUS Rename(__in_z const wchar_t* oldName, __in_z const wchar_t* newName);

	__checkReturn
	__drv_maxIRQL(PASSIVE_LEVEL)
	static NTSTATUS Delete(__in_z const wchar_t* filePath);

	__checkReturn
	__drv_maxIRQL(PASSIVE_LEVEL)
	NTSTATUS Create(__in_z const wchar_t* filePath, __in ULONG flags);

	__drv_maxIRQL(PASSIVE_LEVEL)
	void Close();
	
	__drv_maxIRQL(PASSIVE_LEVEL)
	void Flush();

	__drv_maxIRQL(PASSIVE_LEVEL)
	LARGE_INTEGER GetSize() const;

	__drv_maxIRQL(PASSIVE_LEVEL)
	void SetSize(__in const LARGE_INTEGER& size);

	__drv_maxIRQL(PASSIVE_LEVEL)
	LARGE_INTEGER Seek(__in Position startPos, __in const LARGE_INTEGER& offset);

	__drv_maxIRQL(PASSIVE_LEVEL)
	ULONG_PTR Read(__in_bcount(len) PUCHAR buf, __in ULONG len);

	__drv_maxIRQL(PASSIVE_LEVEL)
	ULONG_PTR Write(__in_bcount(len) PUCHAR buf, __in ULONG len);

private:
	mutable NTSTATUS m_status;
	HANDLE m_file;
};