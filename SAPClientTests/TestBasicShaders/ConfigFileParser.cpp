#include "ConfigFileParser.h"

using namespace std;

#define LINE_MAX_LENGTH 200

size_t FindNonSpaceIndex(const string& str, size_t startIndex, bool lookAhead)
{
	size_t strLen = str.length();
	while ((str[startIndex] == ' ' || str[startIndex] == '\t') &&
	        ((lookAhead && startIndex < strLen) || (!lookAhead && startIndex > 0)))
	{
		startIndex += lookAhead ? 1 : -1;
	}
	return startIndex;
}

size_t FindSpaceIndex(const string& str, size_t startIndex, bool lookAhead)
{
	size_t strLen = str.length();
	while (str[startIndex] != ' ' && str[startIndex] != '\t' &&
	        ((lookAhead && startIndex < strLen) || (!lookAhead && startIndex > 0)))
	{
		startIndex += lookAhead ? 1 : -1;
	}
	return startIndex;
}

bool IsPropertyLine(const string& str)
{
	return str.find('=') != str.npos;
}

void ParseProperty(const string& str, StringMap * properties)
{
	size_t pos = str.find('=');
	if (pos != str.npos)
	{
		//we've got "property", so parse it to get name and value(s), strip off comments and put in in the map
		size_t nameEndPos = pos;
		//get rid of spaces between property name and equal sign
		nameEndPos = FindNonSpaceIndex(str, nameEndPos - 1, false) + 1;
			
		string propName = str.substr(0, nameEndPos);
		size_t valueStartPos = pos + 1;
		valueStartPos = FindNonSpaceIndex(str, valueStartPos, true);
			
		size_t commentStartPos = str.find(CONFIG_COMMENT_SYMBOL, nameEndPos);
		size_t valueEndPos = str.length();
		if (commentStartPos != str.npos)
		{
			//there are comments at the end of the line, get rid of them
			valueEndPos = FindNonSpaceIndex(str, commentStartPos - 1, false) + 1;
		}
		string value = str.substr(valueStartPos, valueEndPos - valueStartPos);
		properties->insert(make_pair(propName, value));
	}
}

bool IsBlockStart(const string& str, string * blockName)
{
	bool isBlock = str.substr(0, 6) == "BEGIN_";
	if (!isBlock)
		return false;
	//find out block name
	size_t pos = 6;
	size_t strLen = str.length();
	while (str[pos] != ' ' && pos < strLen)
	{
		pos++;
	}
	*blockName = str.substr(6, pos - 6);
	return true;
}

bool IsBlockEnd(const string& str, const string& blockName)
{
	string endMarker = string("END_") + blockName;
	size_t pos = 0;
	size_t strLen = str.length();
	while (str[pos] != ' ' && pos < strLen)
	{
		pos++;
	}
	return str.substr(0, pos) == endMarker;
}

const ConfigFileParser * ConfigFileParser::ParseFile(const char * fileName)
{
	ifstream config(fileName, ifstream::in);
	if (config.bad())
		return nullptr;
	StringMap * properties = new StringMap();
	BlocksMap * blockMap = new BlocksMap();
	bool inBlock = false;
	string blockName;
	ConfigFileBlock block;
	while (!config.eof())
	{
		char line[LINE_MAX_LENGTH];
		config.getline(line, 200);
		string str(line);
		if (str.length() == 0)
		{
			//empty line, skip it
			continue;
		}
		size_t index = FindNonSpaceIndex(str, 0, true);
		if (index >= str.length())
		{
			continue;
		}
		if (str[index] == CONFIG_COMMENT_SYMBOL)
		{
			//we've got full-line comment, ignore whole line
			continue;
		}
		//four cases are possible (aside from comments):
		//  1. we are already inside a block, then check for end or push data back to block
		//	2. name = value, "property"
		//	3. BEGIN_something, "block", keep fetching lines until encounter END_something
		//	4. things like "BASE-V2.0" - config file format specifier
		if (inBlock)
		{
			if (!IsBlockEnd(str, blockName))
			{
				str = str.substr(index);
				block.contents.push_back(str);
				continue;
			}
			inBlock = false;
			blockMap->insert(make_pair(blockName, block));
		}
		else if (IsPropertyLine(str))
		{
			//property
			ParseProperty(str, properties);
		}
		else if (IsBlockStart(str, &blockName))
		{
			inBlock = true;
			block.contents.clear();
		}
		else
		{
			//ignore fourth case for now
		}
	}
	ConfigFileParser * result = new ConfigFileParser(properties, blockMap);
	return result;
}

ConfigFileParser::ConfigFileParser(StringMap * properties, BlocksMap * blocks)
{
	m_mapProperties = properties;
	m_mapBlocks = blocks;
}

bool ConfigFileParser::GetIntProperty(char * propName, int * value) const
{
	string propNameStr(propName);
	if (m_mapProperties->count(propNameStr) == 0)
	{
		return false;
	}
	string & val = (*m_mapProperties)[propNameStr];
	*value = atoi(val.c_str());
	return true;
}

bool ConfigFileParser::GetDoubleProperty(char * propName, double * value) const
{
	string propNameStr(propName);
	if (m_mapProperties->count(propNameStr) == 0)
	{
		return false;
	}
	string & val = (*m_mapProperties)[propNameStr];
	*value = atof(val.c_str());
	return true;
}

bool ConfigFileParser::GetStringProperty(char * propName, std::string * value) const
{
	string propNameStr(propName);
	if (m_mapProperties->count(propNameStr) == 0)
	{
		return false;
	}
	*value = (*m_mapProperties)[propNameStr];
	return true;
}

ConfigFileParser::~ConfigFileParser(void)
{
	delete m_mapBlocks;
	delete m_mapProperties;
}
