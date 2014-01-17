/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
$Id: pass.c,v 1.3 2000/01/29 23:19:09 edwardc Exp $
*/

#include "bbs.h"
#include <sys/param.h>
#include <sys/resource.h>
#include <pwd.h>

char   *crypt();

#if !defined(MD5) && !defined(DES)
 	/* nor DES, MD5, fatal error!! */
	#error "(pass.c) you've not define DES nor MD5, fatal error!!"
#endif

static unsigned char itoa64[] =	/* 0 ... 63 => ascii - 64 */
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void
to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v & 0x3f];
		v >>= 6;
	}
}

char   *
genpasswd(char *pw)
{
	char    salt[10];
	static char pwbuf[PASSLEN];
	struct timeval tv;
	if (strlen(pw) == 0)
		return "";

	srand(time(0) % getpid());
	gettimeofday(&tv, 0);

#ifdef MD5			/* use MD5 salt */
	strncpy(&salt[0], "$1$", 3);
	to64(&salt[3], random(), 3);
	to64(&salt[6], tv.tv_usec, 3);
	salt[8] = '\0';
#endif
#ifdef DES			/* use DES salt */
	to64(&salt[0], random(), 3);
	to64(&salt[3], tv.tv_usec, 3);
	to64(&salt[6], tv.tv_sec, 2);
	salt[8] = '\0';
#endif

	strcpy(pwbuf, pw);
	return crypt(pwbuf, salt);
}

int
checkpasswd(char *passwd, char *test)
{
	static char pwbuf[PASSLEN];
	char   *pw;
	strncpy(pwbuf, test, PASSLEN);
	pw = crypt(pwbuf, passwd);
	return (!strcmp(pw, passwd));
}
