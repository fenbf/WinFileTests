// WinFileTest.cpp
// entry code for tests with accessing files on Windows: stdio, iostream, CreateFile, memory mapped
// Bartlomiej Filipek, 2016, bfilipek.com

#include "WINEXCLUDE.H"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <iostream>

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

enum class AppMode {Invalid, Create, Transform, ClearCache};

struct AppParams
{
	AppMode m_mode{ AppMode::Invalid };
	std::wstring m_strFirstFileName;
	std::wstring m_strSecondFileName;
	std::wstring m_strApiName;
	size_t m_byteSize{ 0 };
	size_t m_secondSize{ 0 };
	bool m_benchmark{ false };
	bool m_sequential{ false };
};

AppParams ParseCmd(int argc, LPTSTR * argv)
{
	AppParams outParams;

	if (argc < 3)
	{
		printf("WinFileTests options:\n");
		printf("    create ApiName filename sizeInMB blockSizeInKilobytes\n");
		printf("    transform ApiName filenameSrc filenameOut blockSizeInKilobytes (seq)\n");
		printf("    clear fileName\n");
		printf("api names: crt, std, win, winmap\n");
		return outParams;
	}

	int currentArg = 1;

	// mode:
	if (wcscmp(argv[currentArg], L"create") == 0)
		outParams.m_mode = AppMode::Create;

	if (wcscmp(argv[currentArg], L"transform") == 0)
		outParams.m_mode = AppMode::Transform;

	if (wcscmp(argv[currentArg], L"clear") == 0)
		outParams.m_mode = AppMode::ClearCache;

	if (outParams.m_mode == AppMode::Invalid)
		return outParams;

	if (outParams.m_mode == AppMode::ClearCache)
	{
		outParams.m_strFirstFileName = std::wstring(argv[++currentArg]);
		return outParams;
	}

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
		std::wcout << L"Wrong byte size! " << outParams.m_byteSize << L"\n";
		outParams.m_mode = AppMode::Invalid;
	}

	if (outParams.m_mode == AppMode::Create)
	{
		outParams.m_byteSize *= 1024 * 1024; // for creation we use Mega Bytes! so convert it into bytes...
	}
	else if (outParams.m_mode == AppMode::Transform)
	{
		outParams.m_byteSize *= 1024; // for transform we use kilobytes! so convert it into bytes...
	}

	// second size for creation
	if (outParams.m_mode == AppMode::Create)
	{
		outParams.m_secondSize = _wtoi(argv[++currentArg]);
		if (outParams.m_secondSize <= 0)
		{
			std::wcout << L"Wrong block size! " << outParams.m_secondSize << L"\n";
			outParams.m_mode = AppMode::Invalid;
		}
		else
			outParams.m_secondSize *= 1024; // convert from kilobytes into bytes...

		if (outParams.m_byteSize % outParams.m_secondSize != 0)
		{
			std::wcout << L"File size must be multiple of block size " << outParams.m_byteSize << L", " << outParams.m_secondSize << L"\n";
			outParams.m_mode = AppMode::Invalid;
		}
	}

	if ((outParams.m_mode == AppMode::Transform && argc > currentArg+1 && wcscmp(argv[++currentArg], L"seq") == 0))
		outParams.m_sequential = true;

	// possible future use...
	//if ((outParams.m_mode != AppMode::Invalid && argc > currentArg+1 && wcscmp(argv[++currentArg], L"benchmark") == 0))
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
		ptrTransformer.reset(new StdioFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize, params.m_sequential));
	else if (params.m_strApiName == L"std")
		ptrTransformer.reset(new IoStreamFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize, params.m_sequential));
	else if (params.m_strApiName == L"win")
		ptrTransformer.reset(new WinFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize, params.m_sequential));
	else if (params.m_strApiName == L"winmap")
		ptrTransformer.reset(new MappedWinFileTransformer(params.m_strFirstFileName, params.m_strSecondFileName, params.m_byteSize, params.m_sequential));
	else
	{
		printf("unrecognized api...\n");
		return;
	}

	if (ptrTransformer)
		ptrTransformer->Process(CopyTransform);
}

void ClearFileCache(const AppParams& params)
{
	// trick that open a file with unbuffered mode and that should clear cache for it...

	HANDLE hFile = CreateFile(params.m_strFirstFileName.c_str(), GENERIC_READ, /*shared mode*/0, /*security*/nullptr, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, /*template*/nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		printf("Cannot open file!");
	else
		CloseHandle(hFile);
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
	else if (params.m_mode == AppMode::ClearCache)
	{
		ClearFileCache(params);
	}

	return 0;
}