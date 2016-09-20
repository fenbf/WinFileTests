#include "FileCreators.h"

#include "Utils.h"

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <memory>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////
// StdioFileCreator

bool StdioFileCreator::Create(TGenFunc func)
{
	FILE_unique_ptr pOutputFilePtr = make_fopen(m_strFile.c_str(), L"wb");
	if (!pOutputFilePtr)
		return false;

	std::unique_ptr<char[]> outBuf(new char[m_blockSizeInBytes]);

	// allocate proper size at the beginning...

	size_t bytesWritten = 0;
	size_t blockCount = 0;
	while (bytesWritten < m_sizeInBytes)
	{
		func(outBuf.get(), m_blockSizeInBytes);
		const auto numWritten = fwrite(outBuf.get(), sizeof(char), m_blockSizeInBytes, pOutputFilePtr.get());
		if (numWritten < m_blockSizeInBytes)
		{
			printf("Problem in writing the file!\n");
			break;
		}

		bytesWritten+=m_blockSizeInBytes;
		blockCount++;
	}

	wprintf(L"Created file %s with %d bytes, %d blocks\n", m_strFile.c_str(), bytesWritten, blockCount);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// IoStreamFileCreator

bool IoStreamFileCreator::Create(TGenFunc )
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// WinFileCreator

bool WinFileCreator::Create(TGenFunc )
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// MappedWinFileCreator

bool MappedWinFileCreator::Create(TGenFunc )
{
	return false;
}
