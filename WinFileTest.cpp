// WinFileTest.cpp
// entry code for tests with accessing files on Windows: stdio, iostream, CreateFile, memory mapped
// Bartlomiej Filipek, 2016, bfilipek.com

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>
#include <memory>

#include "FileTransformers.h"
#include "FileCreators.h"

// function used to just copy one file into another
void CopyTransform(uint8_t *inBuf, uint8_t *outBuf, size_t sizeInBytes)
{
	// we care mostly about the performance of IO, so this code is in fact not that important, could be extended if needed later
	memcpy(outBuf, inBuf, sizeInBytes);
}

void GenOrder(char* buf, size_t blockSize)
{
	for (size_t i = 0; i < blockSize; ++i)
	{
		buf[i] = static_cast<char>(32 + i % 96);
	}
}

enum class AppMode {Invalid, Create, Transform};

struct AppParams
{
	AppMode m_mode{ AppMode::Invalid };
	std::wstring m_strFirstFileName;
	std::wstring m_strSecondFileName;
	std::wstring m_strApiName;
	size_t m_byteSize{ 0 };
	size_t m_secondSize{ 0 };
	bool m_benchmark{ false };
};

AppParams ParseCmd(int argc, LPTSTR * argv)
{
	AppParams outParams;

	if (argc < 6)
	{
		printf("WinFileTests options:\n");
		printf("    create ApiName filename sizeInMB blockSizeInBytes\n");
		printf("    transform ApiName filenameSrc filenameOut blockSizeInBytes\n");
		printf("api names: crt, std, win, winmap\n");
		return outParams;
	}

	int currentArg = 1;

	// mode:
	if (wcscmp(argv[currentArg], L"create") == 0)
		outParams.m_mode = AppMode::Create;

	if (wcscmp(argv[currentArg], L"transform") == 0)
		outParams.m_mode = AppMode::Transform;

	if (outParams.m_mode == AppMode::Invalid)
		return outParams;

	// apiName:
	outParams.m_strApiName = std::wstring(argv[++currentArg]);

	// paths:
	outParams.m_strFirstFileName = std::wstring(argv[++currentArg]);

	if (outParams.m_mode == AppMode::Transform)
		outParams.m_strSecondFileName = std::wstring(argv[++currentArg]);

	// byte size
	outParams.m_byteSize = _wtoi(argv[++currentArg]);
	if (outParams.m_byteSize <= 0)
	{
		printf("wrong byte size! %d", outParams.m_byteSize);
		outParams.m_mode = AppMode::Invalid;
	}
	if (outParams.m_mode == AppMode::Create)
	{
		outParams.m_byteSize *= 1024 * 1024; // for creation we use Mega Bytes! so convert it into bytes...
	}

	// second size for creation
	if (outParams.m_mode == AppMode::Create)
	{
		outParams.m_secondSize = _wtoi(argv[++currentArg]);
		if (outParams.m_secondSize <= 0)
		{
			printf("wrong block size! %d", outParams.m_byteSize);
			outParams.m_mode = AppMode::Invalid;
		}
		if (outParams.m_byteSize % outParams.m_secondSize != 0)
		{
			printf("file size must be multiple of block size! %d %d", outParams.m_byteSize, outParams.m_secondSize);
			outParams.m_mode = AppMode::Invalid;
		}
	}

	// possible future use...
	//if ((outParams.m_mode != AppMode::Invalid && argc > 5 && wcscmp(argv[++currentArg], L"benchmark") == 0))
	//	outParams.m_benchmark = true;

	return outParams;
}

void CreateFile(const AppParams& params)
{
	if (params.m_strApiName != L"crt")
		printf("apiname ignored for now, only crt used...\n");

	StdioFileCreator gen(params.m_strFirstFileName, params.m_byteSize, params.m_secondSize);

	gen.Create(GenOrder);

}

void TransformFiles(const AppParams& params)
{
	std::unique_ptr<IFileTransformer> ptrTransformer;
	if (params.m_strApiName == L"crt")
		ptrTransformer.reset(new StdioFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize));
	else if (params.m_strApiName == L"std")
		ptrTransformer.reset(new IoStreamFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize));
	else if (params.m_strApiName == L"win")
		ptrTransformer.reset(new WinFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize));
	else if (params.m_strApiName == L"winmap")
		ptrTransformer.reset(new MappedWinFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize));
	else
	{
		printf("unrecognized api...\n");
		return;
	}

	if (ptrTransformer)
		ptrTransformer->Process(CopyTransform);
}

int _tmain(int argc, LPTSTR argv[])
{
	auto params = ParseCmd(argc, argv);

	if (params.m_mode == AppMode::Create)
	{
		CreateFile(params);
	}
	else if (params.m_mode == AppMode::Transform)
	{
		TransformFiles(params);
	}

	return 0;
}