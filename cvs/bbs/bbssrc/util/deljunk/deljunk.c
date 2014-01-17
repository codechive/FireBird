/*
 * deljunk.c		-- delete junk mail in user mail box
 *	
 * A part of SEEDNetBBS generation 1
 *
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
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
 * CVS: $Id: deljunk.c,v 1.1 2000/01/15 01:45:30 edwardc Exp $
 */

#include <stdio.h>
#include <string.h>
#include "bbs.h"

int
main(int argc, char **argv)
{
	struct userec   userdata;
	struct fileheader file;
	FILE   *fp1, *fp, *fp_dir;
	char   filepath[200], filepath1[200], buf[200], bad_from[100];

	sprintf(buf,"%s/.PASSWDS", BBSHOME);
	fp1 = fopen(buf, "rb");

	if (fp1 == NULL) {
		printf("\nPASSWD Files open error !!\n");
		exit(1);
	}

	if (argc != 2) {
		printf(" Usage: deljunk [spammer@spammsite.blah.foo]\n");
		exit(1);
	}

	strcpy(bad_from, argv[1]);

	while (1) {
		if (fread(&userdata, sizeof(struct userec), 1, fp1) <= 0)
			break;

		if (userdata.userlevel != 0) {
			sprintf(filepath, "%s/mail/%c/%s/.DIR", BBSHOME
			     ,toupper(userdata.userid[0]), userdata.userid);

			sprintf(filepath1, "%s/mail/%c/%s/.DIR1", BBSHOME
			     ,toupper(userdata.userid[0]), userdata.userid);

			fp = fopen(filepath, "rb");	/* Open .DIR */
			fp_dir = fopen(filepath1, "wb");

			if (fp == NULL )
				continue;
			if (fp_dir == NULL) {
				printf("cannot open %s for writing!! \n", filepath1);
				exit(2);
			}
						
			while (1) {
				if (fread(&file, sizeof(struct fileheader), 1, fp) <= 0)
					break;

				if (strstr(file.owner, bad_from) != NULL) {
				
					printf("在 %13s 的信箱中找到垃圾郵件!! ", userdata.userid);
					sprintf(buf, "%s/mail/%c/%s/%s", BBSHOME, 
					toupper(userdata.userid[0]) ,userdata.userid, 
					file.filename);
					if ( unlink(buf) )
						printf("...已刪除 \n");
					else
						printf("...刪除失敗 \n");
						
				} else
					fwrite(&file, sizeof(struct fileheader), 1, fp_dir);
			}

			fclose(fp_dir);
			fclose(fp);
			unlink(filepath);
			if ( rename(filepath1, filepath) ) {
				printf("cannot rename .DIR1 to .DIR!, Stop.\n");
				exit(1);
			}
		}
	}

	fclose(fp1);
}
