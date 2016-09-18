#include "FileTransformers.h"

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <memory>
#include <fstream>

void PrintCannotOpenFile(std::wstring strFname)
{
	wprintf(L"Cannot open %s file!\n", strFname.c_str());
}

// stateless functor object for deleting FILE files
struct FILEDeleter 
{
	void operator()(FILE *pFile) 
	{
		if (pFile)
			fclose(pFile);
	}
};

using FILE_unique_ptr = std::unique_ptr<FILE, FILEDeleter>;

FILE_unique_ptr make_fopen(const wchar_t* fname, const wchar_t* mode)
{
	FILE *f = nullptr;
	auto err = _wfopen_s(&f, fname, mode); // by default it's buffered IO, 4k buffer
	if (err != 0)
	{
		PrintCannotOpenFile(fname);
		return nullptr;
	}

	return FILE_unique_ptr(f);
}

// stateless functor object for deleting Win File files
struct HANDLEDeleter
{
	void operator()(HANDLE handle)
	{
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
	}
};

using HANDLE_unique_ptr = std::unique_ptr<void, HANDLEDeleter>;

HANDLE_unique_ptr make_HANDLE_unique_ptr(HANDLE handle, std::wstring strMsg)
{
	if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
	{
		wprintf_s(L"Invalid handle value! %s\n", strMsg.c_str());
		return nullptr;
	}

	return HANDLE_unique_ptr(handle);
}

///////////////////////////////////////////////////////////////////////////////
// StdioFileTransformer

bool StdioFileTransformer::Process(TProcessFunc func)
{
	FILE_unique_ptr pInputFilePtr = make_fopen(m_strFirstFile.c_str(), L"rb");
	if (!pInputFilePtr)
		return false;

	FILE_unique_ptr pOutputFilePtr = make_fopen(m_strSecondFile.c_str(), L"wb");
	if (!pOutputFilePtr)
		return false;

	std::unique_ptr<uint8_t[]> inBuf(new uint8_t[m_blockSizeInBytes]);
	std::unique_ptr<uint8_t[]> outBuf(new uint8_t[m_blockSizeInBytes]);

	size_t blockCount = 0;
	while (!feof(pInputFilePtr.get()))
	{
		const auto numRead = fread_s(inBuf.get(), m_blockSizeInBytes, /*element size*/sizeof(uint8_t), m_blockSizeInBytes, pInputFilePtr.get());
		
		if (numRead == 0)
		{
			printf("Couldn't read block of data!\n");
			return false;
		}

		if (!func(inBuf.get(), outBuf.get(), numRead))
			break;

		const auto numWritten = fwrite(outBuf.get(), sizeof(uint8_t), numRead, pOutputFilePtr.get());
		if (numRead != numWritten)
			printf("Problem in transforming the file: %d read vs %d (bytes) written\n", numRead, numWritten);

		blockCount++;
	}

	wprintf(L"Transformed %d blocks from %s into %s\n", blockCount, m_strFirstFile.c_str(), m_strSecondFile.c_str());

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// IoStreamFileTransformer

bool IoStreamFileTransformer::Process(TProcessFunc func)
{
	std::ifstream inputStream(m_strFirstFile, std::wios::in | std::wios::binary);
	if (inputStream.bad())
	{
		PrintCannotOpenFile(m_strFirstFile);
		return false;
	}

	std::ofstream outputStream(m_strSecondFile, std::wios::out | std::wios::binary | std::wios::trunc);
	if (outputStream.bad())
	{
		PrintCannotOpenFile(m_strSecondFile);
		return false;
	}

	std::unique_ptr<uint8_t[]> inBuf(new uint8_t[m_blockSizeInBytes]);
	std::unique_ptr<uint8_t[]> outBuf(new uint8_t[m_blockSizeInBytes]);

	size_t blockCount = 0;
	while (!inputStream.eof())
	{
		inputStream.read((char *)(inBuf.get()), m_blockSizeInBytes);

		if (inputStream.bad())
		{
			printf("Couldn't read block of data!\n");
			return false;
		}

		const auto numRead = inputStream.gcount();

		if (!func(inBuf.get(), outBuf.get(), static_cast<size_t>(numRead)))
			break;

		outputStream.write((const char *)outBuf.get(), numRead);
		if (outputStream.bad())
			printf("Cannot write %d bytes into output\n", static_cast<size_t>(numRead));

		blockCount++;
	}

	wprintf(L"Transformed %d blocks from %s into %s\n", blockCount, m_strFirstFile.c_str(), m_strSecondFile.c_str());

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// WinFileTransformer

bool WinFileTransformer::Process(TProcessFunc func)
{
	auto hInputFile = make_HANDLE_unique_ptr(CreateFile(m_strFirstFile.c_str(), GENERIC_READ, /*shared mode*/0, /*security*/nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strFirstFile);
	if (!hInputFile)
		return false;

	auto hOutputFile = make_HANDLE_unique_ptr(CreateFile(m_strSecondFile.c_str(), GENERIC_WRITE, /*shared mode*/0, /*security*/nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strSecondFile);
	if (!hOutputFile)
		return false;

	std::unique_ptr<uint8_t[]> inBuf(new uint8_t[m_blockSizeInBytes]);
	std::unique_ptr<uint8_t[]> outBuf(new uint8_t[m_blockSizeInBytes]);

	DWORD numBytesRead = 0;
	DWORD numBytesWritten = 0;
	size_t blockCount = 0;
	BOOL writeOK = TRUE;
	while (ReadFile(hInputFile.get(), inBuf.get(), m_blockSizeInBytes, &numBytesRead, /*overlapped*/nullptr) && numBytesRead > 0 && writeOK)
	{
		if (!func(inBuf.get(), outBuf.get(), numBytesRead))
			break;

		writeOK = WriteFile(hOutputFile.get(), outBuf.get(), numBytesRead, &numBytesWritten, /*overlapped*/nullptr);
		
		if (numBytesRead != numBytesWritten)
			printf("Problem in transforming the file: %d read vs %d (bytes) written\n", numBytesRead, numBytesWritten);

		blockCount++;
	}

	wprintf(L"Transformed %d blocks from %s into %s\n", blockCount, m_strFirstFile.c_str(), m_strSecondFile.c_str());

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// MappedWinFileTransformer


// with memory mapped files it's required to use SEH, so we need a separate function to do this
// see at: https://blogs.msdn.microsoft.com/larryosterman/2006/10/16/so-when-is-it-ok-to-use-seh/
bool DoProcess(uint8_t* &pIn, uint8_t* ptrInFile, uint8_t* &pOut, uint8_t* ptrOutFile, LARGE_INTEGER &fileSize, const size_t m_blockSizeInBytes, FileTransformer::TProcessFunc func)
{
	size_t bytesProcessed = 0;
	size_t blockSize = 0;

	__try
	{
		pIn = ptrInFile;
		pOut = ptrOutFile;
		while (pIn < ptrInFile + fileSize.QuadPart)
		{
			blockSize = static_cast<size_t>(bytesProcessed + m_blockSizeInBytes < fileSize.QuadPart ? m_blockSizeInBytes : fileSize.QuadPart - bytesProcessed);
			func(pIn, pOut, blockSize);
			pIn += blockSize;
			pOut += blockSize;
			bytesProcessed += m_blockSizeInBytes;
		}
		return true;
	}
	__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		printf("Fatal Error accessing mapped file.\n");
	}

	return false;
}

bool MappedWinFileTransformer::Process(TProcessFunc func)
{
	auto hInputFile = make_HANDLE_unique_ptr(CreateFile(m_strFirstFile.c_str(), GENERIC_READ, /*shared mode*/0, /*security*/nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strFirstFile);
	if (!hInputFile)
		return false;

	/* The output file MUST have Read/Write access for the mapping to succeed. */
	auto hOutputFile = make_HANDLE_unique_ptr(CreateFile(m_strSecondFile.c_str(), GENERIC_READ | GENERIC_WRITE, /*shared mode*/0, /*security*/nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strSecondFile);
	if (!hOutputFile)
		return false;
	
	bool complete = false;
	uint8_t* ptrInFile = nullptr; 
	uint8_t* ptrOutFile = nullptr;
	uint8_t* pIn = nullptr;
	uint8_t* pOut = nullptr;

	HANDLE_unique_ptr hInputMap;
	HANDLE_unique_ptr hOutputMap;

	LARGE_INTEGER fileSize;

	/* Get the input file size. */
	GetFileSizeEx(hInputFile.get(), &fileSize);
			
	/* This is a necessar, but NOT sufficient, test for mappability on 32-bit systems S
	* Also see the long comment a few lines below */
	/*if (fileSize.HighPart > 0 && sizeof(SIZE_T) == 4)
		ReportException(_T("This file is too large to map on a Win32 system."), 4);*/

	/* Create a file mapping object on the input file. Use the file size. */
	hInputMap = make_HANDLE_unique_ptr(CreateFileMapping(hInputFile.get(), NULL, PAGE_READONLY, 0, 0, NULL), L"Input map");
	if (!hInputMap)
		return false;

	/* Map the input file */
	ptrInFile = (uint8_t*)MapViewOfFile(hInputMap.get(), FILE_MAP_READ, 0, 0, 0);
	if (ptrInFile == nullptr)
	{
		printf("Cannot map input file!\n");
		return false;
	}

	/*  Create/Open the output file. */

	hOutputMap = make_HANDLE_unique_ptr(CreateFileMapping(hOutputFile.get(), NULL, PAGE_READWRITE, fileSize.HighPart, fileSize.LowPart, NULL), L"Output map");
	if (!hOutputMap)
		return false;

	ptrOutFile = (uint8_t*)MapViewOfFile(hOutputMap.get(), FILE_MAP_WRITE, 0, 0, (SIZE_T)fileSize.QuadPart);
	if (ptrOutFile == nullptr)
	{
		printf("Cannot map output file!\n");
		return false;
	}

	DoProcess(pIn, ptrInFile, pOut, ptrOutFile, fileSize, m_blockSizeInBytes, func);

	/* Close all views and handles. */
	UnmapViewOfFile(ptrOutFile); 
	UnmapViewOfFile(ptrInFile);

	return complete;
}
