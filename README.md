map2sym
=======

map2sym is simple tool and runtime component for resolving symbols in runtime. 
map2sym tool uses .map file to generate symbol database and it works for both 
MSVC and GCC produced executables.

Notes
-----

map2sym parses regular MSVC mapfile, but GCC mapfile needs to be generated with 
nm binutils tool.

	nm --demangle --numeric-sort --line-numbers <executable> > <mapfile>

	or short form:

	nm -Cnl <executable> > <mapfile>

Example
-------

	#include <stdio.h>
	#include <stdlib.h> // strtoull
	#include "symlookup.h"
	
	int main(int _argc, const char** _argv)
	{
		uint64_t addr = strtoull(_argv[2], NULL, 16);
		char temp[1024];

		SymbolLookup sym;
		sym.open(_argv[1]);
		const SymbolNode* node = sym.find(addr);
		sym.getName(node, temp, sizeof(temp) );
		printf("0x%08llx -> 0x%08llx\n'%s'\n\n", addr, node->addr, temp);

		sym.getFile(node, temp, sizeof(temp) );
		printf("%s (%d)\n", temp, node->line);

		return 0;
	}

	Usage:
	example <symbol db file> <address>

Contact
-------

Twitter @bkaradzic

Web http://www.stuckingeometry.com

License
-------

Copyright 2010-2011 Branimir Karadzic. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.

