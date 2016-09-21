#pragma once

#include "WINEXCLUDE.H"
#include <windows.h>

#include <memory>
#include <string>

// stateless functor object for deleting FILE files
struct FILEDeleter 
{
	void operator()(FILE *pFile);
};

using FILE_unique_ptr = std::unique_ptr<FILE, FILEDeleter>;

// stateless functor object for deleting Win File files
struct HANDLEDeleter
{
	void operator()(HANDLE handle);
};

using HANDLE_unique_ptr = std::unique_ptr<void, HANDLEDeleter>;


FILE_unique_ptr make_fopen(const wchar_t* fname, const wchar_t* mode);
HANDLE_unique_ptr make_HANDLE_unique_ptr(HANDLE handle, std::wstring strMsg);

namespace Logger
{
	void PrintCannotOpenFile(std::wstring strFname);
	void PrintErrorTransformingFile(size_t numRead, size_t numWritten);
	void PrintTransformSummary(size_t blockCount, std::wstring strFirstFile, std::wstring strSecondFile);
}
