/*
 * mail-list.c		-- mailling-list module for mail2bbs
 *	
 * A part of Firebird BBS 3.0 series
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
 * CVS: $Id: mail-list.c,v 1.3 2002/07/10 11:22:31 edwardc Exp $
 */

#include "bbs.h"
#include "mail2bbs.h"

#ifdef MAILLIST_MODULE

/* anti-bailan */
#ifndef MAILLIST_RETURNBRD
  #error "Plz. comment out \"MAILLIST_RETURNBRD\" in mail2bbs.h !!"
#endif

#define CONF_FILE BBSHOME"/etc/mail-list.conf"

char *
seeksetting(char *sender)
{
	FILE *fp;
	char *xx,*bname,*from;

	xx = (char *)malloc(160);
	bname = (char *)malloc(80);
	from = (char *)malloc(80);
	
	fp = fopen(CONF_FILE, "r");
	if ( fp == NULL )
	    return 0;
		
	while ( fgets(xx, 80, fp) != 0 ) {
		if ( xx[0] == '#' || xx[0] == ' ' || xx[0] == ';' )
		    continue;
		sscanf(xx,"%s %s", from, bname);

		if ( strstr(sender,from) == 0 )
		    continue;
		else {
		    fclose(fp);
		    return bname;
		}
	}
	
	fclose(fp);
	free(xx);
	free(bname);
	free(from);
	
	return 0;
}

int
append_mailling(fin, title, sender, from, to, rto)
FILE *fin;
char *title, *sender, *from, *to, *rto;
{
	FILE *fout;
	char fbuf[80],buf[256], *bname;
	time_t now;

	bname = seeksetting(sender);
	if ( bname == NULL ) {
		bname = seeksetting(to);
		if ( bname == NULL ) {
			bname = (char *) malloc(STRLEN);
			strncpy(bname, MAILLIST_RETURNBRD, STRLEN);
		}
	}
	
	snprintf(fbuf, 80, "%s/tmp/mailling.%X",BBSHOME, time(0));
	fout = fopen(fbuf, "w");
	if ( fout == NULL || fin == NULL )
	   return 0;
	 
	fprintf(fout, "%s\n", sender); 
	fprintf(fout, "%s\n", title);
	
	fprintf(fout, "寄信人: %s, 信區: %s\n", sender , bname);
	fprintf(fout, "標  題: %s\n", title);
	fprintf(fout, "發信站: mail-list module for mail2bbs ($Revision: 1.3 $)\n");
	if ( from[0] != '\0' )
 	    fprintf(fout, "來  源: %s\n", from);
	now = time(0);
	fprintf(fout, "日  期: %s\n", ctime(&now));
	
	while ( fgets(buf, 254, fin) != 0 ) {
	      fputs(buf, fout);
	}

	fclose(fin);
	fclose(fout);
	
	snprintf(buf, 256, "%s/innd/bbspost post %s/boards/%s < %s", 
		    BBSHOME, BBSHOME, bname, fbuf);
	system(buf);
	unlink(fbuf);
	free(bname);
	return 0;

}

#endif /* _MAILLIST_MODULE_ */
