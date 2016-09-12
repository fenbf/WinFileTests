#include "FileTransformers.h"
#include <cstdio>
#include <memory>

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
		wprintf(L"Cannot open %s file!\n", fname);
		return nullptr;
	}

	return FILE_unique_ptr(f);
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
