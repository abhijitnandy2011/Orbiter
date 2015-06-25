/*
 * The ShaderDiskCache class. This manages a cache which is realized in memory through a map in
 * ShaderManager. The actual storage is not here.
 *
 */
#pragma once

// System
#include <windows.h> // For HANDLE

// std
#include <string>

namespace Rendering {

struct CacheEntry
{
	UINT keyLength;
	UINT bytecodeLength;
	const char * key;
	const void * bytecode;
};

class ShaderDiskCache
{
public:
    ShaderDiskCache(std::string& cacheRoot);
    virtual ~ShaderDiskCache(void);

    bool searchForEntry(CacheEntry &entry, const std::string &shaderFileName);
    void addCacheEntry(const CacheEntry &entry, const std::string &shaderFileName);

private:
	void writeCacheEntry(HANDLE hFile, const CacheEntry &entry);
	UINT calculateCacheEntrySize(const CacheEntry &entry);
	bool readCacheEntry(HANDLE hFile, CacheEntry &entry, bool readBytecode);

	// entry contains the shader bytecode if the shader was found or its source wasnt modified
	// after the call to searchForEntry() completes
	bool searchForEntry(CacheEntry &entry, const TCHAR * fileName, const TCHAR * shaderFileName);

	void addCacheEntry(const CacheEntry &entry, const TCHAR * fileName, const TCHAR * shaderFileName);

	void createCacheFileName(const std::string &shaderFileName, std::string &result);

	// The directory root for the shader files
	std::string m_cacheDirectoryRoot;
};

} // namespace Rendering


