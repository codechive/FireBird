/*
 * chboard.c			-- translate old anonymous board flag setting.
 *
 * A part of Firebird BBS 3.0 utility kit
 *
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@firebird.dhs.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * CVS: $Id: chboard.c,v 1.1 2000/01/15 01:45:42 edwardc Exp $
 */

#include "bbs.h"

#define MBOARDS	BBSHOME"/"BOARDS

int
report() {}

int
main()
{
	FILE           *rec;
	int             i = 0;

	struct boardheader board;
	rec = fopen(MBOARDS, "rb");

	printf("Board Records Transfering...\n");

	while (1) {
	
		if (fread(&board, sizeof(board), 1, rec) <= 0)
			break;
			
		i++;
		
		printf("%d %s\t\n", i, board.filename);
		
		if ( !strcmp(board.filename, "anonymous") ) {
			board.flag |= ANONY_FLAG;
			printf("%s set to anonymous.\n", board.filename);
			substitute_record(MBOARDS, &board, sizeof(board), i);
		} else {
			board.flag &= ~ANONY_FLAG;
			substitute_record(MBOARDS, &board, sizeof(board), i);
		}
		
	}
	printf("\n%d Board Records Transferred...\n", i);
	fclose(rec);
}
