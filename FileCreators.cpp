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

	// optional buffer for the further tests?
	// std::unique_ptr<char[]> outBuf(new uint8_t[m_blockSizeInBytes]);

	// allocate proper size at the beginning...

	size_t bytesWritten = 0;
	while (bytesWritten < m_sizeInBytes)
	{
		char val = func();
		const auto numWritten = fwrite(&val, sizeof(char), 1, pOutputFilePtr.get());
		if (numWritten < 1)
		{
			printf("Problem in writing the file!\n");
			break;
		}

		bytesWritten++;
	}

	wprintf(L"Created file %s with %d bytes\n", m_strFile.c_str(), bytesWritten);

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
