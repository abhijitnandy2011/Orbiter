/*
 * Impl
 */

#include "ShaderDiskCache.h"
#include <tchar.h>  // for _T and TCHAR

#if _DEBUG
#define CACHE_FILE_SUFFIX _T(".debug.cache")
#else
#define CACHE_FILE_SUFFIX _T(".cache")
#endif

using namespace Rendering;

ShaderDiskCache::ShaderDiskCache(std::string& cacheRoot)
{
    m_cacheDirectoryRoot = cacheRoot;
	if (m_cacheDirectoryRoot[m_cacheDirectoryRoot.length() - 1] != '\\')
	{
		//append backslash
		m_cacheDirectoryRoot += "\\";
	}
}


ShaderDiskCache::~ShaderDiskCache(void)
{
}

void
ShaderDiskCache::createCacheFileName(
    const std::string& shaderFileName,
    std::string& result)
{
	auto index = (int)shaderFileName.find('\\');
	std::string fileName;
	if (index > -1)
	{
		auto idx = index;
		while(idx > -1)
		{
			index = idx;
			idx = (int)shaderFileName.find('\\', idx + 1);
		}
		fileName = shaderFileName.substr(index + 1);
	}
	else
	{
		fileName = shaderFileName;
	}
	result = m_cacheDirectoryRoot + fileName + CACHE_FILE_SUFFIX;
}

void
ShaderDiskCache::writeCacheEntry(
    HANDLE hFile,
    const CacheEntry & entry)
{
	DWORD dwWritten;
	::WriteFile(hFile, &entry.keyLength, sizeof(entry.keyLength) + sizeof(entry.bytecodeLength), &dwWritten, nullptr);
	//::WriteFile(hFile, &entry.bytecodeLength, sizeof(entry.bytecodeLength), &dwWritten, nullptr);
	::WriteFile(hFile, entry.key, sizeof(char) * entry.keyLength, &dwWritten, nullptr);
	::WriteFile(hFile, entry.bytecode, entry.bytecodeLength, &dwWritten, nullptr);

}

UINT
ShaderDiskCache::calculateCacheEntrySize(const CacheEntry & entry)
{
	return sizeof(entry.keyLength) + sizeof(char) * entry.keyLength + 
			sizeof(entry.bytecodeLength) + entry.bytecodeLength;
}

bool
ShaderDiskCache::readCacheEntry(
    HANDLE hFile,
    CacheEntry & entry,
    bool readBytecode)
{
	DWORD dwRead;
	DWORD size = sizeof(entry.keyLength) + sizeof(entry.bytecodeLength);
	if (!ReadFile(hFile, &entry.keyLength, size, &dwRead, nullptr) || dwRead != size)
	{
		return false;
	}
	auto key = new char[entry.keyLength + 1];
	key[entry.keyLength] = '\0';
	ReadFile(hFile, key, entry.keyLength, &dwRead, nullptr);
	entry.key = key;
		
	//ReadFile(hFile, &entry.bytecodeLength, sizeof(entry.bytecodeLength), &dwRead, nullptr);
	if (readBytecode)
	{
		auto bytecode = new BYTE[entry.bytecodeLength];
		ReadFile(hFile, bytecode, entry.bytecodeLength, &dwRead, nullptr);
		entry.bytecode = bytecode;
	}
	else
	{
		//just advance file pointer
		SetFilePointer(hFile, entry.bytecodeLength, nullptr, FILE_CURRENT);
	}
	return true;
}

bool
ShaderDiskCache::searchForEntry(
    CacheEntry& entry,
    const TCHAR * fileName,
    const TCHAR * shaderFileName)
{
	auto attrs = ::GetFileAttributes(fileName);
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		//no file exists, so nowhere to look at
		return false;
	}

	WIN32_FILE_ATTRIBUTE_DATA shaderFileInfo;
	WIN32_FILE_ATTRIBUTE_DATA cacheFileInfo;
	::GetFileAttributesEx(shaderFileName, GetFileExInfoStandard, &shaderFileInfo);
	::GetFileAttributesEx(fileName, GetFileExInfoStandard, &cacheFileInfo);

	ULARGE_INTEGER shaderLastModified, cacheLastModified;
	shaderLastModified.LowPart = shaderFileInfo.ftLastWriteTime.dwLowDateTime;
	shaderLastModified.HighPart = shaderFileInfo.ftLastWriteTime.dwHighDateTime;
	cacheLastModified.LowPart = cacheFileInfo.ftLastWriteTime.dwLowDateTime;
	cacheLastModified.HighPart = cacheFileInfo.ftLastWriteTime.dwHighDateTime;
	if (shaderLastModified.QuadPart > cacheLastModified.QuadPart)
	{
		//cache file found, not outdated, so pretend in doesn't exist
		return false;
	}

	HANDLE hFile = ::CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
				nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//scan file for entry
	CacheEntry curr;
	while (readCacheEntry(hFile, curr, false))
	{
		if (!strcmp(curr.key, entry.key))
		{
			//entry found, stop
			auto size = calculateCacheEntrySize(curr);
			int iSize = size;
			::SetFilePointer(hFile, -iSize, nullptr, FILE_CURRENT);
			readCacheEntry(hFile, entry, true);
			::CloseHandle(hFile);
			return true;
		}
	}
	::CloseHandle(hFile);
	return false;
}

void
ShaderDiskCache::addCacheEntry(
    const CacheEntry & entry,
    const TCHAR * fileName,
    const TCHAR * shaderFileName)
{
	auto attrs = GetFileAttributes(fileName);
	auto isNewFile = false;
	HANDLE hFile = 0;
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		//file does not exist, so create one
		isNewFile = true;
		hFile = ::CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
			nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	}
	else
	{
		WIN32_FILE_ATTRIBUTE_DATA shaderFileInfo;
		WIN32_FILE_ATTRIBUTE_DATA cacheFileInfo;
		GetFileAttributesEx(shaderFileName, GetFileExInfoStandard, &shaderFileInfo);
		GetFileAttributesEx(fileName, GetFileExInfoStandard, &cacheFileInfo);

		ULARGE_INTEGER shaderLastModified, cacheLastModified;
		shaderLastModified.LowPart = shaderFileInfo.ftLastWriteTime.dwLowDateTime;
		shaderLastModified.HighPart = shaderFileInfo.ftLastWriteTime.dwHighDateTime;
		cacheLastModified.LowPart = cacheFileInfo.ftLastWriteTime.dwLowDateTime;
		cacheLastModified.HighPart = cacheFileInfo.ftLastWriteTime.dwHighDateTime;

		if (shaderLastModified.QuadPart > cacheLastModified.QuadPart)
		{
			//shader has been modified, so overwrite the cache file
			hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
				nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			isNewFile = true;
		}
		else
		{
			//cache is up-to-date
			hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
				nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		}
	}
	if (isNewFile)
	{
		//the file is new, so no need to search for anything, just white down
		writeCacheEntry(hFile, entry);
		CloseHandle(hFile);
		return;
	}
	else
	{
		//scan file for entry
		CacheEntry curr;
		while (readCacheEntry(hFile, curr, false))
		{
			if (!strcmp(curr.key, entry.key))
			{
				//entry found, stop
				CloseHandle(hFile);
				return;
			}
		}
		//we're at the end, and entry is not found, so add one
		writeCacheEntry(hFile, entry);
		CloseHandle(hFile);
		return;
	}
}

bool
ShaderDiskCache::searchForEntry(
    CacheEntry& entry,
    const std::string& shaderFileName)
{
	std::string cacheFileName;
	createCacheFileName(shaderFileName, cacheFileName);
	return searchForEntry(entry, cacheFileName.c_str(), shaderFileName.c_str());
}

void
ShaderDiskCache::addCacheEntry(
    const CacheEntry & entry,
    const std::string& shaderFileName)
{
	std::string cacheFileName;
	createCacheFileName(shaderFileName, cacheFileName);
	addCacheEntry(entry, cacheFileName.c_str(), shaderFileName.c_str());
}
