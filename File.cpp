#include "File.h"
#include "KernelNew.h"

static const ULONG tag = 'eliF';

KFile::KFile()
: m_file(NULL)
, m_status(STATUS_SUCCESS)
{
}

KFile::KFile(const wchar_t* filePath, ULONG flags)
: m_file(NULL)
, m_status(STATUS_SUCCESS)
{
	m_status = Create(filePath, flags);
}

KFile::~KFile()
{
	Flush();
	Close();
}

const NTSTATUS KFile::GetStatus() const
{
	return m_status;
}

const bool KFile::IsOpen() const
{
	return (m_file != NULL);
}

#pragma prefast(disable: 6102, "Here we use do-while loop to get rid of goto by the  object deletion.")

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS KFile::Rename(__in_z const wchar_t* oldName, __in_z const wchar_t* newName)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE file = NULL;

	do
	{
		IO_STATUS_BLOCK iosb = { 0 };

		UNICODE_STRING oldNameStr, newNameStr;
		RtlInitUnicodeString(&oldNameStr, oldName);
		RtlInitUnicodeString(&newNameStr, newName);

		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &oldNameStr, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		status = ZwCreateFile(&file, GENERIC_WRITE, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
			FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
			break;

		ULONG len = sizeof(FILE_RENAME_INFORMATION) + newNameStr.MaximumLength;
		PFILE_RENAME_INFORMATION fileRenameInfo = reinterpret_cast<PFILE_RENAME_INFORMATION>(new (tag, PagedPool) UCHAR[len]);
		if (!fileRenameInfo)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		fileRenameInfo->ReplaceIfExists = TRUE;
		fileRenameInfo->RootDirectory = NULL;
		fileRenameInfo->FileNameLength = newNameStr.MaximumLength;
		memcpy(fileRenameInfo->FileName, newNameStr.Buffer, newNameStr.Length);

		status = ZwSetInformationFile(file, &iosb, fileRenameInfo, len, FileDirectoryInformation);

		delete [] fileRenameInfo;

	} while (0);

	if (file)
		ZwClose(file);

	return status;
}

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS KFile::Delete(__in_z const wchar_t* filePath)
{
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE file = NULL;

	do
	{
		IO_STATUS_BLOCK iosb = { 0 };

		UNICODE_STRING filePathStr;
		RtlInitUnicodeString(&filePathStr, filePath);

		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &filePathStr, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		status = ZwCreateFile(&file, GENERIC_WRITE | DELETE, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_DELETE,
			FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		if (!NT_SUCCESS(status))
			break;

		FILE_DISPOSITION_INFORMATION fileDispInfo;
		fileDispInfo.DeleteFile = TRUE;

		status = ZwSetInformationFile(file, &iosb, &fileDispInfo, sizeof(fileDispInfo), FileDirectoryInformation);

	} while (0);

	if (file)
		ZwClose(file);

	return status;
}

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS KFile::Create(__in_z const wchar_t* filePath, __in ULONG flags)
{
	ACCESS_MASK desiredAccess = SYNCHRONIZE;

	if (flags & modeRead)
		desiredAccess |= GENERIC_READ;

	if (flags & modeWrite)
		desiredAccess |= GENERIC_WRITE;

	ULONG shareAccess = 0;

	if (flags & shareRead)
		shareAccess |= FILE_SHARE_READ;

	if (flags & shareWrite)
		shareAccess |= FILE_SHARE_WRITE;

	if (flags & shareDelete)
		shareAccess |= FILE_SHARE_DELETE;

	ULONG disposition = 0;

	if (flags & modeCreate)
		disposition |= FILE_CREATE;

	if (flags & modeOpenIf)
		disposition |= FILE_OPEN_IF;

	if (flags & modeOverwriteIf)
		disposition |= FILE_OVERWRITE_IF;

	ULONG options = FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT;

	if (flags & optSequentialScan)
		options |= FILE_SEQUENTIAL_ONLY;

	if (flags & optRandomAccess)
		options |= FILE_RANDOM_ACCESS;

	if (flags & optWriteThrough)
		options |= FILE_WRITE_THROUGH;

	if (flags & optNoBuffer)
		options |= FILE_NO_INTERMEDIATE_BUFFERING;

	IO_STATUS_BLOCK iosb = {0};

	UNICODE_STRING filePathStr;
	RtlInitUnicodeString(&filePathStr, filePath);

	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, &filePathStr, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	m_status = ZwCreateFile(&m_file, desiredAccess, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
		shareAccess, disposition, options, NULL, 0);

	return m_status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
LARGE_INTEGER KFile::GetSize() const
{
	ASSERT(m_file);

	IO_STATUS_BLOCK iosb = { 0 };
	FILE_END_OF_FILE_INFORMATION fileEofInfo = { 0 };
	m_status = ZwQueryInformationFile(m_file, &iosb, &fileEofInfo, sizeof(fileEofInfo), FileEndOfFileInformation);

	return fileEofInfo.EndOfFile;
}

#pragma prefast(enable: 6102)

__drv_maxIRQL(PASSIVE_LEVEL)
void KFile::Close()
{
	ASSERT(m_file);

	m_status = ZwClose(m_file);
	m_file = NULL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
void KFile::Flush()
{
	ASSERT(m_file);

	IO_STATUS_BLOCK iosb = {0};
	m_status = ZwFlushBuffersFile(m_file, &iosb);
}

__drv_maxIRQL(PASSIVE_LEVEL)
void KFile::SetSize(__in const LARGE_INTEGER& size)
{
	ASSERT(m_file);

	IO_STATUS_BLOCK iosb = { 0 };
	FILE_END_OF_FILE_INFORMATION fileEofInfo = { 0 };
	fileEofInfo.EndOfFile = size;

	m_status = ZwSetInformationFile(m_file, &iosb, &fileEofInfo, sizeof(fileEofInfo), FileEndOfFileInformation);
}

__drv_maxIRQL(PASSIVE_LEVEL)
LARGE_INTEGER KFile::Seek(__in Position startPos, __in const LARGE_INTEGER& offset)
{
	ASSERT(m_file);

	LARGE_INTEGER newPos = { 0 };
	IO_STATUS_BLOCK iosb = { 0 };
	FILE_POSITION_INFORMATION filePosInfo = { 0 };

	m_status = ZwQueryInformationFile(m_file, &iosb, &filePosInfo, sizeof(filePosInfo), FilePositionInformation);
	if (!NT_SUCCESS(m_status))
		return newPos;

	if (startPos == current)
	{
		newPos.QuadPart = filePosInfo.CurrentByteOffset.QuadPart + offset.QuadPart;
	}
	else if (startPos == begin)
	{
		newPos = offset;
	}
	else if (startPos == end)
	{
		if (RtlLargeIntegerGreaterThan(offset, filePosInfo.CurrentByteOffset))
		{
			m_status = STATUS_INVALID_PARAMETER_2;
			return newPos;
		}

		newPos.QuadPart = filePosInfo.CurrentByteOffset.QuadPart - offset.QuadPart;
	}
	else
	{
		m_status = STATUS_INVALID_PARAMETER_1;
		return newPos;
	}

	filePosInfo.CurrentByteOffset = offset;
	m_status = ZwSetInformationFile(m_file, &iosb, &filePosInfo, sizeof(filePosInfo), FilePositionInformation);

	return newPos;
}

__drv_maxIRQL(PASSIVE_LEVEL)
ULONG_PTR KFile::Read(__in_bcount(len) PUCHAR buf, __in ULONG len)
{
	ASSERT(m_file);

	LARGE_INTEGER readOffset;
	readOffset.HighPart = -1;
	readOffset.LowPart = FILE_USE_FILE_POINTER_POSITION;

	IO_STATUS_BLOCK iosb = { 0 };
	m_status = ZwReadFile(m_file, NULL, NULL, NULL, &iosb, buf, len, &readOffset, NULL);
	if (!NT_SUCCESS(m_status))
		return 0;

	return iosb.Information;
}

__drv_maxIRQL(PASSIVE_LEVEL)
ULONG_PTR KFile::Write(__in_bcount(len) PUCHAR buf, __in ULONG len)
{
	ASSERT(m_file);

	LARGE_INTEGER writeOffset;
	writeOffset.HighPart = -1;
	writeOffset.LowPart = FILE_USE_FILE_POINTER_POSITION;

	IO_STATUS_BLOCK iosb = { 0 };
	m_status = ZwWriteFile(m_file, NULL, NULL, NULL, &iosb, buf, len, &writeOffset, NULL);
	if (!NT_SUCCESS(m_status))
		return 0;

	return iosb.Information;
}
