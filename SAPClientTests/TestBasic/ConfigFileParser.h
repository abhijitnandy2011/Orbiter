#pragma once
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#define CONFIG_COMMENT_SYMBOL ';'
typedef std::map<std::string, std::string> StringMap;
typedef std::vector<std::string> StringVector;

struct ConfigFileBlock
{
	StringVector contents;
};

typedef std::map<std::string, ConfigFileBlock> BlocksMap;

size_t FindNonSpaceIndex(const std::string& str, size_t startIndex, bool lookAhead = true);
size_t FindSpaceIndex(const std::string& str, size_t startIndex, bool lookAhead = true);


class ConfigFileBlockParser
{
protected:
	const StringVector& m_vecSource;

	std::string PeekFirstStringValue(const std::string& src) const
	{
		auto index = FindNonSpaceIndex(src, 0, true);
		auto endIndex = FindSpaceIndex(src, index, true);
		return src.substr(index, endIndex - index);
	}

	void SplitString(const std::string & src, std::vector<std::string>& vector, const char delimiter)
	{
		size_t currBeginIndex = 0;
		size_t currEndIndex = 0;
		auto size = src.length();
		while (currBeginIndex < size && currEndIndex < size)
		{
			currEndIndex = src.find(delimiter);
			std::string val = src.substr(currBeginIndex, currEndIndex - currBeginIndex);
			vector.push_back(val);
			currBeginIndex = currEndIndex + 1;
			currEndIndex = src.find(delimiter, currBeginIndex);
		}

	}
	
	template<typename T>
	bool ParseLine(const std::string& src, const std::string& format, T& dataContainer, int skipMembers = 0)
	{
		/*format string:
			Sxx - string with the length of xx, if 00 specified - accumes it's the part until space
			D	- double-precision value
			I	- integer value
		*/
		size_t fmtLen = format.length();
		size_t srcLen = src.length();
		size_t currentFmtIndex = 0;
		size_t currentSrcIndex = 0;
		size_t currentDataOffset = 0;
		while (currentFmtIndex < fmtLen && currentSrcIndex < srcLen)
		{
			char tokenType = format[currentFmtIndex++];
			switch(tokenType)
			{
				case 'S':
					{
						//string
						std::string symbolsCountStr = format.substr(currentFmtIndex, 2);
						int symbolsCount = atoi(symbolsCountStr.c_str());
						if (symbolsCount == 0)
						{
							int endIndex = FindSpaceIndex(src, currentSrcIndex, true);
							symbolsCount = endIndex - currentSrcIndex;
						}
						std::string val = src.substr(currentSrcIndex, symbolsCount);
						if (skipMembers-- <= 0)
						{
							SetToken(&dataContainer, val, currentDataOffset);
						}
						//tokens.push_back(val);

						currentSrcIndex += symbolsCount;
						currentSrcIndex = FindNonSpaceIndex(src, currentSrcIndex, true);
						currentFmtIndex += 2;
					}
					break;
				case 'D':
					{
						//double
						size_t dataEnd = FindSpaceIndex(src, currentSrcIndex, true);

						std::string val = src.substr(currentSrcIndex, dataEnd - currentSrcIndex);
						double dval = atof(val.c_str());
						if (skipMembers-- <= 0)
						{
							SetToken(&dataContainer, dval, currentDataOffset);
						}
						//tokens.push_back(val);

						currentSrcIndex += dataEnd - currentSrcIndex;
						currentSrcIndex = FindNonSpaceIndex(src, currentSrcIndex, true);
					}
					break;
				case 'I':
					{
						//int
						size_t dataEnd = FindSpaceIndex(src, currentSrcIndex, true);

						std::string val = src.substr(currentSrcIndex, dataEnd - currentSrcIndex);

						int ival = atoi(val.c_str());
						if (skipMembers-- <= 0)
						{
							SetToken(&dataContainer, ival, currentDataOffset);
						}
						//tokens.push_back(val);

						currentSrcIndex += dataEnd - currentSrcIndex;
						currentSrcIndex = FindNonSpaceIndex(src, currentSrcIndex, true);
					}
					break;
			}
		}
		return true;
	}

	template<typename T, typename TData>
	bool SetToken(T* dataStructure, TData dataItem, size_t& currentOffset)
	{
		void * pData = (void *)dataStructure;
		pData = (void *)((DWORD)pData + currentOffset);
		TData * objData = (TData *)pData;
		//this is important as copy constructor will be called if neccessary
		(*objData) = dataItem;
		currentOffset += sizeof(TData);
		return true;
	}
public:
	ConfigFileBlockParser(const StringVector& data)
		: m_vecSource(data)
	{
	}

	const StringVector& GetSource() const { return m_vecSource; }
};

class ConfigFileParser
{
private:

	StringMap * m_mapProperties;
	BlocksMap * m_mapBlocks;

	ConfigFileParser(StringMap * properties, BlocksMap * blocks);

public:
	virtual ~ConfigFileParser(void);

	bool GetIntProperty(char * propName, int * value) const;
	bool GetDoubleProperty(char * propName, double * value) const;
	bool GetStringProperty(char * propName, std::string * value) const;

	template<class T>
	T * GetBlock(char * blockName) const
	{
		std::string blockNameStr(blockName);
		if (m_blocks->count(blockNameStr) == 0)
		{
			return nullptr;
		}
		ConfigFileBlock & block = (*m_blocks)[blockNameStr];
		return new T(block.contents);
	}

	static const ConfigFileParser * ParseFile(const char * fileName);
};

