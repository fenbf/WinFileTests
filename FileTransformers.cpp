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

bool MappedWinFileTransformer::Process(TProcessFunc func)
{
	auto hInputFile = make_HANDLE_unique_ptr(CreateFile(m_strFirstFile.c_str(), GENERIC_READ, /*shared mode*/0, /*security*/nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strFirstFile);
	if (!hInputFile)
		return false;

	auto hOutputFile = make_HANDLE_unique_ptr(CreateFile(m_strSecondFile.c_str(), GENERIC_WRITE, /*shared mode*/0, /*security*/nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, /*template*/nullptr), m_strSecondFile);
	if (!hOutputFile)
		return false;
	
	BOOL complete = FALSE;
	HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE;
	HANDLE hInMap = NULL, hOutMap = NULL;
	LPTSTR pIn = NULL, pInFile = NULL, pOut = NULL, pOutFile = NULL;

	HANDLE_unique_ptr hInputMap;
	HANDLE_unique_ptr hOutputMap;

	__try {
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
		pInFile = MapViewOfFile(hInputMap.get(), FILE_MAP_READ, 0, 0, 0);
		if (pInFile == NULL)
			ReportException(_T("Failure Mapping input file."), 3);

		/*  Create/Open the output file. */
		/* The output file MUST have Read/Write access for the mapping to succeed. */
		hOut = CreateFile(fOut, GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOut == INVALID_HANDLE_VALUE) {
			complete = TRUE; /* Do not delete an existing file. */
			ReportException(_T("Failure Opening output file."), 5);
		}

		/* Map the output file. CreateFileMapping will expand
		the file if it is smaller than the mapping. */

		hOutMap = CreateFileMapping(hOut, NULL, PAGE_READWRITE, fileSize.HighPart, fileSize.LowPart, NULL);
		if (hOutMap == NULL)
			ReportException(_T("Failure creating output map."), 7);
		pOutFile = MapViewOfFile(hOutMap, FILE_MAP_WRITE, 0, 0, (SIZE_T)fileSize.QuadPart);
		if (pOutFile == NULL)
			ReportException(_T("Failure mapping output file."), 8);

		/* Now move the input file to the output file, doing all the work in memory. */
		__try
		{
			CHAR cShift = (CHAR)shift;
			pIn = pInFile;
			pOut = pOutFile;

			while (pIn < pInFile + fileSize.QuadPart) {
				*pOut = (*pIn + cShift);
				pIn++; pOut++;
			}
			complete = TRUE;
		}
		__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			complete = FALSE;
			ReportException(_T("Fatal Error accessing mapped file."), 9);
		}

		/* Close all views and handles. */
		UnmapViewOfFile(pOutFile); UnmapViewOfFile(pInFile);
		CloseHandle(hOutMap); CloseHandle(hInMap);
		CloseHandle(hIn); CloseHandle(hOut);
		return complete;
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (pOutFile != NULL) UnmapViewOfFile(pOutFile); if (pInFile != NULL) UnmapViewOfFile(pInFile);
		if (hOutMap != NULL) CloseHandle(hOutMap); if (hInMap != NULL) CloseHandle(hInMap);
		if (hIn != INVALID_HANDLE_VALUE) CloseHandle(hIn); if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);

		/* Delete the output file if the operation did not complete successfully. */
		if (!complete)
			DeleteFile(fOut);
		return FALSE;
	}

	return true; 
}
