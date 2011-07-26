/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "commandline.h"
#include "foreach.h"

#include "symlookup.h"

#if defined(_MSC_VER)
#	define strtoull _strtoui64
#endif // defined(_MSC_VER)

typedef std::map<std::string, uint16_t> FileMap;
FileMap g_fileMap;

struct Symbol
{
	uint64_t addr;
	std::string name;
	uint16_t file;
	uint16_t line;
};

typedef std::vector<Symbol> SymbolArray;
SymbolArray g_symbols;

typedef std::map<uint64_t, Symbol> SymbolMap;
SymbolMap g_symbolMap;

typedef std::vector<std::string> StringArray;
StringArray g_symbolNames;
StringArray g_fileNames;

typedef std::vector<SymbolNode> SymbolNodeArray;
SymbolNodeArray g_symbolNodes;

uint32_t g_nameOffset = 0;
bool g_64bit = false;

void linearizeSymbols_r(uint32_t _start, uint32_t _end)
{
	uint32_t nodes = _end-_start;
	if (0 != nodes)
	{
		uint32_t index = _start+nodes/2;
		
		const Symbol& symbol = g_symbols[index];
		
		SymbolNode node;
		node.addr = symbol.addr;
		node.nodes = nodes/2;
		node.name = g_nameOffset;
		node.file = symbol.file;
		node.line = symbol.line;
		g_symbolNodes.push_back(node);
		g_symbolNames.push_back(symbol.name);
		
		g_nameOffset += (uint32_t)symbol.name.size()+1;
		
		linearizeSymbols_r(_start, index);
		linearizeSymbols_r(index+1, _end);
	}
}

void linearizeSymbols()
{
	g_nameOffset = 0;
	linearizeSymbols_r(0, (uint32_t)g_symbols.size() );
}

void saveSymbols(const char* _filePath)
{
	FILE* file = fopen(_filePath, "wt");
	
	for (SymbolMap::const_iterator it = g_symbolMap.begin(); it != g_symbolMap.end(); ++it)
	{
		fprintf(file, "%08x T %s\n", it->first, it->second.name.c_str() );
	}
	
	fclose(file);
}

#define UNDNAME_NAME_ONLY 0x1000
typedef const char* PCTSTR;
DWORD __stdcall dummyUnDecorateSymbolName(PCTSTR DecoratedName, PTSTR UnDecoratedName, DWORD UndecoratedLength, DWORD /*Flags*/)
{
	strncpy(UnDecoratedName, DecoratedName, UndecoratedLength);
	return (DWORD)strlen(UnDecoratedName);
}

typedef DWORD (__stdcall *UnDecorateSymbolNameFn)(PCTSTR DecoratedName, PTSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags);
UnDecorateSymbolNameFn UnDecorateSymbolName = &dummyUnDecorateSymbolName;

void loadMsvcMapfile(const char* _filePath)
{
	g_64bit = false;
	HMODULE dll = LoadLibraryA("dbghelp.dll");
	if (NULL != dll)
	{
		UnDecorateSymbolName = (UnDecorateSymbolNameFn)GetProcAddress(dll, "UnDecorateSymbolName");
	}
	else
	{
		printf("Unable to open dbghelp.dll.\n");
	}
	
	FILE* file = fopen(_filePath, "rb");
	if (NULL != file)
	{
		char temp[4096];
		
		bool convert = false;
		
		while (!feof(file) )
		{
			char* args = fgets(temp, 4096, file);
				
			if (NULL != args)
			{
				if (!convert)
				{
					convert = (NULL != strstr(args, "  Address         Publics by Value              Rva+Base") );
				}
				else
				{
					char buffer[4096];
					size_t size = sizeof(buffer);
					char* argv[50];
					int argc = tokenizeCommandLine(args, buffer, size, argv, 50);
					
					if (4 <= argc)
					{
						uint64_t addr = strtoull(argv[2], NULL, 16);
						g_64bit |= addr > 0xffffffffull;

						if (0 != addr)
						{
							char name[4096];
							UnDecorateSymbolName(argv[1], name, sizeof(name), UNDNAME_NAME_ONLY);
							
							if ('`' == name[0])
							{
								strncpy(name, argv[1], sizeof(name) );
							}

							SymbolMap::iterator it = g_symbolMap.find(addr);
							if (it == g_symbolMap.end() )
							{
								Symbol symbol;
								symbol.addr = addr;
								symbol.name = name;
								symbol.file = 0;
								symbol.line = 0;

								g_symbolMap.insert(std::make_pair(addr, symbol) );
							}
							else
							{
								it->second.name.append(";");
								it->second.name.append(name);
							}
						}
					}
				}
			}
		}
		
		fclose(file);
	}
}

void loadGccMapfile(const char* _filePath)
{
	g_64bit = false;
	FILE* file = fopen(_filePath, "rb");
	if (NULL != file)
	{
		char temp[4096];
		
		while (!feof(file) )
		{
			char* str = fgets(temp, 4096, file);
			if (NULL != str)
			{
				if (!isspace(*str) )
				{
					char info[12];
					strncpy(info, str, 11);
					
					info[8] = '\0';
					if ('T' == toupper(info[9]) )
					{
						char* cr = strchr(str, '\r');
						if (NULL != cr)
						{
							cr[0] = '\0';
						}
						char* lf = strchr(str, '\n');
						if (NULL != lf)
						{
							lf[0] = '\0';
						}
						
						uint64_t addr = strtoull(info, NULL, 16);
						g_64bit |= addr > 0xffffffffull;
						
						char* tok = strtok(str, "\t");
						std::string name = &tok[11];
						
						tok = strtok(NULL, "\t");
						
						std::string fileName;
						uint16_t line = 0;
						uint16_t file = 0;
						
						if (NULL != tok)
						{
							char* lineNumber = strrchr(tok, ':');
							if (NULL != lineNumber)
							{
								lineNumber[0] = '\0';
								++lineNumber;
								line = (uint16_t)atoi(lineNumber);
							}
							
							fileName = tok;

							FileMap::iterator it = g_fileMap.find(fileName);
							if (it == g_fileMap.end() )
							{
								file = (uint16_t)g_fileMap.size()+1;
								g_fileMap.insert(std::make_pair(fileName, file) );
							}
							else
							{
								file = it->second;
							}
						}
						
						SymbolMap::iterator it = g_symbolMap.find(addr);
						if (it == g_symbolMap.end() )
						{
							Symbol symbol;
							symbol.addr = addr;
							symbol.name = name;
							symbol.file = file;
							symbol.line = line;

							g_symbolMap.insert(std::make_pair(addr, symbol) );
						}
						else if ('.' == it->second.name.at(0) )
						{
							it->second.name = name;
						}
						else
						{
							it->second.name.append(";");
							it->second.name.append(name);
						}
					}
				}
			}
		}
		
		fclose(file);
	}
}

const char* g_banner = "Copyright 2010-2011 Branimir Karadzic. All rights reserved.\n"
					   "License: http://www.opensource.org/licenses/BSD-2-Clause\n";

void err(const char* _str)
{
	printf("%s\nUsage: map2sym -i <input file> -o <output file>\n"
		"\n%s"
		, g_banner
		, _str
		);

	exit(EXIT_FAILURE);
}

void patch(FILE* _file, uint32_t _offset)
{
	uint32_t current = ftell(_file);
	fseek(_file, _offset, SEEK_SET);
	fwrite(&current, 4, 1, _file);
	fseek(_file, current, SEEK_SET);
}

int main(int _argc, const char** _argv)
{
	if (1 == _argc)
	{
		err("");
	}

	CommandLine cmdLine(_argc, _argv);

	const char* inFilePath = cmdLine.findOption('i');
	if (NULL == inFilePath)
	{
		err("Input file must be specified.\n");
	}
	
	const char* outFilePath = cmdLine.findOption('o');
	if (NULL == outFilePath)
	{
		err("Output file must be specified.\n");
	}
	
	loadMsvcMapfile(inFilePath);
	if (0 == g_symbolMap.size() )
	{
		loadGccMapfile(inFilePath);
	}
	
	if (0 < g_symbolMap.size() )
	{
		for (SymbolMap::const_iterator it = g_symbolMap.begin(); it != g_symbolMap.end(); ++it)
		{
			g_symbols.push_back(it->second);
		}

		StringArray files;
		files.resize(g_fileMap.size()+1 );
		files[0] = "<unknown>";
		for (FileMap::const_iterator it = g_fileMap.begin(); it != g_fileMap.end(); ++it)
		{
			files[it->second] = it->first;
		}
		
		linearizeSymbols();
		
		FILE* out = fopen(outFilePath, "wb");
		
		uint32_t numSymbols = (uint32_t)g_symbolNodes.size();
		fwrite(&numSymbols, 4, 1, out);

		uint16_t numFiles = (uint16_t)files.size();
		fwrite(&numFiles, 2, 1, out);

		uint32_t dummy = 0;
		uint32_t namesOffset = ftell(out);
		fwrite(&dummy, 4, 1, out);
		
		uint32_t filesOffset = ftell(out);
		fwrite(&dummy, 4, 1, out);

		uint64_t imageBase = 0x00400000;
		if (g_64bit)
		{
			imageBase = 0x0000000140000000ull;
		}
		fwrite(&imageBase, 8, 1, out);

		foreach (const SymbolNode& symbolNode, g_symbolNodes)
		{
			fwrite(&symbolNode, sizeof(SymbolNode), 1, out);
		}
		
		uint32_t fileOffset = 0;
		foreach (const std::string& name, files)
		{
			fwrite(&fileOffset, 4, 1, out);
			fileOffset += (uint32_t)(name.size()+1);
		}

		patch(out, namesOffset);
		const char term = '\0';
		foreach (const std::string& name, g_symbolNames)
		{
			fwrite(name.c_str(), name.size(), 1, out);
			fwrite(&term, 1, 1, out);
		}

		patch(out, filesOffset);
		foreach (const std::string& name, files)
		{
			fwrite(name.c_str(), name.size(), 1, out);
			fwrite(&term, 1, 1, out);
		}

		fwrite(g_banner, strlen(g_banner)+1, 1, out);
		
		fclose(out);
	}
	else
	{
		printf("Failed to load mapfile.\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
