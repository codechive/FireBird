/*
 * rehome.c				-- translate old user home directory hierarchy.
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
 * CVS: $Id: rehome.c,v 1.1 2000/01/15 01:45:43 edwardc Exp $
 */

/* in now. we use .PASSWDS.new */

#include "bbs.h"
#define  PASSWDS	".PASSWDS.new"

int
main()
{
	FILE           *rec;
	int             i;
	char            buf[256];
	char            buf2[256];
	struct userec   user;

	rec = fopen(PASSWDS, "rb");

	printf("1. Create Index directory\n");

	for (i = 'A'; i <= 'Z'; i++) {

		sprintf(buf, "%s/home/%c", BBSHOME, i);
		mkdir(buf, 0760);
		chown(buf, BBSUID, BBSGID);
		sprintf(buf, "%s/mail/%c", BBSHOME, i);
		mkdir(buf, 0760);
		chown(buf, BBSUID, BBSGID);

	}

	i = 0;

	printf("2. Moving User Directory\n");
	while (1) {

		if (fread(&user, sizeof(user), 1, rec) <= 0)
			break;

		i++;

		if ( user.numlogins <= 0 )
			continue;

		sprintf(buf, "%s/home/%s", BBSHOME, user.userid);
		sprintf(buf2, "%s/home/%c/%s", BBSHOME, toupper(user.userid[0]), user.userid);
		rename(buf, buf2);

		sprintf(buf, "%s/mail/%s", BBSHOME, user.userid);
		sprintf(buf2, "%s/mail/%c/%s", BBSHOME, toupper(user.userid[0]), user.userid);
		rename(buf, buf2);

	}
	printf("3. Done. total %d user home transfeered\n", i);
	fclose(rec);
}
