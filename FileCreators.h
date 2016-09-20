#pragma once

#include <string>
#include <cassert>

class FileCreator
{
public:
	FileCreator(std::wstring strFile, size_t sizeInBytes, size_t blockSizeInBytes)
		: m_strFile(std::move(strFile))
		, m_sizeInBytes(sizeInBytes)
		, m_blockSizeInBytes(blockSizeInBytes)
	{ 
		assert(m_sizeInBytes % m_blockSizeInBytes == 0 && "size must be multiple of blockSize");
	}

	// buf and block size to generate data
	using TGenFunc = void(*)(char*, size_t);

	virtual bool Create(TGenFunc func) = 0;

protected:
	const std::wstring m_strFile;
	const size_t m_sizeInBytes;
	const size_t m_blockSizeInBytes;
};

// creator using STDIO, 
class StdioFileCreator : public FileCreator
{
public:
	using FileCreator::FileCreator; // inheriting constructor

	virtual bool Create(TGenFunc func) override;
};

// transformer using STD library from C++, streams, 
class IoStreamFileCreator : public FileCreator
{
public:
	using FileCreator::FileCreator; // inheriting constructor

	virtual bool Create(TGenFunc func) override;
};

// transformer using Windows Api, standard
class WinFileCreator : public FileCreator
{
public:
	using FileCreator::FileCreator; // inheriting constructor

	virtual bool Create(TGenFunc func) override;
};

// transformer using Windows Api, memory mapped files
class MappedWinFileCreator : public FileCreator
{
public:
	using FileCreator::FileCreator; // inheriting constructor

	virtual bool Create(TGenFunc func) override;
};

