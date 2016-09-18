#pragma once

#include <string>

class FileCreator
{
public:
	FileCreator(std::wstring strFile, size_t sizeInBytes) 
		: m_strFile(std::move(strFile))
		, m_sizeInBytes(sizeInBytes)
	{ }

	// generates char that will be written to a file
	using TGenFunc = char(*)(); 

	virtual bool Create(TGenFunc func) = 0;

protected:
	const std::wstring m_strFile;
	const size_t m_sizeInBytes;
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

