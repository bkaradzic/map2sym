/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __SYMLOOKUP_H__
#define __SYMLOOKUP_H__

#include <stdint.h>
#include <string.h>

struct SymbolNode
{
	uint64_t addr;
	uint32_t nodes;
	uint32_t name;
	uint16_t file;
	uint16_t line;
};

#if defined(_WIN32) || defined(_WIN64)
extern "C" char __ImageBase;
#endif // defined(_WIN32) || defined(_WIN64)

class SymbolLookup
{
public:
	SymbolLookup()
		: m_fixup(0)
		, m_symbols(NULL)
		, m_files(NULL)
		, m_db(NULL)
		, m_filesOffset(0)
		, m_namesOffset(0)
	{
		memset(&m_dummy, 0, sizeof(m_dummy) );
	}

	~SymbolLookup()
	{
		close();
	}

	void open(const char* _filePath, bool _fixup = false)
	{
		close();

		m_db = fopen(_filePath, "rb");
		if (NULL != m_db)
		{
			uint32_t numSymbols;
			fread(&numSymbols, 4, 1, m_db);

			uint16_t numFiles;
			fread(&numFiles, 2, 1, m_db);

			fread(&m_namesOffset, 4, 1, m_db);
			fread(&m_filesOffset, 4, 1, m_db);

			uint64_t imageBase;
			fread(&imageBase, 8, 1, m_db);
			m_fixup = 0;

#if defined(_WIN32) || defined(_WIN64)
			if (_fixup)
			{
				m_fixup = imageBase - (uint64_t)&__ImageBase;
			}
#endif // defined(_WIN32) || defined(_WIN64)

			m_symbols = new SymbolNode[numSymbols];
			fread(m_symbols, sizeof(SymbolNode), numSymbols, m_db);

			m_files = new uint32_t[numFiles];
			fread(m_files, sizeof(uint32_t), numFiles, m_db);
		}
	}

	void close()
	{
		delete [] m_files;
		m_files = NULL;

		delete [] m_symbols;
		m_symbols = NULL;

		if (NULL != m_db)
		{
			fclose(m_db);
			m_db = NULL;
		}
	}

	const SymbolNode* find(void* _addr)
	{
		uint64_t addr = (uintptr_t)_addr;
		return find(addr);
	}

	const SymbolNode* find(uint64_t _addr)
	{
		_addr += m_fixup;

		if (NULL != m_db)
		{
			const SymbolNode* last = m_symbols;
			const SymbolNode* current = m_symbols;
			while (0 != current->nodes && current->addr != _addr)
			{
				if (_addr < current->addr)
				{
					++current;
				}
				else if (_addr > current->addr)
				{
					last = current;
					current += current->nodes+1;
				}
			}

			if (0 == current->nodes && _addr < current->addr)
			{
				return last;
			}

			return current;
		}

		return &m_dummy;
	}

	void getName(const SymbolNode* _node, char* _out, uint16_t _len)
	{
		if (NULL != m_db)
		{
			uint32_t offset = _node->name+m_namesOffset;
			fseek(m_db, offset, SEEK_SET);
			fgets(_out, _len, m_db);
			char* pos = strchr(_out, ';');
			if (NULL != pos)
			{
				*pos = '\0';
			}
			_out[_len-1] = '\0';

			return;
		}

		strncpy(_out, "<unknown>", _len);
	}

	void getFile(const SymbolNode* _node, char* _out, uint16_t _len)
	{
		if (NULL != m_db)
		{
			_out[0] = '\0';
			uint32_t offset = m_files[_node->file]+m_filesOffset;
			fseek(m_db, offset, SEEK_SET);
			fgets(_out, _len, m_db);
			_out[_len-1] = '\0';

			return;
		}

		strncpy(_out, "<unknown>", _len);
	}

private:
	uint64_t m_fixup;
	SymbolNode* m_symbols;
	uint32_t* m_files;
	FILE* m_db;
	uint32_t m_namesOffset;
	uint32_t m_filesOffset;
	SymbolNode m_dummy;
};

#endif // __SYMLOOKUP_H__
