/*
 * admintool.c	-- admin tools, moves from SYSOP-only-tool
 *
 * A part of Firebird BBS Project
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
 * CVS: $Id: admintool.c,v 1.11 2002/07/28 12:13:04 edwardc Exp $
 */

#ifndef WITHOUT_ADMIN_TOOLS
#include "bbs.h"

extern int cmpbnames();
extern int numboards;
extern int cleanmail();
extern char *chgrp();
extern int dowall();
extern int cmpuids();
extern int t_cmpuids();

int  showperminfo(int, int);
char    cexplain[STRLEN];
char	buf2[STRLEN];
char    lookgrp[30];
FILE   *cleanlog;

int
m_info()
{
	struct userec uinfo;
	int     id;
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("­×§ï¨Ï¥ÎªÌ¸ê®Æ");
	move(1, 0);
	usercomplete("½Ð¿é¤J¨Ï¥ÎªÌ¥N¸¹: ", genbuf);
	if (*genbuf == '\0') {
		clear();
		return -1;
	}
	if (!(id = getuser(genbuf))) {
		move(3, 0);
		prints("¿ù»~ªº¨Ï¥ÎªÌ¥N¸¹");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	memcpy(&uinfo, &lookupuser, sizeof(uinfo));

	move(1, 0);
	clrtobot();
	disply_userinfo(&uinfo, 1);
	uinfo_query(&uinfo, 1, id);
	return 0;
}

int
m_newbrd()
{
	struct boardheader newboard;
	extern int numboards;
	char    ans[4];
	char    vbuf[100];
	char   *group;
	int     bid;


	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("¶}±Ò·s°Q½×°Ï");
	memset(&newboard, 0, sizeof(newboard));
	move(2, 0);
	ansimore2("etc/boardref", NA, 3, 7);
	while (1) {
		getdata(10, 0, "°Q½×°Ï¦WºÙ:   ", newboard.filename, 18, DOECHO, YEA);
		if (newboard.filename[0] != 0) {
			struct boardheader dh;
			if (search_record(BOARDS, &dh, sizeof(dh), cmpbnames, newboard.filename)) {
				prints("\n¿ù»~! ¦¹°Q½×°Ï¤w¸g¦s¦b!!");
				pressanykey();
				return -1;
			}
		} else
			return -1;
		if (valid_brdname(newboard.filename))
			break;
		prints("\n¤£¦Xªk¦WºÙ!!");
	}
	newboard.flag = 0;
	while (1) {
		getdata(11, 0, "°Q½×°Ï»¡©ú:   ", newboard.title, 60, DOECHO, YEA);
		if (newboard.title[0] == '\0')
			return -1;
		if (strstr(newboard.title, "¡´") || strstr(newboard.title, "¡ó")) {
			newboard.flag |= OUT_FLAG;
			break;
		} else if (strstr(newboard.title, "¡³")) {
			newboard.flag &= ~OUT_FLAG;
			break;
		} else
			prints("¿ù»~ªº®æ¦¡, µLªk§PÂ_¬O§_Âà«H!!");
	}
	strcpy(vbuf, "vote/");
	strcat(vbuf, newboard.filename);
	setbpath(genbuf, newboard.filename);
	if (getbnum(newboard.filename) > 0 || mkdir(genbuf, 0755) == -1
		|| mkdir(vbuf, 0755) == -1) {
		prints("\n¿ù»~ªº°Q½×°Ï¦WºÙ!!\n");
		pressreturn();
		clear();
		return -1;
	}
	getdata(12, 0, "°Q½×°ÏºÞ²z­û: ", newboard.BM, BM_LEN - 1, DOECHO, YEA);
	move(13, 0);
	if (askyn("¬O§_­­¨î¦s¨úÅv¤O", NA, NA) == YEA) {
		getdata(14, 0, "­­¨î Read/Post? [R]: ", ans, 2, DOECHO, YEA);
		if (*ans == 'P' || *ans == 'p')
			newboard.level = PERM_POSTMASK;
		else
			newboard.level = 0;
		move(1, 0);
		clrtobot();
		move(2, 0);
		prints("³]©w %s Åv¤O. °Q½×°Ï: '%s'\n",
			(newboard.level & PERM_POSTMASK ? "POST" : "READ"),
			newboard.filename);
		newboard.level = setperms(newboard.level, "Åv­­", NUMPERMS, showperminfo);
		clear();
	} else
		newboard.level = 0;
	move(14, 0);
	if (askyn("¬O§_¥[¤J°Î¦WªO", NA, NA) == YEA)
		newboard.flag |= ANONY_FLAG;
	else
		newboard.flag &= ~ANONY_FLAG;
	if ((bid = getbnum("")) > 0) {
		substitute_record(BOARDS, &newboard, sizeof(newboard), bid);
	} else if (append_record(BOARDS, &newboard, sizeof(newboard)) == -1) {
		pressreturn();
		clear();
		return -1;
	}
	group = chgrp();
	if (group != NULL) {
		if (newboard.BM[0] != '\0')
			snprintf(vbuf, 100, "%-38.38s(BM: %s)", newboard.title + 8, newboard.BM);
		else
			snprintf(vbuf, 100, "%-38.38s", newboard.title + 8);

		if (add_grp(group, cexplain, newboard.filename, vbuf) == -1)
			prints("\n¦¨¥ßºëµØ°Ï¥¢±Ñ....\n");
		else
			prints("¤w¸g¸m¤JºëµØ°Ï...\n");
	}
	numboards = -1;
	prints("\n·s°Q½×°Ï¦¨¥ß\n");
	{
		char    secu[STRLEN];
		snprintf(secu, STRLEN, "¦¨¥ß·sªO¡G%s", newboard.filename);
		securityreport(secu);
	}
	pressreturn();
	clear();
	return 0;
}

int
m_editbrd()
{
	char    bname[STRLEN], buf[STRLEN], oldtitle[STRLEN], vbuf[256],
	       *group;
	char    oldpath[STRLEN], newpath[STRLEN], tmp_grp[30];
	int     pos, noidboard, a_mv;
	struct boardheader fh, newfh;
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("­×§ï°Q½×°Ï¸ê°T");
	move(1, 0);
	make_blist();
	namecomplete("¿é¤J°Q½×°Ï¦WºÙ: ", bname);
	if (*bname == '\0') {
		move(2, 0);
		prints("¿ù»~ªº°Q½×°Ï¦WºÙ");
		pressreturn();
		clear();
		return -1;
	}
	pos = search_record(BOARDS, &fh, sizeof(fh), cmpbnames, bname);
	if (!pos) {
		move(2, 0);
		prints("¿ù»~ªº°Q½×°Ï¦WºÙ");
		pressreturn();
		clear();
		return -1;
	}
	noidboard = fh.flag & ANONY_FLAG;
	move(3, 0);
	memcpy(&newfh, &fh, sizeof(newfh));
	prints("°Q½×°Ï¦WºÙ:   %s\n", fh.filename);
	prints("°Q½×°Ï»¡©ú:   %s\n", fh.title);
	prints("°Q½×°ÏºÞ²z­û: %s\n", fh.BM);
	prints("°Î¦W°Q½×°Ï:   %s\n", (noidboard) ? "Yes" : "No");
	strcpy(oldtitle, fh.title);
	prints("­­¨î %s Åv¤O: %s",
		(fh.level & PERM_POSTMASK) ? "POST" :
		(fh.level & PERM_NOZAP) ? "ZAP" : "READ",
		(fh.level & ~PERM_POSTMASK) == 0 ? "¤£³]­­" : "¦³³]­­");
	move(8, 0);
	if (askyn("¬O§_§ó§ï¥H¤W¸ê°T", NA, NA) == YEA) {
		move(9, 0);
		prints("ª½±µ«ö <Return> ¤£­×§ï¦¹Äæ¸ê°T...");

		while (1) {
			getdata(10, 0, "·s°Q½×°Ï¦WºÙ: ", genbuf, 18, DOECHO, YEA);
			if (*genbuf != 0) {
				struct boardheader dh;
				if (search_record(BOARDS, &dh, sizeof(dh), cmpbnames, genbuf)) {
					move(2, 0);
					prints("¿ù»~! ¦¹°Q½×°Ï¤w¸g¦s¦b!!");
					move(10, 0);
					clrtoeol();
					continue;
				}
				if (valid_brdname(genbuf)) {
					strncpy(newfh.filename, genbuf, sizeof(newfh.filename));
					strcpy(bname, genbuf);
					break;
				} else {
					move(2, 0);
					prints("¤£¦Xªkªº°Q½×°Ï¦WºÙ!");
					move(10, 0);
					clrtoeol();
					continue;
				}
			} else {
				break;
			}
		}
		ansimore2("etc/boardref", NA, 11, 7);
		while (1) {
			getdata(18, 0, "·s°Q½×°Ï»¡©ú: ", genbuf, 60, DOECHO, YEA);
			if (*genbuf != 0)
				strncpy(newfh.title, genbuf, sizeof(newfh.title));
			else
				break;
			if (strstr(newfh.title, "¡´") || strstr(newfh.title, "¡ó")) {
				newfh.flag |= OUT_FLAG;
				break;
			} else if (strstr(newfh.title, "¡³")) {
				newfh.flag &= ~OUT_FLAG;
				break;
			} else
				prints("\n¿ù»~ªº®æ¦¡, µLªk§PÂ_¬O§_Âà«H!!");
		}
		move(20, 0);
		clrtoeol();
		getdata(19, 0, "°Q½×°ÏºÞ²z­û: ", genbuf, 60, DOECHO, YEA);
		if (*genbuf != 0)
			strncpy(newfh.BM, genbuf, sizeof(newfh.BM));
		if (*genbuf == ' ')
			strncpy(newfh.BM, "\0", sizeof(newfh.BM));
		snprintf(buf, STRLEN, "°Î¦Wª© (Y/N)? [%c]: ", (noidboard) ? 'Y' : 'N');
		getdata(20, 0, buf, genbuf, 4, DOECHO, YEA);
		if (*genbuf == 'y' || *genbuf == 'Y' || *genbuf == 'N' || *genbuf == 'n') {
			if (*genbuf == 'y' || *genbuf == 'Y')
				newfh.flag |= ANONY_FLAG;
			else
				newfh.flag &= ~ANONY_FLAG;
		}
		if (a_mv != 1) {
			move(21, 0);
			if (askyn("¬O§_²¾°ÊºëµØ°Ïªº¦ì¸m", NA, NA) == YEA)
				a_mv = 2;
			else
				a_mv = 0;
			move(22, 0); 
		} else 
			move(21, 0);
		if (askyn("¬O§_§ó§ï¦s¨úÅv­­", NA, NA) == YEA) {
			char    ans[4];
			snprintf(genbuf, 1024, "­­¨î (R)¾\\Åª ©Î (P)±i¶K ¤å³¹ [%c]: ",
				(newfh.level & PERM_POSTMASK ? 'P' : 'R'));
			getdata(23, 0, genbuf, ans, 2, DOECHO, YEA);
			if ((newfh.level & PERM_POSTMASK) && (*ans == 'R' || *ans == 'r'))
				newfh.level &= ~PERM_POSTMASK;
			else if (!(newfh.level & PERM_POSTMASK) && (*ans == 'P' || *ans == 'p'))
				newfh.level |= PERM_POSTMASK;
			clear();
			move(2, 0);
			prints("³]©w %s '%s' °Q½×°ÏªºÅv­­\n",
				newfh.level & PERM_POSTMASK ? "±i¶K" : "¾\\Åª", newfh.filename);
			newfh.level = setperms(newfh.level, "Åv­­", NUMPERMS, showperminfo);
			clear();
			getdata(0, 0, "½T©w­n§ó§ï¶Ü? (Y/N) [N]: ", genbuf, 4, DOECHO, YEA);
		} else {
			getdata(23, 0, "½T©w­n§ó§ï¶Ü? (Y/N) [N]: ", genbuf, 4, DOECHO, YEA);
		}
		if (*genbuf == 'Y' || *genbuf == 'y') {
			{
				char    secu[STRLEN];
				snprintf(secu, STRLEN, "­×§ï°Q½×°Ï¡G%s(%s)", fh.filename, newfh.filename);
				securityreport(secu);
			}
			if (strcmp(fh.filename, newfh.filename)) {
				char    old[256], tar[256];
				a_mv = 1;
				setbpath(old, fh.filename);
				setbpath(tar, newfh.filename);
				rename(old, tar);
				snprintf(old, 256, "vote/%s", fh.filename);
				snprintf(tar, 256, "vote/%s", newfh.filename);
				rename(old, tar);
			}
			if (newfh.BM[0] != '\0')
				snprintf(vbuf, 256, "%-38.38s(BM: %s)", newfh.title + 8, newfh.BM);
			else
				snprintf(vbuf, 256, "%-38.38s", newfh.title + 8);
			get_grp(fh.filename);
			edit_grp(fh.filename, lookgrp, oldtitle + 8, vbuf);

			if (a_mv >= 1) {
				group = chgrp();
				get_grp(fh.filename);
				strcpy(tmp_grp, lookgrp);
				if (strcmp(tmp_grp, group) || a_mv != 2) {
					del_from_file("0Announce/.Search", fh.filename);
					if (group != NULL) {
						if (newfh.BM[0] != '\0')
							snprintf(vbuf, 256, "%-38.38s(BM: %s)", newfh.title + 8, newfh.BM);
						else
							snprintf(vbuf, 256, "%-38.38s", newfh.title + 8);

						if (add_grp(group, cexplain, newfh.filename, vbuf) == -1)
							prints("\n¦¨¥ßºëµØ°Ï¥¢±Ñ....\n");
						else
							prints("¤w¸g¸m¤JºëµØ°Ï...\n");
						snprintf(newpath, STRLEN, "0Announce/groups/%s/%s", group, newfh.filename);
						snprintf(oldpath, STRLEN, "0Announce/groups/%s/%s", tmp_grp, fh.filename);
						if (dashd(oldpath)) {
							deltree(newpath);
						}
						rename(oldpath, newpath);
						del_grp(tmp_grp, fh.filename, fh.title + 8);
					}
				}
			}
			substitute_record(BOARDS, &newfh, sizeof(newfh), pos);
			snprintf(genbuf, 1024, "§ó§ï°Q½×°Ï %s ªº¸ê®Æ --> %s",
				fh.filename, newfh.filename);
			report(genbuf);
			numboards = -1;	/* force re-caching */
		}
	}
	clear();
	return 0;
}

int
m_mclean()
{
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("²M°£¨p¤H«H¥ó");
	move(1, 0);
	prints("²M°£©Ò¦³¤wÅª¥B¥¼ mark ªº«H¥ó\n");
	if (askyn("½T©w¶Ü", NA, NA) == NA) {
		clear();
		return 0;
	} 
	securityreport("²M°£©Ò¦³¨Ï¥ÎªÌ¤wÅª«H¥ó¡C");

	cleanlog = fopen("mailclean.log", "w");
	move(3, 0);
	prints("½Ð­@¤ßµ¥­Ô.\n");
	refresh();
	if (apply_record(PASSFILE, cleanmail, sizeof(struct userec)) == -1) {
		move(4, 0);
		prints("apply PASSFILE err...\n");
		pressreturn();
		clear();
		return -1;
	}
	move(4, 0);
	fclose(cleanlog);
	prints("²M°£§¹¦¨! °O¿ýÀÉ mailclean.log.\n");
	pressreturn();
	clear();
	return 0;
}

int
m_trace()
{
	struct stat ostatb, cstatb;
	int     otflag, ctflag, done = 0;
	char    ans[3];
	char   *msg;
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("Set Trace Options");
	while (!done) {
		move(2, 0);
		otflag = stat("trace", &ostatb);
		ctflag = stat("trace.chatd", &cstatb);
		prints("¥Ø«e³]©w:\n");
		trace_state(otflag, "¤@¯ë", ostatb.st_size);
		trace_state(ctflag, "²á¤Ñ", cstatb.st_size);
		move(9, 0);
		prints("<1> ¤Á´«¤@¯ë°O¿ý\n");
		prints("<2> ¤Á´«²á¤Ñ°O¿ý\n");
		getdata(12, 0, "½Ð¿ï¾Ü (1/2/Exit) [E]: ", ans, 2, DOECHO, YEA);

		switch (ans[0]) {
		case '1':
			if (otflag) {
				touchfile("trace");
				msg = "¤@¯ë°O¿ý ON";
			} else {
				rename("trace", "trace.old");
				msg = "¤@¯ë°O¿ý OFF";
			}
			break;
		case '2':
			if (ctflag) {
				touchfile("trace.chatd");
				msg = "²á¤Ñ°O¿ý ON";
			} else {
				rename("trace.chatd", "trace.chatd.old");
				msg = "²á¤Ñ°O¿ý OFF";
			}
			break;
		default:
			msg = NULL;
			done = 1;
		}
		move(t_lines - 2, 0);
		if (msg) {
			prints("%s\n", msg);
			report(msg);
		}
	}
	clear();
	return 0;
}

int
m_register()
{
	FILE   *fn;
	char    ans[3], *fname;
	int     x, y, wid, len;
	char    uident[STRLEN];
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();

	stand_title("³]©w¨Ï¥ÎªÌµù¥U¸ê®Æ");
	for (;;) {
		getdata(1, 0, "(0)Â÷¶}  (1)¼f¬d·sµù¥U¸ê®Æ  (2)¬d¸ß¨Ï¥ÎªÌµù¥U¸ê®Æ ? : ",
			ans, 2, DOECHO, YEA);
		if (ans[0] == '1' || ans[0] == '2')
			break;
		else
			return 0;
	}
	if (ans[0] == '1') {
		fname = "new_register";
		if ((fn = fopen(fname, "r")) == NULL) {
			prints("\n\n¥Ø«e¨ÃµL·sµù¥U¸ê®Æ.");
			pressreturn();
		} else {
			y = 3, x = wid = 0;
			while (fgets(genbuf, STRLEN, fn) != NULL && x < 65) {
				if (strncmp(genbuf, "userid: ", 8) == 0) {
					move(y++, x);
					prints(genbuf + 8);
					len = strlen(genbuf + 8);
					if (len > wid)
						wid = len;
					if (y >= t_lines - 2) {
						y = 3;
						x += wid + 2;
					}
				}
			}
			fclose(fn);
			if (askyn("³]©w¸ê®Æ¶Ü", NA, YEA) == YEA) {
				securityreport("³]©w¨Ï¥ÎªÌµù¥U¸ê®Æ");
				scan_register_form(fname);
			}
		}
	} else {
		move(1, 0);
		usercomplete("½Ð¿é¤J­n¬d¸ßªº¥N¸¹: ", uident);
		if (uident[0] != '\0')
			if (!getuser(uident)) {
				move(2, 0);
				prints("¿ù»~ªº¨Ï¥ÎªÌ¥N¸¹...");
			} else {
				snprintf(genbuf, 1024, "home/%c/%s/register", toupper(lookupuser.userid[0]), lookupuser.userid);
				if ((fn = fopen(genbuf, "r")) != NULL) {
					prints("\nµù¥U¸ê®Æ¦p¤U:\n\n");
					for (x = 1; x <= 15; x++) {
						if (fgets(genbuf, STRLEN, fn))
							prints("%s", genbuf);
						else
							break;
					}
				} else
					prints("\n\n§ä¤£¨ì¥L/¦oªºµù¥U¸ê®Æ!!\n");
			}
		pressanykey();
	}
	clear();
	return 0;
}

int
d_board()
{
	struct boardheader binfo;
	int     bid, ans;
	char    bname[STRLEN];
	extern char lookgrp[];
	extern int numboards;
	if (!HAS_PERM(PERM_BLEVELS)) {
		return 0;
	}
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("§R°£°Q½×°Ï");
	make_blist();
	move(1, 0);
	namecomplete("½Ð¿é¤J°Q½×°Ï: ", bname);
	if (bname[0] == '\0')
		return 0;
	bid = getbnum(bname);
	if (get_record(BOARDS, &binfo, sizeof(binfo), bid) == -1) {
		move(2, 0);
		prints("¤£¥¿½Tªº°Q½×°Ï\n");
		pressreturn();
		clear();
		return 0;
	}
	ans = askyn("§A½T©w­n§R°£³o­Ó°Q½×°Ï", NA, NA);
	if (ans != 1) {
		move(2, 0);
		prints("¨ú®ø§R°£¦æ°Ê\n");
		pressreturn();
		clear();
		return 0;
	} 
	{
		char    secu[STRLEN];
		snprintf(secu, STRLEN, "§R°£°Q½×°Ï¡G%s", binfo.filename);
		securityreport(secu);
	}
	if (seek_in_file("0Announce/.Search", bname)) {
		move(4, 0);
		if (askyn("²¾°£ºëµØ°Ï", NA, NA) == YEA) {
			get_grp(binfo.filename);
			del_grp(lookgrp, binfo.filename, binfo.title + 8);
		}
	}
	if (seek_in_file("etc/junkboards", bname))
		del_from_file("etc/junkboards", bname);
	if (seek_in_file("0Announce/.Search", bname))
		del_from_file("0Announce/.Search", bname);

	if (binfo.filename[0] == '\0')
		return -1;	/* rrr - precaution */
	snprintf(genbuf, 1024, "boards/%s", binfo.filename);
	f_rm(genbuf);
	snprintf(genbuf, 1024, "vote/%s", binfo.filename);
	f_rm(genbuf);
	snprintf(genbuf, 1024, " << '%s' ³Q %s §R°£ >>",
		binfo.filename, currentuser.userid);
	memset(&binfo, 0, sizeof(binfo));
	strncpy(binfo.title, genbuf, STRLEN);
	binfo.level = PERM_SYSOP;
	substitute_record(BOARDS, &binfo, sizeof(binfo), bid);

	move(4, 0);
	prints("\n¥»°Q½×°Ï¤w¸g§R°£...\n");
	pressreturn();
	numboards = -1;
	clear();
	return 0;
}

int
d_user(cid)
char   *cid;
{
	int     id;
	char    secu[STRLEN];
	FILE	*fn;
	char	*ptr;
	char	buf[20];
	char	buf2[STRLEN];

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	stand_title("§R°£¨Ï¥ÎªÌ±b¸¹");
	move(1, 0);
	usercomplete("½Ð¿é¤J±ý§R°£ªº¨Ï¥ÎªÌ¥N¸¹: ", genbuf);
	if (*genbuf == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf))) {
		move(3, 0);
		prints("¿ù»~ªº¨Ï¥ÎªÌ¥N¸¹...");
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	prints("§R°£¨Ï¥ÎªÌ [%s].", genbuf);
	clrtoeol();
	move(2, 0);
	if (askyn("½T©w¶Ü", NA, NA) == NA) {
		move(2, 0);
		prints("¨ú®ø§R°£¨Ï¥ÎªÌ...\n");
		pressreturn();
		clear();
		return 0;
	}
	if (lookupuser.userid[0] == '\0' || !strcmp(lookupuser.userid, "SYSOP")) {
		prints("µLªk§R°£!!\n");
		pressreturn();
		clear();
		return 0;
	}
	snprintf(secu, STRLEN, "§R°£¨Ï¥ÎªÌ¡G%s", lookupuser.userid);
	securityreport(secu);
	sethomefile(genbuf,lookupuser.userid,"mentor");
	if (dashf(genbuf)) {
		if( (fn = fopen( genbuf, "r" )) != NULL ) {
			if( (ptr = strchr( buf, '\n' )) != NULL )
				*ptr = '\0';
			sethomefile(buf2,buf,"downline");
			del_from_file(buf2,lookupuser.userid);
		}
	}
	snprintf(genbuf, 1024, "mail/%c/%s", toupper(lookupuser.userid[0]), lookupuser.userid);
	f_rm(genbuf);
	snprintf(genbuf, 1024, "home/%c/%s", toupper(lookupuser.userid[0]), lookupuser.userid);
	f_rm(genbuf);
	lookupuser.userlevel = 0;
	strcpy(lookupuser.address, "");
	strcpy(lookupuser.username, "");
	strcpy(lookupuser.realname, "");
	strcpy(lookupuser.termtype, "");
	move(2, 0);
	prints("%s ¤w¸g³Q·Àµ´¤F...\n", lookupuser.userid);
	lookupuser.userid[0] = '\0';
	substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), id);
	setuserid(id, lookupuser.userid);
	pressreturn();
	clear();
	return 1;
}

int
x_level()
{
	int     id;
	unsigned int newlevel;
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	move(0, 0);
	prints("§ó§ï¨Ï¥ÎªÌÅv­­\n");
	clrtoeol();
	move(1, 0);
	usercomplete("¿é¤J±ý§ó§ïªº¨Ï¥ÎªÌ±b¸¹: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("³]©w¨Ï¥ÎªÌ '%s' ªºÅv­­ \n", genbuf);
	newlevel = setperms(lookupuser.userlevel, "Åv­­", NUMPERMS, showperminfo);
	move(2, 0);
	if (newlevel == lookupuser.userlevel)
		prints("¨Ï¥ÎªÌ '%s' Åv­­¨S¦³ÅÜ§ó\n", lookupuser.userid);
	else {
		lookupuser.userlevel = newlevel;
		{
			char    secu[STRLEN];
			snprintf(secu, STRLEN, "­×§ï %s ªºÅv­­", lookupuser.userid);
			securityreport(secu);
		}

		substitute_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
		prints("¨Ï¥ÎªÌ '%s' Åv­­¤w¸g§ó§ï§¹²¦.\n", lookupuser.userid);
	}
	pressreturn();
	clear();
	return 0;
}

void
a_edits()
{
	int     aborted;
	char    ans[7], buf[STRLEN], buf2[STRLEN];
	int     ch, num, confirm;
	static char *e_file[] =
	{"../Welcome", "../Welcome2", "issue", "logout", "../vote/notes",
		"menu.ini", "bad_id", "bad_email", "../.bansite", "../.blockmail",
		"autopost", "junkboards", "sysops", "expire.ctl", "whatdate", "../NOLOGIN", "../etc/hosts", "../innd/black.list.general", NULL};
	static char *explain_file[] =
	{"¯S®í¶i¯¸¤½§GÄæ", "¶i¯¸µe­±", "¶i¯¸ÅwªïÀÉ", "Â÷¯¸µe­±"
		,"¤½¥Î³Æ§Ñ¿ý", "menu.ini", "¤£¥iµù¥Uªº ID", "¤£¥i½T»{¤§E-Mail", "¤£¥i¤W¯¸¤§¦ì§}"
		,"©Ú¦¬E-mail¶Â¦W³æ", "¨C¤é¦Û°Ê°e«HÀÉ", "¤£ºâPOST¼ÆªºªO", "ºÞ²zªÌ¦W³æ", "©w®É¬å«H³]©wÀÉ",
		"¬ö©À¤é²M³æ", "NOLOGIN", "¤W¯¸¨Ó·½: Domain»P¤¤¤å¹ï·Ó", "Âà«H¶Â¦W³æ", NULL};
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	move(1, 0);
	prints("½s­×¨t²ÎÀÉ®×\n\n");
	for (num = 0; HAS_PERM(PERM_SYSOP) ? e_file[num] != NULL && explain_file[num] != NULL : explain_file[num] != "menu.ini"; num++) {
		prints("[[1;32m%2d[m] %s\n", num + 1, explain_file[num]);
	}
	prints("[[1;32m%2d[m] ³£¤£·Q§ï\n", num + 1);

	getdata(num + 5, 0, "§A­n½s­×­þ¤@¶µ¨t²ÎÀÉ®×: ", ans, 3, DOECHO, YEA);
	ch = atoi(ans);
	if (!isdigit(ans[0]) || ch <= 0 || ch > num || ans[0] == '\n' || ans[0] == '\0')
		return;
	ch -= 1;
	snprintf(buf2, STRLEN, "etc/%s", e_file[ch]);
	move(3, 0);
	clrtobot();
	snprintf(buf, STRLEN, "(E)½s¿è (D)§R°£ %s? [E]: ", explain_file[ch]);
	getdata(3, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		confirm = askyn("§A½T©w­n§R°£³o­Ó¨t²ÎÀÉ", NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("¨ú®ø§R°£¦æ°Ê\n");
			pressreturn();
			clear();
			return;
		} {
			char    secu[STRLEN];
			snprintf(secu, STRLEN, "§R°£¨t²ÎÀÉ®×¡G%s", explain_file[ch]);
			securityreport(secu);
		}
		unlink(buf2);
		move(5, 0);
		prints("%s ¤w§R°£\n", explain_file[ch]);
		pressreturn();
		clear();
		return;
	}
	modify_user_mode(EDITSFILE);
	aborted = vedit(buf2, NA);
	clear();
	if (aborted != -1) {
		prints("%s §ó·s¹L", explain_file[ch]);
		{
			char    secu[STRLEN];
			snprintf(secu, STRLEN, "­×§ï¨t²ÎÀÉ®×¡G%s", explain_file[ch]);
			securityreport(secu);
		}

		if (!strcmp(e_file[ch], "../Welcome")) {
			unlink("Welcome.rec");
			prints("\nWelcome °O¿ýÀÉ§ó·s");
		}
	}
	pressreturn();
}

int
wall()
{
	if (!HAS_PERM(PERM_SYSOP))
		return 0;
	modify_user_mode(MSG);
	move(2, 0);
	clrtobot();
	if (!get_msg("©Ò¦³¨Ï¥ÎªÌ", buf2, 1)) {
		return 0;
	}
	if (apply_ulist(dowall) == 0) {
		move(2, 0);
		prints("½u¤WªÅµL¤@¤H\n");
		pressanykey();
	}
	prints("\n¤w¸g¼s¼½§¹²¦...\n");
	pressanykey();
	return 1;
}

int
setsystempasswd()
{
	FILE   *pass;
	char    passbuf[20], prepass[20];
	modify_user_mode(ADMIN);
	if (strcmp(currentuser.userid, "SYSOP"))
		return;
	if (!check_systempasswd())
		return;
	getdata(2, 0, "½Ð¿é¤J·sªº¨t²Î±K½X: ", passbuf, 19, NOECHO, YEA);
	getdata(3, 0, "½T»{·sªº¨t²Î±K½X: ", prepass, 19, NOECHO, YEA);
	if (strcmp(passbuf, prepass))
		return;
	if ((pass = fopen("etc/.syspasswd", "w")) == NULL) {
		move(4, 0);
		prints("¨t²Î±K½XµLªk³]©w....");
		pressanykey();
		return;
	}
	fprintf(pass, "%s\n", genpasswd(passbuf));
	fclose(pass);
	move(4, 0);
	prints("¨t²Î±K½X³]©w§¹¦¨....");
	pressanykey();
	return;
}

int
x_csh()
{
	char    buf[PASSLEN];
	int     save_pager;
	int     magic;
	if (!HAS_PERM(PERM_SYSOP)) {
		return 0;
	}
	if (!check_systempasswd()) {
		return;
	}
	modify_user_mode(SYSINFO);
	clear();
	getdata(1, 0, "½Ð¿é¤J³q¦æ·t¸¹: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd(currentuser.passwd, buf)) {
		prints("\n\n·t¸¹¤£¥¿½T, ¤£¯à°õ¦æ¡C\n");
		pressreturn();
		clear();
		return;
	}
	randomize();
	magic = rand() % 1000;
	prints("\nMagic Key: %d", magic * 5 - 2);
	getdata(4, 0, "Your Key : ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !(atoi(buf) == magic)) {
		securityreport("Fail to shell out");
		prints("\n\nKey ¤£¥¿½T, ¤£¯à°õ¦æ¡C\n");
		pressreturn();
		clear();
		return;
	}
	securityreport("Shell out");
	modify_user_mode(SYSINFO);
	clear();
	refresh();
	reset_tty();
	save_pager = uinfo.pager;
	uinfo.pager = 0;
	update_utmp();
	do_exec("csh", NULL);
	restore_tty();
	uinfo.pager = save_pager;
	update_utmp();
	clear();
	return 0;
}

int
kick_user(userinfo)
struct user_info *userinfo;
{
	int     id, ind;
	struct user_info uin;
	struct userec kuinfo;
	char    kickuser[40], buffer[40];
	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
		modify_user_mode(ADMIN);
		stand_title("½ð¨Ï¥ÎªÌ¤U¯¸");
		move(1, 0);
		usercomplete("¿é¤J¨Ï¥ÎªÌ±b¸¹: ", kickuser);
		if (*kickuser == '\0') {
			clear();
			return 0;
		}
		if (!(id = getuser(kickuser))) {
			move(3, 0);
			prints("Invalid User Id");
			clrtoeol();
			pressreturn();
			clear();
			return 0;
		}
		move(1, 0);
		prints("½ð±¼¨Ï¥ÎªÌ : [%s].", kickuser);
		clrtoeol();
		move(2, 0);
		if (askyn("½T©w¶Ü?", NA, NA) == NA) {
			move(2, 0);
			prints("¨ú®ø½ð¨Ï¥ÎªÌ..\n");
			pressreturn();
			clear();
			return 0;
		}
		search_record(PASSFILE, &kuinfo, sizeof(kuinfo), cmpuids, kickuser);
		ind = search_ulist(&uin, t_cmpuids, id);
	} else {
		uin = *userinfo;
		strcpy(kickuser, uin.userid);
/*        id = getuser(kickuser);
        search_record(PASSFILE, &kuinfo, sizeof(kuinfo), cmpuids, kickuser);
        ind = search_ulist( &uin, t_cmpuids, id ); */
		ind = YEA;
	}
	if (!ind || !uin.active || (kill(uin.pid, 0) == -1)) {
		if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
			move(3, 0);
			prints("User Has Logged Out");
			clrtoeol();
			pressreturn();
			clear();
		}
		return 0;
	}
	kill(uin.pid, SIGHUP);
	snprintf(buffer, 40, "kicked %s", kickuser);
	report(buffer);
	snprintf(genbuf, 1024, "%s (%s)", kuinfo.userid, kuinfo.username);
	log_usies("KICK ", genbuf);
	uin.active = NA;
	uin.pid = 0;
	uin.invisible = YEA;
	uin.sockactive = 0;
	uin.sockaddr = 0;
	uin.destuid = 0;
	update_ulist(&uin, ind);
	move(2, 0);
	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
		prints("User has been Kicked\n");
		pressreturn();
		clear();
	}
	return 1;
}

#endif
