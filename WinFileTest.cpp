// WinFileTest.cpp
// entry code for tests with accessing files on Windows: stdio, iostream, CreateFile, memory mapped
// Bartlomiej Filipek, 2016, bfilipek.com

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>

#include "FileTransformers.h"

// function used to just copy one file into another
bool CopyTransform(uint8_t *inBuf, uint8_t *outBuf, size_t sizeInBytes)
{
	memcpy(outBuf, inBuf, sizeInBytes);
	return true;
}

enum class AppMode {Invalid, Create, Transform};

struct AppParams
{
	AppMode m_mode{ AppMode::Invalid };
	std::wstring m_strFirstFileName;
	std::wstring m_strSecondFileName;
	size_t m_byteSize{ 0 };
	bool m_benchmark{ false };
};

AppParams ParseCmd(int argc, LPTSTR * argv)
{
	AppParams outParams;

	if (argc < 5)
	{
		printf("WinFileTests options:\n");
		printf("    create filename bytes [benchmark]\n");
		printf("    transform filenameSrc filenameOut blockSize [benchmark]\n");
		return outParams;
	}

	if (wcscmp(argv[1], L"create") == 0)
		outParams.m_mode = AppMode::Create;

	if (wcscmp(argv[1], L"transform") == 0)
		outParams.m_mode = AppMode::Transform;

	if (outParams.m_mode == AppMode::Invalid)
		return outParams;

	outParams.m_strFirstFileName = std::wstring(argv[2]);

	if (outParams.m_mode == AppMode::Transform)
		outParams.m_strFirstFileName = std::wstring(argv[3]);

	outParams.m_byteSize = _wtoi(outParams.m_mode == AppMode::Transform ? argv[4] : argv[3]);
	if (outParams.m_byteSize <= 0)
	{
		printf("wrong byte size! %d", outParams.m_byteSize);
		outParams.m_mode = AppMode::Invalid;
	}

	if ((outParams.m_mode == AppMode::Transform && argc > 5 && wcscmp(argv[5], L"benchmark") == 0) ||
		(outParams.m_mode == AppMode::Create && argc > 4 && wcscmp(argv[4], L"benchmark") == 0))
		outParams.m_benchmark = true;

	return outParams;
}

void TransformFiles(const AppParams& params)
{
	//StdioFileTransformer trans(argv[1], argv[2], blockSize);
	//IoStreamFileTransformer trans(argv[1], argv[2], blockSize);
	//WinFileTransformer trans(argv[1], argv[2], blockSize);
	MappedWinFileTransformer trans(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize);

	trans.Process(CopyTransform);
}

int _tmain(int argc, LPTSTR argv[])
{
	auto params = ParseCmd(argc, argv);

	if (params.m_mode == AppMode::Create)
	{

	}
	else if (params.m_mode == AppMode::Transform)
	{
		TransformFiles(params);
	}

	return 0;
}