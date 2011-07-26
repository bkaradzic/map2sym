/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "commandline.h"

// Reference:
// http://msdn.microsoft.com/en-us/library/a1y7w461.aspx
int tokenizeCommandLine(const char* _commandLine, char* _outBuffer, size_t& _outBufferSize, char** _outArgv, int _outMaxArgvs)
{
	int argc = 0;
	const char* curr = _commandLine;
	char* currOut = _outBuffer;
	char term = ' ';
	bool sub = false;

	enum ParserState
	{
		SkipWhitespace,
		SetTerm,
		Copy,
		Escape,
		End,
	};

	ParserState state = SkipWhitespace;
	
	while ('\0' != *curr
	&&     argc < _outMaxArgvs)
	{
		switch (state)
		{
			case SkipWhitespace:
				for (; isspace(*curr); ++curr); // skip whitespace
				state = SetTerm;
				break;
				
			case SetTerm:
				if ('"' == *curr)
				{
					term = '"';
					++curr; // skip begining quote
				}
				else
				{
					term = ' ';
				}
				
				_outArgv[argc] = currOut;
				++argc;
				
				state = Copy;
				break;
				
			case Copy:
				if ('\\' == *curr)
				{
					state = Escape;
				}
				else if ('"' == *curr
					 &&  '"' != term)
				{
					sub = !sub;
				}
				else if (term != *curr || sub)
				{
					*currOut = *curr;
					++currOut;
				}
				else
				{
					state = End;
				}
				++curr;
				break;
				
			case Escape:
				{
					const char* start = --curr;
					for (; '\\' == *curr; ++curr);

					if ('"' != *curr)
					{
						size_t count = curr-start;

						curr = start;
						for (size_t ii = 0; ii < count; ++ii)
						{
							*currOut = *curr;
							++currOut;
							++curr;
						}
					}
					else
					{
						curr = start+1;
						*currOut = *curr;
						++currOut;
						++curr;
					}
				}
				state = Copy;
				break;
				
			case End:
				*currOut = '\0';
				++currOut;
				state = SkipWhitespace;
				break;
		}
	}
	
	*currOut = '\0';
	if (0 < argc
	&&  '\0' == _outArgv[argc-1][0])
	{
		--argc;
	}
	_outBufferSize = currOut - _outBuffer;
	
	return argc;
}
