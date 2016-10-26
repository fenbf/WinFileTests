#include "Utils.h"

#include <iostream>

namespace Logger
{
	void PrintCannotOpenFile(std::wstring strFname)
	{
		wprintf(L"Cannot open %s file!\n", strFname.c_str());
	}

	void PrintErrorTransformingFile(size_t numRead, size_t numWritten)
	{
		std::wcout << L"Problem in transforming the file: " << numRead << L" read vs " << numWritten << L" written!\n";
	}

	void PrintTransformSummary(size_t blockCount, size_t blockSizeInBytes, std::wstring strFirstFile, std::wstring strSecondFile)
	{
		std::wcout << L"Transformed " << blockCount << L" blocks of " << blockSizeInBytes << L" bytes from " << strFirstFile << L" into " << strSecondFile << L"\n";
	}
}

void FILEDeleter::operator()(FILE *pFile) const
{
	if (pFile)
		fclose(pFile);
}

FILE_unique_ptr make_fopen(const wchar_t* fname, const wchar_t* mode)
{
	FILE *fileHandle = nullptr;
	auto err = _wfopen_s(&fileHandle, fname, mode); // by default it's buffered IO, 4k buffer
	if (err != 0)
	{
		Logger::PrintCannotOpenFile(fname);
		return nullptr;
	}

	return FILE_unique_ptr(fileHandle);
}

FILE_shared_ptr make_fopen_shared(const wchar_t* fname, const wchar_t* mode)
{
	FILE *fileHandle = nullptr;
	auto err = _wfopen_s(&fileHandle, fname, mode); // by default it's buffered IO, 4k buffer
	if (err != 0)
	{
		Logger::PrintCannotOpenFile(fname);
		return nullptr;
	}

	return FILE_shared_ptr(fileHandle, FILEDeleter());
}

HANDLE_unique_ptr make_HANDLE_unique_ptr(HANDLE handle, std::wstring strMsg)
{
	if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
	{
		wprintf_s(L"Invalid handle value! %s\n", strMsg.c_str());
		return nullptr;
	}

	return HANDLE_unique_ptr(handle);
}


void HANDLEDeleter::operator()(HANDLE handle)
{
	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle(handle);
}
