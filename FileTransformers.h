#pragma once

#include <string>

// future extension and improvement...
class ITransformMethod
{
public:
	virtual ~ITransformMethod() { }

	// estimates about the final output file size
	virtual size_t ComputeFinalFileSize(size_t inputFileSize) = 0;

	// size of the output buffer, it can be the same as inputBufferSize (we process files block by block)
	virtual size_t GetDefaultOutbutBufferSize(size_t inputBufferSize) = 0;

	// inBuf, outBuf, sizeInBytes: transforms inBuf and writes into outBuf, 
	// poutSize informs about written bytes into the output buffer (might be less than GetDefaultOutbutBufferSize)
	virtual void Process(uint8_t* inBuf, uint8_t* outBuf, size_t inSize, size_t* pOutSize) = 0;
};

// base class for our tests, defines basic interface and common methods
// takes two file names, transforms the first file and writes output to the second file
// transform using external function, operates on blocks of bytes
class IFileTransformer
{
public:
	IFileTransformer(std::wstring strFirstFile, std::wstring strSecondFile, size_t blockSizeInBytes, bool useSequential) 
		: m_strFirstFile(std::move(strFirstFile))
		, m_strSecondFile(std::move(strSecondFile))
		, m_blockSizeInBytes(blockSizeInBytes)
		, m_useSequential(useSequential)
	{ }
	virtual ~IFileTransformer() { }

	using TProcessFunc = void(*) (uint8_t*, uint8_t*, size_t); 

	virtual bool Process(TProcessFunc func) = 0;

protected:
	const std::wstring m_strFirstFile;
	const std::wstring m_strSecondFile;
	const size_t m_blockSizeInBytes;
	const bool m_useSequential;
};

// transformer using STDIO, 
class StdioFileTransformer : public IFileTransformer
{
public:
	using IFileTransformer::IFileTransformer; // inheriting constructor

	virtual bool Process(TProcessFunc func) override;
};

// transformer using STD library from C++, streams, 
class IoStreamFileTransformer : public IFileTransformer
{
public:
	using IFileTransformer::IFileTransformer; // inheriting constructor

	virtual bool Process(TProcessFunc func) override;
};

// transformer using Windows Api, standard
class WinFileTransformer : public IFileTransformer
{
public:
	using IFileTransformer::IFileTransformer; // inheriting constructor

	virtual bool Process(TProcessFunc func) override;
};

// transformer using Windows Api, memory mapped files
class MappedWinFileTransformer : public IFileTransformer
{
public:
	using IFileTransformer::IFileTransformer; // inheriting constructor

	virtual bool Process(TProcessFunc func) override;
};

