// WinFileTest.cpp
// entry code for tests with accessing files on Windows: stdio, iostream, CreateFile, memory mapped
// Bartlomiej Filipek, 2016, bfilipek.com

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>

#include "FileTransformers.h"

bool CopyTransform(uint8_t *inBuf, uint8_t *outBuf, size_t sizeInBytes)
{
	memcpy(outBuf, inBuf, sizeInBytes);
	return true;
}

int _tmain(int argc, LPTSTR argv[])
{
	if (argc < 4)
	{
		printf("inFile outFile blockSize...");
		return 0;
	}

	const int blockSize = _wtoi(argv[3]);
	if (blockSize <= 0)
	{
		printf("wrong block size! %d", blockSize);
		return 0;
	}

	StdioFileTransformer trans(argv[1], argv[2], blockSize);

	trans.Process(CopyTransform);

	return 0;
}