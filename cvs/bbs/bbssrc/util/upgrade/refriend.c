/*
 * refriend.c			-- translate old friend record to Firebird 3.0
 *						   (purpose used for PH4,CCU and M series)
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
 * CVS: $Id: refriend.c,v 1.1 2000/01/15 01:45:43 edwardc Exp $
 */

#include "bbs.h"

struct oldfriend {
	char            id[13];
	char            exp[15];
};

struct newfriend {
	char            id[13];
	char            exp[40];
};

int
report() { }

int
transfer(char *uid)
{
	struct newfriend nf;
	char  *str, fname[80], dname[80], genbuf[128];
	FILE  *fp;

	memset(&nf, 0, sizeof(struct newfriend));

	sprintf(fname, "%s/home/%s/overrides", BBSHOME, uid);
	sprintf(dname, "%s/home/%s/friends", BBSHOME, uid);

	if ((fp = fopen(fname, "r")) == NULL) {
		return 0;
	}
	while (fgets(genbuf, 80, fp) != NULL) {
		if ((str = strtok(genbuf, " \n\r\t")) != NULL) {
			memset(&nf, 0, sizeof(struct newfriend));
			sprintf(nf.id, "%.12s", str);
			str = strtok(NULL, " \n\r\t");
			sprintf(nf.exp, "%.39s", (str == NULL) ? "\0" : str);
			append_record(dname, &nf, sizeof(nf));
		}
	}

	fclose(fp);
	chown(dname, (uid_t) BBSUID, (gid_t) BBSGID);
	unlink(fname);
	return 1;
}

int
transfer2(char *uid)
{
	struct oldfriend fh;
	struct newfriend nfh;
	char  fname[80], dname[80];
	FILE           *fp, *fp2;

	memset(&fh, 0, sizeof(struct oldfriend));
	memset(&nfh, 0, sizeof(struct newfriend));

	sprintf(fname, "%s/home/%c/%s/friends", BBSHOME, toupper(uid[0]), uid);
	sprintf(dname, "%s/home/%c/%s/friends.new", BBSHOME, toupper(uid[0]), uid);

	if ((fp = fopen(fname, "rb")) == NULL)
		return 0;

	if ((fp2 = fopen(dname, "wb")) == NULL)
		return 0;

	while (1) {
		if (fread(&fh, sizeof(fh), 1, fp) <= 0)
			break;

		memset(&nfh, 0, sizeof(struct newfriend));
		strcpy(nfh.id, fh.id);
		strcpy(nfh.exp, fh.exp);
		append_record(dname, &nfh, sizeof(nfh));
	}
	fclose(fp);
	fclose(fp2);
	if (chown(dname, (uid_t) BBSUID, (gid_t) BBSGID) != 0) {
		printf("\n\n cannot chown %s to %d:%d\n\n", dname, BBSUID, BBSGID);
		exit(2);
	}
	unlink(fname);
	if (rename(dname, fname) != 0) {
		printf("\n\n cannot rename %s to %s!!\n", dname, fname);
		exit(3);
	}
	return 1;
}

int
main()
{
	FILE           *rec;
	int             i = 0, j = 0;
	struct userec   user;
#ifdef PH4_CCU
	int             type = 1;
#else
	int				type = 0;
#endif
	char			genbuf[128];


#ifdef USEINBBBSHOME	
	sprintf(genbuf, "%s/.PASSWDS", BBSHOME);
#else
	sprintf(genbuf, ".PASSWDS.new");
#endif
	
	rec = fopen(genbuf, "rb");

	printf("[1;31;5mFriends Records Transfering to FB 3.0. %s[m", 
		( type == 1 ) ? "(Type 1)" : "Type 2");
		
	while (1) {

		if (fread(&user, sizeof(user), 1, rec) <= 0)
			break;
		if (user.numlogins <= 0)
			continue;

#ifdef PH4_CCU
		if (transfer(user.userid) == 1)
			printf("[1m%.12s[36m Type 1 Transferred[m\n", user.userid);
		else {
			printf("[1m%.12s[34m No overrides File...[m\n", user.userid);
			j++;
			continue;
		}
#else	
		if (transfer2(user.userid) == 1)
			printf("[1m%.12s[36m Transferred[m\n", user.userid);
		else {
			printf("[1m%.12s[34m No overrides File...[m\n", user.userid);
			j++;
			continue;
		}
#endif		
		i++;
	}
	printf("\n[1m%d [32mFriends Records Tranferred to Firebird BBS 3.0..%s[m\n", i
	, (type == 1) ? "(Type 1)" : "(Type 2)" );
	printf("%d records haven't file to transfeer (ignored)\n", j);
	fclose(rec);
}
