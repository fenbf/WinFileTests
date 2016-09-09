#include "FileTransformers.h"
#include <cstdio>
#include <memory>

// stateless functor object for deleting FILE files
struct FILEDeleter {
	void operator()(FILE *pFile) {
		fclose(pFile);
	}
};

using FILE_unique_ptr = std::unique_ptr<FILE, FILEDeleter>;

FILE_unique_ptr make_fopen(const wchar_t* fname, const wchar_t* mode)
{
	FILE *f = nullptr;
	_wfopen_s(&f, fname, mode); // by default it's buffered IO, 4k buffer
	return FILE_unique_ptr(f);
}

///////////////////////////////////////////////////////////////////////////////
// StdioFileTransformer

bool StdioFileTransformer::Process(TProcessFunc func)
{
	FILE_unique_ptr pInputFilePtr = make_fopen(m_strFirstFile.c_str(), L"rb");

	// obtain file size:
	fseek(pInputFilePtr.get(), 0, SEEK_END);
	auto lSize = ftell(pInputFilePtr.get());
	rewind(pInputFilePtr.get());

	//// allocate memory to contain the whole file:
	//buffer = (char*)malloc(sizeof(char)*lSize);
	//if (buffer == NULL) { fputs("Memory error", stderr); exit(2); }

	//// copy the file into the buffer:
	//result = fread(buffer, 1, lSize, pFile);
	//if (result != lSize) { fputs("Reading error", stderr); exit(3); }

	return true;
}
