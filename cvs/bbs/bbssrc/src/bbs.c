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
$Id: bbs.c,v 1.21 2002/09/08 00:06:24 edwardc Exp $
*/

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1999,2000\n
	Firebird BBS Project. All rights reserved. http://www.firebird.org.tw\n";
#endif
#include "bbs.h"
#include <time.h>
#if defined(BSD44)
#include <stdlib.h>

#elif defined(LINUX)
/* include nothing :-) */
#else

#include <rpcsvc/rstat.h>
#endif

int     mot;
struct postheader header;
static int quiting;
extern int numofsig;
int     continue_flag;
int     scrint = 0;
int     local_article;
int     readpost;
int     digestmode;
struct userec currentuser;
int     usernum;
char    currboard[STRLEN - BM_LEN];
char    currBM[BM_LEN - 1];
int     selboard = 0;

char    ReadPost[STRLEN] = "";
char    ReplyPost[STRLEN] = "";
int     FFLL = 0;

char   *
filemargin();
void    report();
void    board_usage();
void    cancelpost();
void    add_loginfo();
void    add_crossinfo();
int     thesis_mode();
/*For read.c*/
int     auth_search_down();
int     auth_search_up();
int     t_search_down();
int     t_search_up();
int     post_search_down();
int     post_search_up();
int     thread_up();
int     thread_down();
int     deny_user();
int     show_author();
int     SR_first_new();
int     SR_last();
int     SR_first();
int     SR_read();
int     SR_read();
int     SR_author();
int     SR_BMfunc();
int     Q_Goodbye();
int     t_friends();
int     s_msg();
int     b_notes_passwd();

extern int numboards;
extern time_t login_start_time;
extern char BoardName[];
extern int cmpbnames();
extern int toggle1, toggle2;
extern char fromhost[];

char    genbuf[1024];
char    quote_title[120], quote_board[120];
char    quote_file[120], quote_user[120];
#ifndef NOREPLY
char    replytitle[STRLEN];
#endif

struct shortfile *getbcache();
int     totalusers, usercounter;

int
check_stuffmode()
{
	if (uinfo.mode == RMAIL || (uinfo.mode == READING && junkboard()))
		return YEA;
	else
		return NA;
}

#ifndef BBSD
void
get_load(load)
double  load[];
{
#if defined(LINUX)
	FILE   *fp;
	fp = fopen("/proc/loadavg", "r");
	if (!fp)
		load[0] = load[1] = load[2] = 0;
	else {
		float   av[3];
		fscanf(fp, "%g %g %g", av, av + 1, av + 2);
		fclose(fp);
		load[0] = av[0];
		load[1] = av[1];
		load[2] = av[2];
	}
#elif defined(BSD44)
	getloadavg(load, 3);
#else
	struct statstime rs;
	rstat("localhost", &rs);
	load[0] = rs.avenrun[0] / (double) (1 << 8);
	load[1] = rs.avenrun[1] / (double) (1 << 8);
	load[2] = rs.avenrun[2] / (double) (1 << 8);
#endif
}
#endif

int
set_safe_record()
{
	struct userec tmp;
	extern int ERROR_READ_SYSTEM_FILE;
	if (get_record(PASSFILE, &tmp, sizeof(currentuser), usernum) == -1) {
		char    buf[STRLEN];
		snprintf(buf, STRLEN, "Error:Read Passfile %4d %12.12s", usernum, currentuser.userid);
		report(buf);
		ERROR_READ_SYSTEM_FILE = YEA;
		abort_bbs();
		return -1;
	}
	currentuser.numposts = tmp.numposts;
	currentuser.userlevel = tmp.userlevel;
	currentuser.numlogins = tmp.numlogins;
	currentuser.stay = tmp.stay;
	return;
}

char   *
sethomepath(buf, userid)
char   *buf, *userid;
{
	sprintf(buf, "home/%c/%s", toupper(userid[0]), userid);
	return buf;
}
/*Add by SmallPig*/
void
setqtitle(stitle)
char   *stitle;
{
	FFLL = 1;
	if (strncmp(stitle, "Re: ", 4) != 0 && strncmp(stitle, "RE: ", 4) != 0) {
		snprintf(ReplyPost, STRLEN, "Re: %s", stitle);
		strcpy(ReadPost, stitle);
	} else {
		strcpy(ReplyPost, stitle);
		strcpy(ReadPost, ReplyPost + 4);
	}
}

int
chk_currBM(BMstr)
char    BMstr[STRLEN - 1];
{
	char   *ptr;
	char    BMstrbuf[STRLEN - 1];
	if (HAS_PERM(PERM_BLEVELS))
		return YEA;
	if (!HAS_PERM(PERM_BOARDS))
		return NA;
	strcpy(BMstrbuf, BMstr);
	ptr = strtok(BMstrbuf, ",: ;|&()\0\n");
	while (1) {
		if (ptr == NULL)
			return NA;
		if (!strcmp(ptr, currentuser.userid	/* ,strlen(currentuser.us
				    erid) */ ))
			return YEA;
		ptr = strtok(NULL, ",: ;|&()\0\n");
	}
}

void
setquotefile(filepath)
char    filepath[];
{
	strcpy(quote_file, filepath);
}

char   *
sethomefile(buf, userid, filename)
char   *buf, *userid, *filename;
{
	sprintf(buf, "home/%c/%s/%s", toupper(userid[0]), userid, filename);
	return buf;
}

char   *
setuserfile(buf, filename)
char   *buf, *filename;
{
	sprintf(buf, "home/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, filename);
	return buf;
}

char   *
setbpath(buf, boardname)
char   *buf, *boardname;
{
	strcpy(buf, "boards/");
	strcat(buf, boardname);
	return buf;
}

char   *
setbdir(buf, boardname)
char   *buf, *boardname;
{
	char    dir[STRLEN];
	switch (digestmode) {
	case NA:
		strncpy(dir, DOT_DIR, STRLEN);
		break;
	case YEA:
		strncpy(dir, DIGEST_DIR, STRLEN);
		break;
	case 2:
		strncpy(dir, THREAD_DIR, STRLEN);
		break;

	}
	dir[STRLEN - 1] = '\0';
	sprintf(buf, "boards/%s/%s", boardname, dir);
	return buf;
}

char   *
setbfile(buf, boardname, filename)
char   *buf, *boardname, *filename;
{
	sprintf(buf, "boards/%s/%s", boardname, filename);
	return buf;
}

int
deny_me(bname)
char *bname;
{
	char    buf[STRLEN];
	
	if ( !bname )
		setbfile(buf, bname, "deny_users");
	else
		setbfile(buf, currboard, "deny_users");
		
	return seek_in_file(buf, currentuser.userid);
}
/*Add by SmallPig*/
void
shownotepad()
{
	modify_user_mode(NOTEPAD);
	ansimore("etc/notepad", YEA);
	clear();
	return;
}

int
uleveltochar(buf, lvl)
char   *buf;
unsigned int lvl;
{
	if (!(lvl & PERM_BASIC)) {
		strcpy(buf, "----- ");
		return 0;
	}
	if (lvl < PERM_DEFAULT) {
		strcpy(buf, "- --- ");
		return 1;
	}
	buf[0] = (lvl & (PERM_CLOAK)) ? 'C' : ' ';
	buf[1] = (lvl & (PERM_XEMPT)) ? 'X' : ' ';
	buf[2] = (lvl & (PERM_BOARDS)) ? 'B' : ' ';
	buf[3] = (lvl & (PERM_ACCOUNTS)) ? 'A' : ' ';
	if (lvl & PERM_CHATCLOAK)
		buf[3] = '#';
	buf[4] = (lvl & (PERM_SYSOP)) ? 'S' : ' ';
	buf[5] = (lvl & (PERM_DENYPOST)) ? 'p' : ' ';
	buf[6] = '\0';
	return 1;
}

#ifdef USE_CHINESE_DATE
char *
    Ctime(clock)
time_t *clock;
{
    static char chinese[STRLEN];
    static char wday[7][3] = {"¤é", "¤@", "¤G", "¤T", "¥|", "¤­", "¤»"};
    struct tm *t = localtime(clock);

    snprintf(chinese, STRLEN, "%4d/%.2d/%.2d (%s) %.2d:%.2d",
	    t->tm_year+1900, t->tm_mon + 1, t->tm_mday, wday[t->tm_wday],
	    t->tm_hour, t->tm_min);
    return (chinese);
}
#else
char   *
Ctime(clock)
time_t *clock;
{
	char   *foo;
	char   *ptr = ctime(clock);
        
	if (foo = strchr(ptr, '\n'))
		*foo = '\0';
                    
	return (ptr);
}
#endif

char *
    Ctime2(clock)
time_t *clock;
{
    static char chinese[STRLEN];

    struct tm *t = localtime(clock);

    snprintf(chinese, STRLEN, "%.2d/%.2d %.2d:%.2d:%.2d",
	    t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    return (chinese);
}

void
printutitle()
{
	move(2, 0);
	prints("[1;44m ½s ¸¹  ¨Ï¥ÎªÌ¥N¸¹     %-19s  #¤W¯¸ #%-4s %6s %-12s   [m\n",
#if defined(ACTS_REALNAMES)
		HAS_PERM(PERM_SYSOP) ? "¯u¹ê©m¦W" : "¨Ï¥ÎªÌ¼ÊºÙ",
#else
		"¨Ï¥ÎªÌ¼ÊºÙ",
#endif
		(toggle2 == 0) ? "¤å³¹" : (toggle2 == 1) ? "®É¼Æ" : "«H¥ó",
		HAS_PERM(PERM_SEEULEVELS) ? "µ¥  ¯Å" : "",
		(toggle1 == 0) ? "³Ìªñ¥úÁ{¤é´Á" :
		(toggle1 == 1) ? "³Ìªñ¥úÁ{¦aÂI" : "±b¸¹«Ø¥ß¤é´Á");
}


int
g_board_names(fhdrp)
struct boardheader *fhdrp;
{
	if ((fhdrp->level & PERM_POSTMASK) || HAS_PERM(fhdrp->level)
		|| (fhdrp->level & PERM_NOZAP)) {
		AddNameList(fhdrp->filename);
	}
	return 0;
}

void
make_blist()
{
	CreateNameList();
	apply_boards(g_board_names);
}

int
Select()
{
	modify_user_mode(SELECT);
	do_select(0, NULL, genbuf);
	return 0;
}

int
junkboard()
{
	if (strstr(JUNKBOARDS, currboard))
		return 1;
	else
		return 0;
}

int
Post()
{
	if (!selboard) {
		prints("\n\n¥ý¥Î (S)elect ¥h¿ï¾Ü¤@­Ó°Q½×°Ï¡C\n");
		pressreturn();
		clear();
		return 0;
	}
#ifndef NOREPLY
	*replytitle = '\0';
#endif
	do_post();
	return 0;
}

int
postfile(filename, nboard, posttitle, mode)
char   *filename, *nboard, *posttitle;
int     mode;
{
	char    bname[STRLEN];
	char    dbname[STRLEN];
	struct boardheader fh;
	if (search_record(BOARDS, &fh, sizeof(fh), cmpbnames, nboard) <= 0) {
		snprintf(bname, STRLEN, "%s °Q½×°Ï§ä¤£¨ì", nboard);
		report(bname);
		return -1;
	}
	in_mail = NA;
	strcpy(quote_board, nboard);
	strcpy(dbname, currboard);
	strcpy(currboard, nboard);
	strcpy(quote_file, filename);
	strcpy(quote_title, posttitle);
	post_cross('l', mode);
	strcpy(currboard, dbname);
	return;
}

int
get_a_boardname(bname, prompt)
char   *bname, *prompt;
{
	struct boardheader fh;
	make_blist();
	namecomplete(prompt, bname);
	if (*bname == '\0') {
		return 0;
	}
	if (search_record(BOARDS, &fh, sizeof(fh), cmpbnames, bname) <= 0) {
		move(1, 0);
		prints("¿ù»~ªº°Q½×°Ï¦WºÙ\n");
		pressreturn();
		move(1, 0);
		return 0;
	}
	return 1;
}
/* Add by SmallPig */
int
do_cross(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    bname[STRLEN];
	char    dbname[STRLEN];
	char    ispost[10];
	if (!HAS_PERM(PERM_POST))
		return DONOTHING;
	if (uinfo.mode != RMAIL)
		snprintf(genbuf, 1024, "boards/%s/%s", currboard, fileinfo->filename);
	else
		snprintf(genbuf, 1024, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, fileinfo->filename);
	strncpy(quote_file, genbuf, sizeof(quote_file));
	quote_file[sizeof(quote_file) - 1] = '\0';
	strcpy(quote_title, fileinfo->title);

	clear();
	if (!get_a_boardname(bname, "½Ð¿é¤J­nÂà¶Kªº°Q½×°Ï¦WºÙ: ")) {
		return FULLUPDATE;
	}
	if (!haspostperm(bname))
	{
		move( 1, 0 );
		prints("±z©|µLÅv­­¦b %s µoªí¤å³¹.\n",bname);
		pressreturn();
		return FULLUPDATE;
	}
	move(0, 0);
	clrtoeol();
	prints("Âà¸ü ' %s ' ¨ì %s ªO ", quote_title, bname);
	move(1, 0);
	getdata(1, 0, "(S)Âà«H (L)¥»¯¸ (A)¨ú®ø? [A]: ", ispost, 9, DOECHO, YEA);
	if (ispost[0] == 's' || ispost[0] == 'S' || ispost[0] == 'L' || ispost[0] == 'l') {
		strcpy(quote_board, currboard);
		strcpy(dbname, currboard);
		strcpy(currboard, bname);
		if (post_cross(ispost[0], 0) == -1) {
			pressreturn();
			move(2, 0);
			strcpy(currboard, dbname);
			return FULLUPDATE;
		}
		strcpy(currboard, dbname);
		move(2, 0);
		prints("' %s ' ¤wÂà¶K¨ì %s ªO \n", quote_title, bname);
	} else {
		prints("¨ú®ø");
	}
	move(2, 0);
	pressreturn();
	return FULLUPDATE;
}

void
readtitle()
{
	struct shortfile *bp;

	char    header[STRLEN], title[STRLEN];
	char    readmode[10];
	bp = getbcache(currboard);
	memcpy(currBM, bp->BM, BM_LEN - 1);
	if (currBM[0] == '\0' || currBM[0] == ' ') {
		strcpy(header, "¸Û¼xªO¥D¤¤");
	} else {
		snprintf(header, STRLEN, "ªO¥D: %s", currBM);
	}
	if (chkmail())
		strcpy(title, "[±z¦³«H¥ó]");
	else if ((bp->flag & VOTE_FLAG))
		snprintf(title, STRLEN, "¡° §ë²¼¤¤¡A«ö V ¶i¤J§ë²¼ ¡°");
	else
		strcpy(title, bp->title + 11); /* chinsan.20011229: ¥h±¼¤@¨Ç¤£¥²­nªºÅã¥Ü */

	showtitle(header, title);
	prints("Â÷¶}[[1;32m¡ö[m,[1;32me[m] ¿ï¾Ü[[1;32m¡ô[m,[1;32m¡õ[m] ¾\\Åª[[1;32m¡÷[m,[1;32mRtn[m] µoªí¤å³¹[[1;32mCtrl-P[m] ¬å«H[[1;32md[m] ³Æ§Ñ¿ý[[1;32mTAB[m] ¨D§U[[1;32mh[m]\n");
	if (digestmode == 0) {
		if (DEFINE(DEF_THESIS))	/* youzi 1997.7.8 */
			strcpy(readmode, "¥DÃD");
		else
			strcpy(readmode, "¤@¯ë");
	} else if (digestmode == 1)
		strcpy(readmode, "¤åºK");
	else if (digestmode == 2)
		strcpy(readmode, "¥DÃD");
	if (DEFINE(DEF_THESIS) && digestmode == 0)
		prints("[1;37;44m  ½s¸¹   %-12s %5s %-39s[%4s¦¡¬Ýª©] [m\n", "¥Z µn ªÌ", "¤é ´Á", " ¼Ð  ÃD", readmode);
	else
		prints("[1;37;44m  ½s¸¹   %-12s %5s %-41s[%4s¼Ò¦¡] [m\n", "¥Z µn ªÌ", "¤é ´Á", " ¼Ð  ÃD", readmode);
	clrtobot();
}

char   *
readdoent(num, ent)
int     num;
struct fileheader *ent;
{
	static char buf[128];
	time_t  filetime;
	char   *date;
	char   *TITLE, color[10];
	int     type;
#ifdef COLOR_POST_DATE
	struct tm *mytm;
#endif

	type = brc_unread(ent->filename) ? 'N' : ' ';
	if ((ent->accessed[0] & FILE_DIGEST) /* && HAS_PERM(PERM_MARKPOST) */ ) {
		if (type == ' ')
			type = 'g';
		else
			type = 'G';
	}
	if (ent->accessed[0] & FILE_MARKED) {
		switch (type) {
		case ' ':
			type = 'm';
			break;
		case 'N':
			type = 'M';
			break;
		case 'g':
			type = 'b';
			break;
		case 'G':
			type = 'B';
			break;
		}
	}
	filetime = atoi(ent->filename + 2);
	if (filetime > 740000000) {
		date = Ctime(&filetime) + 5;
	} else {
		date = "";
	}
	/*
	 * 990604.edwardc (¤Ñ¦wªù¨Æ¥ó¬ö©À¤é :p) ¤å³¹¤é´ÁÃC¦â
	 */

#ifdef COLOR_POST_DATE
	mytm = localtime(&filetime);
	strftime(buf, 5, "%w", mytm);
	snprintf(color, 10, "[1;%dm", 30 + atoi(buf) + 1);
#else
	strcpy(color, "");
#endif

	/* Re-Write By Excellent */

	/* 990604.edwardc ¶¶«K­«±Æ¤@¤UÁY±Æ .. */

	TITLE = ent->title;

	if ( TITLE[48] )
		strcpy(TITLE + 45, " ¡K");  /* §â¦h¾lªº string ¬å±¼ */
		
	if (FFLL == 0) {
		if (!strncmp("Re:", ent->title, 3) || !strncmp("RE:", ent->title, 3)) {
			snprintf(buf, 128, " %5d %c %-12.12s %s%5.5s[m  %-.48s ", num, type,
				ent->owner, color, date, TITLE);
		} else {
			snprintf(buf, 128, " %5d %c %-12.12s %s%5.5s[m  ¡´ %-.45s ", num, type,
				ent->owner, color, date, TITLE);
		}
	} else {
		if (!strncmp("Re:", ent->title, 3) || !strncmp("RE:", ent->title, 3)) {
			if (strcmp(ReplyPost, ent->title) == 0) {
				snprintf(buf, 128, " [1;36m%5d[m %c %-12.12s %s%5.5s[1;36m¡E%-.48s[m "
					,num, type, ent->owner, color, date, TITLE);
			} else {
				snprintf(buf, 128, " %5d %c %-12.12s %s%5.5s[m  %-.48s ", num, type,
					ent->owner, color, date, TITLE);
			}
		} else {
			if (strcmp(ReadPost, ent->title) == 0) {
				snprintf(buf, 128, " [1;33m%5d[m %c %-12.12s %s%5.5s[1;33m¡D¡´ %-.45s[m "
					,num, type, ent->owner, color, date, TITLE);
			} else {
				snprintf(buf, 128, " %5d %c %-12.12s %s%5.5s[m  ¡´ %-.45s ", num, type,
					ent->owner, color, date, TITLE);
			}
		}
	}
	return buf;
}

char    currfile[STRLEN];

int
cmpfilename(fhdr)
struct fileheader *fhdr;
{
	if (!strncmp(fhdr->filename, currfile, STRLEN))
		return 1;
	return 0;
}

int
cpyfilename(fhdr)
struct fileheader *fhdr;
{
	char    buf[STRLEN];
	time_t  tnow;
	struct tm *now;
	tnow = time(0);
	now = localtime(&tnow);

	snprintf(buf, STRLEN, "-%s", fhdr->owner);
	strncpy(fhdr->owner, buf, IDLEN + 2);
	snprintf(buf, STRLEN, "<< ¥»¤å³Q %s ©ó %d/%d %d:%02d:%02d §R°£ >>",
		currentuser.userid, now->tm_mon + 1, now->tm_mday,
		now->tm_hour, now->tm_min, now->tm_sec);
	strncpy(fhdr->title, buf, STRLEN);
	fhdr->filename[STRLEN - 1] = 'L';
	fhdr->filename[STRLEN - 2] = 'L';
	return 0;
}

int
read_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char   *t;
	char    buf[512];
	int     ch;
	int     cou;
	brc_addlist(fileinfo->filename);
	if (fileinfo->owner[0] == '-')
		return FULLUPDATE;
	clear();
	strcpy(buf, direct);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, 1024, "%s/%s", buf, fileinfo->filename);
	strncpy(quote_file, genbuf, sizeof(quote_file));
	strcpy(quote_board, currboard);
	strcpy(quote_title, fileinfo->title);
	quote_file[255] = fileinfo->filename[STRLEN - 2];
	strcpy(quote_user, fileinfo->owner);
#ifndef NOREPLY
	ch = ansimore(genbuf, NA);
#else
	ch = ansimore(genbuf, YEA);
#endif
#ifndef NOREPLY
	move(t_lines - 1, 0);
	clrtoeol();
	if (haspostperm(currboard)) {
		prints("[1;44;31m[¾\\Åª¤å³¹]  [33m¦^«H R ¢x µ²§ô Q,¡ö ¢x¤W¤@«Ê ¡ô¢x¤U¤@«Ê <Space>,¡õ¢x¥DÃD¾\\Åª ^X©Îp [m");
	} else {
		prints("[1;44;31m[¾\\Åª¤å³¹]  [33mµ²§ô Q,¡ö ¢x¤W¤@«Ê ¡ô¢x¤U¤@«Ê <Space>,<Enter>,¡õ¢x¥DÃD¾\\Åª ^X ©Î p [m");
	}

	/* Re-Write By Excellent */

	FFLL = 1;
	if (strncmp(fileinfo->title, "Re:", 3) != 0) {
		strcpy(ReplyPost, "Re: ");
		strncat(ReplyPost, fileinfo->title, STRLEN - 4);
		strcpy(ReadPost, fileinfo->title);
	} else {
		strcpy(ReplyPost, fileinfo->title);
		for (cou = 0; cou < STRLEN; cou++)
			ReadPost[cou] = ReplyPost[cou + 4];
	}

	refresh();
#ifndef BBSD
	sleep(1);
#endif	
	if (!(ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_PGUP))
		ch = egetch();

	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
		if (DEFINE(DEF_THESIS)) {	/* youzi */

			sread(0, 0, ent, 0, fileinfo);
			break;
		} else
			return READ_NEXT;
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	case 'Y':
	case 'R':
	case 'y':
	case 'r':
		do_reply(fileinfo->title);
		break;
	case Ctrl('R'):
		post_reply(ent, fileinfo, direct);
		break;
	case 'g':
		digest_post(ent, fileinfo, direct);
		break;
	case Ctrl('U'):
		sread(0, 1, ent, 1, fileinfo);
		break;
	case Ctrl('N'):
		sread(2, 0, ent, 0, fileinfo);
		sread(3, 0, ent, 0, fileinfo);
		sread(0, 1, ent, 0, fileinfo);
		break;
	case Ctrl('S'):
	case Ctrl('X'):
	case 'p':		/* Add by SmallPig */
		sread(0, 0, ent, 0, fileinfo);
		break;
	case Ctrl('A'):	/* Add by SmallPig */
		clear();
		show_author(0, fileinfo, '\0');
		return READ_NEXT;
		break;
	case 'S':		/* by youzi */
		if (!HAS_PERM(PERM_PAGE))
			break;
		clear();
		s_msg();
		break;

	default:
		break;
	}
#endif
	return FULLUPDATE;
}

int
skip_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	brc_addlist(fileinfo->filename);
	return GOTO_NEXT;
}

int
do_select(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    bname[STRLEN], bpath[STRLEN];
	struct stat st;
	move(0, 0);
	clrtoeol();
	prints("¿ï¾Ü¤@­Ó°Q½×°Ï (­^¤å¦r¥À¤j¤p¼g¬Ò¥i)\n");
	prints("¿é¤J°Q½×°Ï¦W («öªÅ¥ÕÁä¦Û°Ê·j´M): ");
	clrtoeol();

	make_blist();
	namecomplete((char *) NULL, bname);
	setbpath(bpath, bname);
	if ((*bname == '\0') || (stat(bpath, &st) == -1)) {
		move(2, 0);
		prints("¤£¥¿½Tªº°Q½×°Ï.\n");
		pressreturn();
		return FULLUPDATE;
	}
	if (!(st.st_mode & S_IFDIR)) {
		move(2, 0);
		prints("¤£¥¿½Tªº°Q½×°Ï.\n");
		pressreturn();
		return FULLUPDATE;
	}
	selboard = 1;
	brc_initial(bname);

	move(0, 0);
	clrtoeol();
	move(1, 0);
	clrtoeol();
	setbdir(direct, currboard);
	return NEWDIRECT;
}

int
digest_mode()
{
	extern char currdirect[STRLEN];
	if (digestmode == YEA) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = YEA;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
do_thread()
{
	char    buf[STRLEN];
	snprintf(buf, STRLEN, "Threading %s", currboard);
	report(buf);
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[1;5m¨t²Î³B²z¼ÐÃD¤¤, ½Ðµy­Ô...[m ");
	refresh();
	
	/* edwardc.001113 Yes, we're moving anything into SO ... */
	/* edwardc.020908 Oops, fixed problem that ret doesnt give initial value */
	{
		int d_ret = 0, p_ret = 0;
		int (*p)()= (void *)DL_get("bin/SO/thread.so:thread_main", &d_ret);
	
		if (p)
			p_ret = (*p)();

		if ( d_ret != 0 || p_ret != 0 ) {
		
			if ( d_ret == -1 )
				report("do_thread() error: -1");
			else if ( d_ret == -2 )
				report("do_thread() error: -2");
			else if ( d_ret == -3 )
				report("do_thread() error: -3");
				
			move(15, 0);
			clrtoeol();
			refresh();
			snprintf(genbuf, 80, "[1;31m¨Ï¥Î¥~±¾µ{¦¡µo¥Í¿ù»~¡AÀÉ®× thread.so ¿ù»~­È d: %d/p: %d ½Ð¦^³ø¯¸ªø[0m", d_ret, p_ret);
			prints("%s", genbuf);
			report(genbuf);
            pressanykey();
            return DONOTHING;
		}

		prints("\n");
		return FULLUPDATE;
	}
	
}

int
thread_mode()
{
	extern char currdirect[STRLEN];
	if (digestmode == 2) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = 2;
		setbdir(currdirect, currboard);
		do_thread();
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return PARTUPDATE;
		}
	}
	return NEWDIRECT;
}

int
dele_digest(char *dname, char *direc)
{
	char    digest_name[STRLEN];
	char    new_dir[STRLEN];
	char    buf[STRLEN];
	char   *ptr;
	struct fileheader fh;
	int     pos;
	strcpy(digest_name, dname);
	strcpy(new_dir, direc);

	digest_name[0] = 'G';
	ptr = strrchr(new_dir, '/') + 1;
	strcpy(ptr, DIGEST_DIR);
	strcpy(buf, currfile);
	strcpy(currfile, digest_name);
	pos = search_record(new_dir, &fh, sizeof(fh), cmpfilename, digest_name);
	if (pos <= 0) {
		return;
	}
	delete_file(new_dir, sizeof(struct fileheader), pos, cmpfilename);
	strcpy(currfile, buf);
	*ptr = '\0';

	snprintf(buf, STRLEN,  "%s%s", new_dir, digest_name);
	unlink(buf);
	
	return;
}

int
digest_post(ent, fhdr, direct)
int     ent;
struct fileheader *fhdr;
char   *direct;
{

	if (!chk_currBM(currBM)) {
		return DONOTHING;
	}
	if (digestmode == YEA)
		return DONOTHING;

	if (fhdr->accessed[0] & FILE_DIGEST) {
		fhdr->accessed[0] = (fhdr->accessed[0] & ~FILE_DIGEST);
		dele_digest(fhdr->filename, direct);
	} else {
		struct fileheader digest;
		char   *ptr, buf[64];
		memcpy(&digest, fhdr, sizeof(digest));
		digest.filename[0] = 'G';
		strcpy(buf, direct);
		ptr = strrchr(buf, '/') + 1;
		ptr[0] = '\0';
		snprintf(genbuf, 1024, "%s%s", buf, digest.filename);
		if (dashf(genbuf)) {
			fhdr->accessed[0] = fhdr->accessed[0] | FILE_DIGEST;
			substitute_record(direct, fhdr, sizeof(*fhdr), ent);
			return PARTUPDATE;
		}
		digest.accessed[0] = 0;
		sprintf(&genbuf[512], "%s%s", buf, fhdr->filename);
		link(&genbuf[512], genbuf);
		strcpy(ptr, DIGEST_DIR);
		if (get_num_records(buf, sizeof(digest)) > MAX_DIGEST) {
			move(3, 0);
			clrtobot();
			move(4, 10);
			prints("©êºp¡A§Aªº¤åºK¤å³¹¤w¸g¶W¹L %d ½g¡AµLªk¦A¥[¤J...\n", MAX_DIGEST);
			pressanykey();
			return PARTUPDATE;
		}
		append_record(buf, &digest, sizeof(digest));
		fhdr->accessed[0] = fhdr->accessed[0] | FILE_DIGEST;
	}
	substitute_record(direct, fhdr, sizeof(*fhdr), ent);
	return PARTUPDATE;
}
#ifndef NOREPLY
int
do_reply(title)
char   *title;
{
	strcpy(replytitle, title);
	
/*    MSIA BBS specific settings */
/*
	if (strstr("NanYang SinChew", currboard) || !strncmp(currboard, "CNA_", 4))
		post_article("Discuss");
	else
*/
		post_article(currboard);
	replytitle[0] = '\0';
	return FULLUPDATE;
}
#endif

int
garbage_line(str)
char   *str;
{
	int     qlevel = 0;
	while (*str == ':' || *str == '>') {
		str++;
		if (*str == ' ')
			str++;
		if (qlevel++ >= 1)
			return 1;
	}
	while (*str == ' ' || *str == '\t')
		str++;
	if (qlevel >= 1)
		if (strstr(str, "´£¨ì:\n") || strstr(str, ": ¡j\n") || strncmp(str, "==>", 3) == 0 || strstr(str, "ªº¤å³¹ ¡¼"))
			return 1;
	return (*str == '\n');
}
/* this is a ³´¨À for bad people to cover my program to his */

/* When there is an old article that can be included -jjyang */
void
do_quote(filepath, quote_mode)
char   *filepath;
char    quote_mode;
{
	FILE   *inf, *outf;
	char   *qfile, *quser;
	char    buf[256], *ptr;
	char    op;
	int     bflag;
	qfile = quote_file;
	quser = quote_user;
	bflag = strncmp(qfile, "mail", 4);
	outf = fopen(filepath, "w");
	if (*qfile != '\0' && (inf = fopen(qfile, "r")) != NULL) {
		op = quote_mode;
		if (op != 'N') {
			fgets(buf, 256, inf);
			if ((ptr = strrchr(buf, ')')) != NULL) {
				ptr[1] = '\0';
				if ((ptr = strchr(buf, ':')) != NULL) {
					quser = ptr + 1;
					while (*quser == ' ')
						quser++;
				}
			}
			if (bflag)
				fprintf(outf, "¡i ¦b %-.55s ªº¤j§@¤¤´£¨ì: ¡j\n", quser);
			else
				fprintf(outf, "¡i ¦b %-.55s ªº¨Ó«H¤¤´£¨ì: ¡j\n", quser);

			if (op == 'A') {
				while (fgets(buf, 256, inf) != NULL) {
					fprintf(outf, ": %s", buf);
				}
			} else if (op == 'R') {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;
				while (fgets(buf, 256, inf) != NULL) {
					if (Origin2(buf))
						continue;
					fprintf(outf, "%s", buf);
				}
			} else {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;
				while (fgets(buf, 256, inf) != NULL) {
					if (strcmp(buf, "--\n") == 0)
						break;
					if (buf[250] != '\0')
						strcpy(buf + 250, "\n");
					if (!garbage_line(buf))
						fprintf(outf, ": %s", buf);
				}
			}
		}
		fprintf(outf, "\n");
		fclose(inf);
	}
	*quote_file = '\0';
	*quote_user = '\0';

	if (!(currentuser.signature == 0 || header.chk_anony == 1)) {
		addsignature(outf, 1);
	}
	fclose(outf);
}
/* Add by SmallPig */
void
getcross(filepath, mode)
char   *filepath;
int     mode;
{
	FILE   *inf, *of;
	char    buf[256];
	char    owner[248];
	int     count;
	time_t  now;
	now = time(0);
	inf = fopen(quote_file, "r");
	of = fopen(filepath, "w");
	if (inf == NULL || of == NULL) {
		report("Cross Post error");
		return;
	}
	if (mode == 0) {
		if (in_mail == YEA) {
			in_mail = NA;
			write_header(of, 1 /* ¤£¼g¤J .posts */ );
			in_mail = YEA;
		} else
			write_header(of, 1 /* ¤£¼g¤J .posts */ );
		if (fgets(buf, 256, inf) != NULL)
			if (in_mail && strncmp(buf, "±H«H¤H: ", 8))
				strcpy(owner, currentuser.userid);
			else {
				for (count = 8; buf[count] != ' ' && buf[count] != '\n' && buf[count] != '\0'; count++)
					owner[count - 8] = buf[count];
				owner[count - 8] = '\0';
			}
		if (in_mail == YEA)
			fprintf(of, "[1;37m¡i ¥H¤U¤å¦rÂà¸ü¦Û [32m%s [37mªº«H½c ¡j\n", currentuser.userid);
		else
			fprintf(of, "[1;37m¡i ¥H¤U¤å¦rÂà¸ü¦Û [32m%s [37m°Q½×°Ï ¡j\n", quote_board);
		fprintf(of, "¡i ­ì¤å¥Ñ[32m %s[37m ©Òµoªí ¡j[m\n", owner);
		while (fgets(buf, 256, inf) != NULL)	/* Clear Post header */
			if (buf[0] == '\n')
				break;
	} else if (mode == 1) {
		fprintf(of, "µo«H¤H: deliver (¦Û°Êµo«H¨t²Î), «H°Ï: %s\n", quote_board);
		fprintf(of, "¼Ð  ÃD: %s\n", quote_title);
		fprintf(of, "µo«H¯¸: %s¦Û°Êµo«H¨t²Î (%24.24s)\n\n", BoardName, ctime(&now));
		fprintf(of, "¡i¦¹½g¤å³¹¬O¥Ñ¦Û°Êµo«H¨t²Î©Ò±i¶K¡j\n\n");
	} else if (mode == 2) {
		write_header(of, 0 /* ¼g¤J .posts */ );
	}
	while (fgets(buf, 256, inf) != NULL) {
		if ((strstr(buf, "¡i ¥H¤U¤å¦rÂà¸ü¦Û ") && strstr(buf, "°Q½×°Ï ¡j")) || (strstr(buf, "¡i ­ì¤å¥Ñ") && strstr(buf, "©Òµoªí ¡j")))
			continue;
		else
			fprintf(of, "%s", buf);
	}
	fclose(inf);
	fclose(of);
	*quote_file = '\0';
}


int
do_post()
{
	*quote_file = '\0';
	*quote_user = '\0';
	return post_article(currboard);
}
/*ARGSUSED*/
int
post_reply(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    uid[STRLEN];
	char    title[STRLEN];
	char   *t;
	FILE   *fp;
	if (!strcmp(currentuser.userid, "guest"))
		return DONOTHING;
	if (!HAS_PERM(PERM_LOGINOK)) {
		clear();
		move(3, 0);
		clrtobot();
		prints("\n\n              ±z©|¥¼³q¹L»{µý¡A©Ò¥HµLªk¨Ï¥Î±H«Hªº¥\\¯à¡I¡I  \n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	clear();
	if (check_maxmail()) {
		pressreturn();
		return FULLUPDATE;
	}
	modify_user_mode(SMAIL);

/* indicate the quote file/user */
	setbfile(quote_file, currboard, fileinfo->filename);
	strcpy(quote_user, fileinfo->owner);

/* find the author */
	if (!getuser(quote_user)) {
		genbuf[0] = '\0';
		fp = fopen(quote_file, "r");
		if (fp != NULL) {
			fgets(genbuf, 255, fp);
			fclose(fp);
		}
		t = strtok(genbuf, ":");
		if (strncmp(t, "µo«H¤H", 6) == 0 ||
			strncmp(t, "±H«H¤H", 6) == 0 ||
			strncmp(t, "Posted By", 9) == 0 ||
			strncmp(t, "§@  ªÌ", 6) == 0) {
			while (t != NULL) {
				t = (char *) strtok(NULL, " \r\t\n<>");
				if (t == NULL)
					break;
				if (!invalidaddr(t))
					break;
			}
			if (t != NULL)
				strncpy(uid, t, STRLEN);
		} else {
			prints("Error: Cannot find Author ... \n");
			pressreturn();
		}
	} else
		strcpy(uid, quote_user);
	/* make the title */
	if (toupper(fileinfo->title[0]) != 'R' || fileinfo->title[1] != 'e' ||
		fileinfo->title[2] != ':')
		strcpy(title, "Re: ");
	else
		title[0] = '\0';
	strncat(title, fileinfo->title, STRLEN - 5);

/* edit, then send the mail */
	switch (do_send(uid, title)) {
	case -1:
		prints("¨t²ÎµLªk°e«H\n");
		break;
	case -2:
		prints("°e«H°Ê§@¤w¸g¤¤¤î\n");
		break;
	case -3:
		prints("¨Ï¥ÎªÌ '%s' µLªk¦¬«H\n", uid);
		break;
	default:
		prints("«H¥ó¤w¦¨¥\\¦a±Hµ¹­ì§@ªÌ %s\n", uid);
	}
	pressreturn();
	return FULLUPDATE;
}
/* Add by SmallPig */
int
post_cross(islocal, mode)
char    islocal;
int     mode;
{
	struct fileheader postfile;
	struct shortfile *bp;
	char    filepath[STRLEN], fname[STRLEN];
	char    buf[256], buf4[STRLEN], whopost[IDLEN + 2];
	int     fp, count;
	time_t  now;
	if (!haspostperm(currboard) && !mode) {
		move(1, 0);
		prints("±z©|µLÅv­­¦b %s µoªí¤å³¹.\n", currboard);
		return -1;
	}
	memset(&postfile, 0, sizeof(postfile));
	strncpy(save_filename, fname, 4096);

	now = time(0);
	snprintf(fname, STRLEN, "M.%d.A", now);
	if (!mode) {
		if (!strstr(quote_title, "[Âà¸ü]"))
			snprintf(buf4, STRLEN, "[Âà¸ü] %.70s", quote_title);
		else
			strcpy(buf4, quote_title);
	} else
		strcpy(buf4, quote_title);
	strncpy(save_title, buf4, STRLEN);
	setbfile(filepath, currboard, fname);
	count = 0;

	while ((fp = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
		now++;
		snprintf(fname, STRLEN, "M.%d.A", now);
		setbfile(filepath, currboard, fname);
		if (count++ > MAX_POSTRETRY) {
			return -1;
		}
	}
	close(fp);
	strcpy(postfile.filename, fname);
	if (mode == 1)
		strcpy(whopost, "deliver");
	else
		strcpy(whopost, currentuser.userid);

	strncpy(postfile.owner, whopost, STRLEN);
	setbfile(filepath, currboard, postfile.filename);

	local_article = 0;
	if (islocal == 'l' || islocal == 'L')
		local_article = YEA;

	modify_user_mode(POSTING);

	getcross(filepath, mode);

	strncpy(postfile.title, save_title, STRLEN);
	bp = getbcache(currboard);
	if ((local_article) || !(bp->flag & OUT_FLAG)) {
		postfile.filename[STRLEN - 1] = 'L';
		postfile.filename[STRLEN - 2] = 'L';
	} else {
		postfile.filename[STRLEN - 1] = 'S';
		postfile.filename[STRLEN - 2] = 'S';
		outgo_post(&postfile, currboard);
	}
	setbdir(buf, currboard);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		if (!mode) {
			snprintf(buf, 256, "cross_posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		} else {
			snprintf(buf, 256, "Posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		}
		report(buf);
		pressreturn();
		clear();
		return 1;
	}
	/* brc_addlist( postfile.filename ) ; */
	if (!mode) {
		add_crossinfo(filepath, 1);
		snprintf(buf, 256, "cross_posted '%s' on '%s'", postfile.title, currboard);
		report(buf);
	}
/*    else
        snprintf(buf, 256, "¦Û°Êµoªí¨t²Î POST '%s' on '%s'", postfile.title, currboard) ;
    report(buf) ;*/
	return 1;
}


void 
add_loginfo(filepath)
char   *filepath;
{
	FILE   *fp;
	int     color, noidboard;
	char    fname[STRLEN];
	noidboard = header.chk_anony;
	color = (currentuser.numlogins % 7) + 31;
	setuserfile(fname, "signatures");
	if ((fp = fopen(filepath, "a")) == NULL)
		return;
	if (!dashf(fname) || currentuser.signature == 0 || noidboard)
		fputs("\n--", fp);
	fprintf(fp, "\n[m[1;%2dm¡° ¨Ó·½:¡E%s %s¡E[FROM: %-.20s][m\n"
		,color, BoardName, BBSHOST, (noidboard) ?
		"°Î¦W¤Ñ¨Ïªº®a" : fromhost);
	fclose(fp);
	return;
}

void 
add_crossinfo(filepath, mode)
char   *filepath;
int     mode;
{
	FILE   *fp;
	int     color;
	color = (currentuser.numlogins % 7) + 31;
	if ((fp = fopen(filepath, "a")) == NULL)
		return;
	fprintf(fp, "--\n[m[1;%2dm¡° Âà%s:¡E%s %s¡E[FROM: %-.20s][m\n"
		,color, (mode == 1) ? "¸ü" : "±H", BoardName, BBSHOST, fromhost);
	fclose(fp);
	return;
}

int
show_board_notes(bname, command)
char    bname[30];
{
	char    buf[256];
	snprintf(buf, 256, "vote/%s/notes", bname);
	if (dashf(buf)) {
		if (command == 1) {
			ansimore2(buf, NA, 0, 19);
			return 1;
		} else {
			ansimore(buf);
			return 1;
		}
	} else if (dashf("vote/notes")) {
		if (command == 1) {
			ansimore2("vote/notes", NA, 0, 19);
		} else {
			ansimore("vote/notes", NA, 0, 19);
		}
		return 1;
	}
	return -1;
}

int
show_user_notes()
{
	char    buf[256];
	setuserfile(buf, "notes");
	if (dashf(buf)) {
		ansimore(buf);
		return FULLUPDATE;
	}
	clear();
	move(10, 15);
	prints("±z©|¥¼¦b InfoEdit->WriteFile ½s¿è­Ó¤H³Æ§Ñ¿ý¡C\n");
	pressanykey();
	return FULLUPDATE;
}


int
outgo_post(fh, board)
struct fileheader *fh;
char   *board;
{
	char    buf[256];
	snprintf(buf, 256, "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, header.chk_anony ? board : currentuser.userid,
		header.chk_anony ? "§Ú¬O°Î¦W¤Ñ¨Ï" : currentuser.username, save_title);
	file_append("innd/out.bntp", buf);

}

int
post_article(postboard)
char   *postboard;
{
	struct fileheader postfile;
	struct shortfile *bp;
	char    filepath[STRLEN], fname[STRLEN], buf[STRLEN];
	int     fp, aborted, count;
	time_t  now;
	modify_user_mode(POSTING);
	if (!haspostperm(postboard)) {
		move(3, 0);
		clrtobot();
		if (digestmode == NA) {
			prints("\n\n        ¦¹°Q½×°Ï¬O°ßÅªªº, ©Î¬O±z©|µLÅv­­¦b¦¹µoªí¤å³¹¡C");
		} else {
			prints("\n\n     ¥Ø«e¬O¤åºK©Î¥DÃD¼Ò¦¡, ©Ò¥H¤£¯àµoªí¤å³¹ («ö¥ªÁä¥iÂ÷¶}¦¹¼Ò¦¡)¡C");
		}
		pressreturn();
		clear();
		return FULLUPDATE;
	} else if (deny_me(postboard) && !HAS_PERM(PERM_SYSOP)) {
		move(3, 0);
		clrtobot();
		prints("\n\n                 «Ü©êºp¡A§A³QªO¥D°±¤î POST ªºÅv¤O¡C");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	memset(&postfile, 0, sizeof(postfile));
	clear();
	show_board_notes(postboard, 1);
#ifndef NOREPLY
	if (replytitle[0] != '\0') {
		if (ci_strncmp(replytitle, "Re:", 3) == 0)
			strcpy(header.title, replytitle);
		else
			snprintf(header.title, STRLEN, "Re: %s", replytitle);
		header.reply_mode = 1;
	} else
#endif
	{
		header.title[0] = '\0';
		header.reply_mode = 0;

	}
	strcpy(header.ds, postboard);
	header.postboard = YEA;
	if (post_header(&header)) {
		strcpy(postfile.title, header.title);
		strncpy(save_title, postfile.title, STRLEN);
		strncpy(save_filename, fname, 4096);
	} else
		return FULLUPDATE;
	now = time(0);
	snprintf(fname, STRLEN, "M.%d.A", now);
	setbfile(filepath, postboard, fname);
	count = 0;
	while ((fp = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
		now++;
		snprintf(fname, STRLEN, "M.%d.A", now);
		setbfile(filepath, postboard, fname);
		if (count++ > MAX_POSTRETRY) {
			return -1;
		}
	}
	fchmod(fp, 0644);	/* youzi 1999.1.8 */
	close(fp);
	strcpy(postfile.filename, fname);
	in_mail = NA;
	strncpy(postfile.owner, (header.chk_anony) ?
		postboard : currentuser.userid, STRLEN);
	setbfile(filepath, postboard, postfile.filename);
	local_article = 0;
	if (!strcmp(postfile.title, buf) && quote_file[0] != '\0')
		if (quote_file[255] == 'L')
			local_article = 1;
	modify_user_mode(POSTING);
	do_quote(filepath, header.include_mode);
	aborted = vedit(filepath, YEA);
	add_loginfo(filepath);
	strncpy(postfile.title, save_title, STRLEN);
	bp = getbcache(postboard);
	if ((aborted == 1) || !(bp->flag & OUT_FLAG)) {
		postfile.filename[STRLEN - 1] = 'L';
		postfile.filename[STRLEN - 2] = 'L';
	} else {
		postfile.filename[STRLEN - 1] = 'S';
		postfile.filename[STRLEN - 2] = 'S';
		outgo_post(&postfile, postboard);
	}
	/* Anony=0; *//* Inital For ShowOut Signature */
	if (aborted == -1) {
		unlink(filepath);
		clear();
/*        pressreturn() ;*/
		return FULLUPDATE;
	}
	setbdir(buf, postboard);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		snprintf(buf, STRLEN, "posting '%s' on '%s': append_record failed!",
			postfile.title, currboard);
		report(buf);
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	brc_addlist(postfile.filename);

	snprintf(buf, STRLEN, "posted '%s' on '%s'", postfile.title, currboard);
	report(buf);
	if (!junkboard()) {
		set_safe_record();
		currentuser.numposts++;
	}
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);

	return FULLUPDATE;
}


int
add_edit_mark(fname, mode, title)
char   *fname;
int     mode;
char   *title;
{
	FILE   *fp, *out;
	char    buf[256];
	time_t  now;
	char    outname[STRLEN];
	int     step = 0, signature = 0;
	if ((fp = fopen(fname, "r")) == NULL)
		return 0;
	snprintf(outname, STRLEN, "tmp/editpost.%s.%05d", currentuser.userid, uinfo.pid);
	if ((out = fopen(outname, "w")) == NULL)
		return 0;

	while ((fgets(buf, 256, fp)) != NULL) {
		if (!strncmp(buf, "--", 2))
			signature = 1;
		if (mode == 1) {
			if (!strncmp(buf, "[1;36m¡° ­×§ï:¡E", 17))
				continue;
			if (Origin2(buf) && (step != 3)) {
				now = time(0);
				fprintf(out, "%s[1;36m¡° ­×§ï:¡E%s ©ó %15.15s ­×§ï¥»¤å¡E[FROM: %-.20s][m\n",
					(signature) ? "" : "--\n", currentuser.userid, ctime(&now) + 4, fromhost);
				step = 3;
			}
			fputs(buf, out);
		} else {
			if (!strncmp(buf, "¼Ð  ÃD: ", 8)) {
				fprintf(out, "¼Ð  ÃD: %s\n", title);
				continue;
			}
			fputs(buf, out);
		}
	}
	if ((step != 3) && (mode == 1)) {
		now = time(0);
		fprintf(out, "%s[1;36m¡° ­×§ï:¡E%s ©ó %15.15s ­×§ï¥»¤å¡E[FROM: %-.20s][m\n",
			(signature) ? "" : "--\n", currentuser.userid, ctime(&now) + 4, fromhost);
	}
	fclose(fp);
	fclose(out);
	rename(outname, fname);
/*	youzi 1999.1.10 */
	chmod(fname, 0644);
	return;
}
/*ARGSUSED*/
int
edit_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[512];
	char   *t;
	extern char currmaildir[STRLEN];
	if (!in_mail) {
		if (!chk_currBM(currBM)) {
			struct shortfile *bp;
			/* edwardc.010807 guest ¤£¯à§ï¤å³¹, ´Nºâ¬O¦Û¤vµoªº ... */
			if (strcmp(fileinfo->owner, currentuser.userid) || !strcmp(currentuser.userid, "guest") )
				return DONOTHING;
			bp = getbcache(currboard);
			if ((bp->flag & ANONY_FLAG) && !strcmp(fileinfo->owner, currboard))
				return DONOTHING;
		}
	}
	modify_user_mode(EDIT);
	clear();
	if (in_mail)
		strcpy(buf, currmaildir);
	else
		strcpy(buf, direct);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, 1024, "%s/%s", buf, fileinfo->filename);
	if (vedit(genbuf, NA) != -1) {
		if (ADD_EDITMARK)
			add_edit_mark(genbuf, 1, NULL);
	}
	if (!in_mail) {
		snprintf(genbuf, 1024, "edited post '%s' on %s", fileinfo->title, currboard);
		report(genbuf);
	}
	return FULLUPDATE;
}

int
edit_title(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[STRLEN];
	if (!chk_currBM(currBM)) {
		struct shortfile *bp;
		if (strcmp(fileinfo->owner, currentuser.userid))
			return DONOTHING;
		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !strcmp(fileinfo->owner, currboard))
			return DONOTHING;
	}
	getdata(t_lines - 1, 0, "·s¤å³¹¼ÐÃD: ", buf, 50, DOECHO, YEA);
	if (buf[0] != '\0') {
		char    tmp[STRLEN * 2], *t;
		strcpy(fileinfo->title, buf);
		strcpy(tmp, direct);
		if ((t = strrchr(tmp, '/')) != NULL)
			*t = '\0';
		snprintf(genbuf, 1024, "%s/%s", tmp, fileinfo->filename);
		add_edit_mark(genbuf, 2, buf);
		substitute_record(direct, fileinfo, sizeof(*fileinfo), ent);
	}
	return PARTUPDATE;
}

int
mark_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (!chk_currBM(currBM)) {
		return DONOTHING;
	}
	if (fileinfo->accessed[0] & FILE_MARKED)
		fileinfo->accessed[0] &= ~FILE_MARKED;
	else
		fileinfo->accessed[0] |= FILE_MARKED;
	substitute_record(direct, fileinfo, sizeof(*fileinfo), ent);
	return PARTUPDATE;
}

int
del_range(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    num[8];
	int     inum1, inum2;
	if (uinfo.mode == READING)
		if (!chk_currBM(currBM)) {
			return DONOTHING;
		}
	if (digestmode == 2)
		return DONOTHING;
	getdata(t_lines - 1, 0, "­º½g¤å³¹½s¸¹: ", num, 6, DOECHO, YEA);
	inum1 = atoi(num);
	if (inum1 <= 0) {
		move(t_lines - 1, 50);
		prints("¿ù»~½s¸¹...");
		egetch();
		return PARTUPDATE;
	}
	getdata(t_lines - 1, 25, "¥½½g¤å³¹½s¸¹: ", num, 6, DOECHO, YEA);
	inum2 = atoi(num);
	if (inum2 - inum1 <= 1) {
		move(t_lines - 1, 50);
		prints("¿ù»~°Ï¶¡...");
		egetch();
		return PARTUPDATE;
	}
	move(t_lines - 1, 50);
	if (askyn("½T©w§R°£", NA, NA) == YEA) {
		delete_range(direct, inum1, inum2);
		fixkeep(direct, inum1, inum2);
		if (uinfo.mode == READING) {
			snprintf(genbuf, 1024, "Range delete %d-%d on %s", inum1, inum2, currboard);
		} else {
			snprintf(genbuf, 1024, "Range delete %d-%d in mailbox", inum1, inum2);
		}
		report(genbuf);
		return DIRCHANGED;
	}
	move(t_lines - 1, 50);
	clrtoeol();
	prints("©ñ±ó§R°£...");
	egetch();
	return PARTUPDATE;
}

int
del_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	FILE   *fn;
	char    buf[512];
	char    usrid[STRLEN];
	char   *t;
	int     owned, keep, fail;
	extern int SR_BMDELFLAG;
	if (digestmode == 2 || !strcmp(currboard, "deleted") || !strcmp(currboard, "junk"))
		return DONOTHING;
	if (fileinfo->owner[0] == '-')
		return DONOTHING;
	keep = sysconf_eval("KEEP_DELETED_HEADER");
	/* edwardc.010807 guest ¤£¯à¬å¤å³¹, ´Nºâ¬O¦Û¤vµoªº ... */
	owned = ( !strcmp(fileinfo->owner, currentuser.userid) && strcmp(currentuser.userid, "guest") );
	strcpy(usrid, fileinfo->owner);
	if (!chk_currBM(currBM)) {
		struct shortfile *bp;
		if (!(owned))
			return DONOTHING;
		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !strcmp(usrid, currboard))
			return DONOTHING;
	}
	if (!SR_BMDELFLAG) {
		snprintf(genbuf, 1024, "§R°£¤å³¹ [%-.55s]", fileinfo->title);
		if (askyn(genbuf, NA, YEA) == NA) {
			move(t_lines - 1, 0);
			prints("©ñ±ó§R°£¤å³¹...");
			clrtoeol();
			egetch();
			return PARTUPDATE;
		}
	}
	strcpy(buf, direct);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, 1024, "Del '%s' on '%s'", fileinfo->title, currboard);
	report(genbuf);
	strncpy(currfile, fileinfo->filename, STRLEN);

	/* edwardc.991031 ­×¥¿¬å¤åºK¨Ã¤£·|§â .DIGEST ¤¤ªº entry µ¹¬å±¼ */
	if ( fileinfo->accessed[0] & FILE_DIGEST ) {
		fileinfo->accessed[0] &= ~FILE_DIGEST;
	 	dele_digest(currfile, direct);
	}
			
	if (keep <= 0) {
		fail = delete_file(direct, sizeof(struct fileheader), ent, cmpfilename);
	} else {
		fail = update_file(direct, sizeof(struct fileheader), ent, cmpfilename,
			cpyfilename);
	}
	if (!fail) {
		cancelpost(currboard, currentuser.userid, fileinfo, owned);
		snprintf(genbuf, 1024, "%s/%s", buf, fileinfo->filename);
		if (keep <= 0) {
			unlink(genbuf);
		} else if ((fn = fopen(genbuf, "w")) != NULL) {
			fprintf(fn, "\n\tArticle canceled by %s.\n", currentuser.userid);
			fclose(fn);
		}
		if (owned) {
			set_safe_record();
			if (currentuser.numposts > 0 && !junkboard())
				currentuser.numposts--;
			substitute_record(PASSFILE, &currentuser,
				sizeof(currentuser), usernum);
		} else if (!strstr(usrid, ".") && BMDEL_DECREASE) {
			int     id;
			if (id = getuser(usrid))
				if (!junkboard()) {
					if (lookupuser.numposts > 0)
						lookupuser.numposts--;
					substitute_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
				}
		}
		return DIRCHANGED;
	}
	move(t_lines - 1, 0);
	prints("§R°£¥¢±Ñ...");
	clrtoeol();
	egetch();
	return PARTUPDATE;
}

static int sequent_ent;

int
sequent_messages(fptr)
struct fileheader *fptr;
{
	static int idc;
	if (fptr == NULL) {
		idc = 0;
		return 0;
	}
	idc++;
	if (readpost) {
		if (idc < sequent_ent)
			return 0;
		if (!brc_unread(fptr->filename))
			return 0;
		mot = 1;
		if (continue_flag != 0) {
			genbuf[0] = 'y';
		} else {
			prints("°Q½×°Ï: '%s' ¼ÐÃD:\n\"%s\" posted by %s.\n",
				currboard, fptr->title, fptr->owner);
			getdata(3, 0, "Åª¨ú (Y/N/Quit) [Y]: ", genbuf, 5, DOECHO, YEA);
		}
		if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
			if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
				clear();
				return QUIT;
			}
			clear();
			return 0;
		}
		setbfile(genbuf, currboard, fptr->filename);
		strncpy(quote_file, genbuf, sizeof(quote_file));
		quote_file[sizeof(quote_file) - 1] = '\0';
		strcpy(quote_user, fptr->owner);
#ifdef NOREPLY
		more(genbuf, YEA);
#else
		ansimore(genbuf, NA);
		move(t_lines - 1, 0);
		clrtoeol();
		prints("\033[1;44;31m[³sÄòÅª«H]  \033[33m¦^«H R ¢x µ²§ô Q,¡ö ¢x¤U¤@«Ê ' ',¡õ ¢x^R ¦^«Hµ¹§@ªÌ                \033[m");
		continue_flag = 0;
		switch (egetch()) {
		case 'N':
		case 'Q':
		case 'n':
		case 'q':
		case KEY_LEFT:
			break;
		case 'Y':
		case 'R':
		case 'y':
		case 'r':
			do_reply(fptr->title);
		case ' ':
		case '\n':
		case KEY_DOWN:
			continue_flag = 1;
			break;
		case Ctrl('R'):
			post_reply(0, fptr, (char *) NULL);
			break;

		default:
			break;
		}
#endif
		clear();
	}
	setbdir(genbuf, currboard);
	brc_addlist(fptr->filename);
	return 0;
}

int
clear_new_flag(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	readpost = 0;
	sequential_read2(ent);
	return PARTUPDATE;
}
int
sequential_read(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	readpost = 1;
	clear();
	return sequential_read2(ent);
}
/*ARGSUSED*/
int
sequential_read2(ent /* ,fileinfo,direct */ )
int     ent;
/*struct fileheader *fileinfo ;
char *direct ;*/
{
	char    buf[STRLEN];
	sequent_messages((struct fileheader *) NULL);
	sequent_ent = ent;
	quiting = NA;
	continue_flag = 0;
	setbdir(buf, currboard);
	apply_record(buf, sequent_messages, sizeof(struct fileheader));
	return FULLUPDATE;
}
/* Added by netty to handle post saving into (0)Announce */
int
Save_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (!chk_currBM(currBM))
		return DONOTHING;
	return (a_Save("0Announce", currboard, fileinfo, NA));
}
/* Added by netty to handle post saving into (0)Announce */
int
Import_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (!chk_currBM(currBM))
		return DONOTHING;
	return (a_Import("0Announce", currboard, fileinfo, NA));
}

int
check_notespasswd()
{
	FILE   *pass;
	char    passbuf[20], prepass[STRLEN];
	char    buf[STRLEN];
	setvfile(buf, currboard, "notespasswd");
	if ((pass = fopen(buf, "r")) != NULL) {
		fgets(prepass, STRLEN, pass);
		fclose(pass);
		prepass[strlen(prepass) - 1] = '\0';
		getdata(2, 0, "½Ð¿é¤J¯µ±K³Æ§Ñ¿ý±K½X: ", passbuf, 19, NOECHO, YEA);
		if (passbuf[0] == '\0' || passbuf[0] == '\n')
			return NA;
		if (!checkpasswd(prepass, passbuf)) {
			move(3, 0);
			prints("¿ù»~ªº¯µ±K³Æ§Ñ¿ý±K½X...");
			pressanykey();
			return NA;
		}
	}
	return YEA;
}

int
show_b_secnote()
{
	char    buf[256];
	clear();
	setvfile(buf, currboard, "secnotes");
	if (dashf(buf)) {
		if (!check_notespasswd())
			return FULLUPDATE;
		clear();
		ansimore(buf, NA);
	} else {
		move(3, 25);
		prints("¦¹°Q½×°Ï©|µL¡u¯µ±K³Æ§Ñ¿ý¡v¡C");
	}
	pressanykey();
	return FULLUPDATE;
}

int
show_b_note()
{
	clear();
	if (show_board_notes(currboard, 2) == -1) {
		move(3, 30);
		prints("¦¹°Q½×°Ï©|µL¡u³Æ§Ñ¿ý¡v¡C");
	}
	return FULLUPDATE;
}

int
into_announce()
{
	if (a_menusearch("0Announce", currboard, (HAS_PERM(PERM_ANNOUNCE) ||
				HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_OBOARDS)) ? PERM_BOARDS : 0))
		return FULLUPDATE;
	return DONOTHING;
}
#ifdef INTERNET_EMAIL
int
forward_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (strcmp("guest", currentuser.userid) == 0)
		return DONOTHING;
	return (mail_forward(ent, fileinfo, direct));
}

int
forward_u_post(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (strcmp("guest", currentuser.userid) == 0)
		return DONOTHING;
	return (mail_u_forward(ent, fileinfo, direct));
}
#endif

extern int 
mainreadhelp();
extern int b_vote();
extern int b_results();
extern int b_vote_maintain();
extern int b_notes_edit();
extern int show_allmsgs();

struct one_key read_comms[] = {
	'r', read_post,
	'K', skip_post,
	'u', skip_post,
	'd', del_post,
	'D', del_range,
	'm', mark_post,
	'E', edit_post,
	Ctrl('G'), digest_mode,
	'`', digest_mode,
	'g', digest_post,
	'L', show_allmsgs,
	'T', edit_title,
	's', do_select,
	Ctrl('C'), do_cross,
	Ctrl('P'), do_post,
	'c', clear_new_flag,	/* youzi */
	'o', sequential_read,
#ifdef INTERNET_EMAIL
	'F', forward_post,
	'U', forward_u_post,
	Ctrl('R'), post_reply,
#endif
	'i', Save_post,
	'I', Import_post,
	'R', b_results,
	'v', b_vote,
	'V', b_vote,
	'M', b_vote_maintain,
	'W', b_notes_edit,
	Ctrl('W'), b_notes_passwd,
	'h', mainreadhelp,
	Ctrl('J'), mainreadhelp,
	KEY_TAB, show_b_note,
	'z', show_b_secnote,
	'x', into_announce,
	'a', auth_search_down,
	'A', auth_search_up,
	'/', t_search_down,
	'?', t_search_up,
	'\'', post_search_down,
	'\"', post_search_up,
	']', thread_down,
	'[', thread_up,
	Ctrl('D'), deny_user,
	Ctrl('A'), show_author,
	Ctrl('N'), SR_first_new,
	'n', SR_first_new,
	'\\', SR_last,
	'=', SR_first,
	Ctrl('S'), SR_read,
	'p', SR_read,
	Ctrl('X'), SR_read,
	Ctrl('U'), SR_author,
	'b', SR_BMfunc,
	'B', SR_BMfunc,
	Ctrl('T'), thread_mode,
	't', thesis_mode,	/* youzi 1997.7.8 */
	'!', Q_Goodbye,
	'S', s_msg,
	'f', t_friends,
	'\0', NULL
};

int
Read()
{
	char    buf[STRLEN];
	char    notename[STRLEN];
	time_t  usetime;
	struct stat st;
	if (!selboard) {
		move(2, 0);
		prints("½Ð¥ý¿ï¾Ü°Q½×°Ï\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	}
	in_mail = NA;
	brc_initial(currboard);
	setbdir(buf, currboard);

	setvfile(notename, currboard, "notes");
	if (stat(notename, &st) != -1) {
		if (st.st_mtime < (time(NULL) - 7 * 86400)) {
			utimes(notename, NULL);
			setvfile(genbuf, currboard, "noterec");
			unlink(genbuf);
		}
	}
#ifdef ALWAYS_SHOW_BRDNOTE
	if (dashf(notename))
		ansimore3(notename, YEA);
#else
	if (vote_flag(currboard, '\0', 1 /* ÀË¬dÅª¹L·sªº³Æ§Ñ¿ý¨S */ ) == 0) {
		if (dashf(notename)) {
			ansimore3(notename, YEA);
			vote_flag(currboard, 'R', 1 /* ¼g¤JÅª¹L·sªº³Æ§Ñ¿ý */ );
		}
	}
#endif

	usetime = time(0);
	i_read(READING, buf, readtitle, readdoent, &read_comms[0], sizeof(struct fileheader));
	board_usage(currboard, time(0) - usetime);

	brc_update();
	return 0;
}
/*Add by SmallPig*/
void
notepad()
{
	char    tmpname[STRLEN], note1[4];
	char    note[3][STRLEN - 4];
	char    tmp[STRLEN];
	FILE   *in;
	int     i, n;
	time_t  thetime = time(0);
	extern int talkrequest;


	clear();
	move(0, 0);
	prints("¶}©l§Aªº¯d¨¥§a¡I¤j®a¥¿«ø¥Ø¥H«Ý....\n");
	modify_user_mode(WNOTEPAD);
	snprintf(tmpname, STRLEN, "tmp/notepad.%s.%05d", currentuser.userid, uinfo.pid);
	if ((in = fopen(tmpname, "w")) != NULL) {
		for (i = 0; i < 3; i++)
			memset(note[i], 0, STRLEN - 4);
		while (1) {
			for (i = 0; i < 3; i++) {
				getdata(1 + i, 0, ": ", note[i], STRLEN - 5, DOECHO, NA);
				if (note[i][0] == '\0')
					break;
			}
			if (i == 0) {
				fclose(in);
				unlink(tmpname);
				return;
			}
			getdata(5, 0, "¬O§_§â§Aªº¤j§@©ñ¤J¯d¨¥ªO (Y)¬Oªº (N)¤£­n (E)¦A½s¿è [Y]: ", note1, 3, DOECHO, YEA);
			if (note1[0] == 'e' || note1[0] == 'E')
				continue;
			else
				break;
		}
		if (note1[0] != 'N' && note1[0] != 'n') {
			snprintf(tmp, STRLEN, "[1;32m%s[37m¡]%.18s¡^", currentuser.userid, currentuser.username);
			fprintf(in, "[1;34m¢y[44m¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v[36m»Ä[32m²¢[33m­W[31m»¶[37mªO[34m¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v¢v[44m¢©[m\n");
			fprintf(in, "[1;34m¢y[32;40m %-46s[32m¦b [36m%.21s[32m Â÷¶}®É¯d¤Uªº¸Ü  [m\n", tmp, Ctime(&thetime));
			for (n = 0; n < i; n++) {
				if (note[n][0] == '\0')
					break;
				fprintf(in, "[1;34m¢y[33;40m %-75.75s[1;34m[m \n", note[n]);
			}
			fprintf(in, "[1;34m¢y[44m ¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w [m \n");
			catnotepad(in, "etc/notepad");
			fclose(in);
			rename(tmpname, "etc/notepad");
		} else {
			fclose(in);
			unlink(tmpname);
		}
	}
	if (talkrequest) {
		talkreply();
	}
	clear();
	return;
}
/* youzi quick goodbye */
int
Q_Goodbye()
{
	extern int started;
	time_t  stay;
	char    fname[STRLEN];
	int     logouts;
	setuserfile(fname, "msgfile");

#ifdef LOG_MY_MESG
	unlink(fname);
	setuserfile(fname, "msgfile.me");
#endif

	/* edwardc.990423 °T®§ÂsÄý¾¹ */
	if (dashf(fname)) {
		clear();
		mesgmore(fname, NA, 0, 9999);
	}
	clear();
	prints("\n\n\n\n");

	setuserfile(fname, "notes");
	if (dashf(fname))
		ansimore(fname, YEA);

	setuserfile(fname, "logout");
	if (dashf(fname)) {
		logouts = countlogouts(fname);
		if (logouts >= 1) {
			user_display(fname,
				(logouts == 1) ? 1 : (currentuser.numlogins % (logouts)) + 1, YEA);
		}
	} else {
		if (fill_shmfile(2, "etc/logout", "GOODBYE_SHMKEY"))
			show_goodbyeshm();
	}

	report("exit");

	stay = time(NULL) - login_start_time;
	if (started) {
		snprintf(genbuf, 1024, "Stay:%3ld (%s)", stay / 60, currentuser.username);
		log_usies("EXIT ", genbuf);
		u_exit();
	}
	set_safe_record();
	currentuser.stay += stay;
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	pressreturn();
	if (num_user_logins(currentuser.userid) == 0 &&
		strcmp(currentuser.userid, "guest")) {
		FILE   *fp;
		char    buf[STRLEN], *ptr;
		if ((fp = fopen("friendbook", "r")) != NULL) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				char    uid[14];
				ptr = strstr(buf, "@");
				if (ptr == NULL) {
					del_from_file("friendbook", buf);
					continue;
				}
				ptr++;
				strcpy(uid, ptr);
				ptr = strstr(uid, "\n");
				*ptr = '\0';
				if (!strcmp(uid, currentuser.userid))
					del_from_file("friendbook", buf);
			}
			fclose(fp);
		}
	}
	shm_deatch();	/* deatch shm */
	sleep(1);
#ifndef BBSD	
	reset_tty();
#endif	
	exit(0);
	return -1;
}

int
Goodbye()
{
	char    sysoplist[20][STRLEN], syswork[20][STRLEN], spbuf[STRLEN],
	        buf[STRLEN];
	int     i, num_sysop, choose;
	FILE   *sysops;
	*quote_file = '\0';
	i = 0;
	if ((sysops = fopen("etc/sysops", "r")) != NULL) {
		while (fgets(buf, STRLEN, sysops) != NULL && i <= 19) {
			strncpy(sysoplist[i], (char *) strtok(buf, " \n\r\t"), STRLEN - 1);
			strncpy(syswork[i], (char *) strtok(NULL, " \n\r\t"), STRLEN - 1);
			i++;
		}
		fclose(sysops);
	}
	num_sysop = i;
	move(1, 0);
	alarm(0);
	clear();
	move(0, 0);
	prints("§A´N­nÂ÷¶} %s ¡A¥i¦³¤°»ò«ØÄ³¶Ü¡H\n", BoardName);
	prints("[[1;33m1[m] ±H«Hµ¹ºÞ²z¤H­û\n");
	prints("[[1;33m2[m] «ö¿ù¤F°Õ¡A§ÚÁÙ­nª±\n");
	if (strcmp(currentuser.userid, "guest") != 0) {
		if (USE_NOTEPAD == 1)
			prints("[[1;33m3[m] ¼g¼g[1;32m¯d[33m¨¥[35mªO[mÅo\n");
	}
	prints("[[1;33m4[m] ¤£±HÅo¡A­nÂ÷¶}°Õ\n");
	snprintf(spbuf, STRLEN, "§Aªº¿ï¾Ü¬O [[1;32m4[m]¡G");
	getdata(7, 0, spbuf, genbuf, 4, DOECHO, YEA);
	clear();
	choose = genbuf[0] - '0';
	if (choose == 1) {
		prints("     USER ID      ­t ³d ªº Â¾ °È\n");
		prints("     ============ =====================\n");
		for (i = 1; i <= num_sysop; i++) {
			prints("[[1;33m%2d[m] %-12s %s\n", i, sysoplist[i - 1]
				,syswork[i - 1]);
		}
		prints("[[1;33m%2d[m] ÁÙ¬O¨«¤FÅo¡I\n", num_sysop + 1);
		snprintf(spbuf, STRLEN, "§Aªº¿ï¾Ü¬O [[1;32m%d[m]¡G", num_sysop + 1);
		getdata(num_sysop + 5, 0, spbuf, genbuf, 4, DOECHO, YEA);
		choose = atoi(genbuf);
		if (choose >= 1 && choose <= num_sysop)
			do_send(sysoplist[choose - 1], "¨Ï¥ÎªÌ±H¨Óªº«ØÄ³«H");
		choose = -1;
	}
	if (choose == 2)
		return FULLUPDATE;
	if (strcmp(currentuser.userid, "guest") != 0) {
		if (choose == 3)
			if (USE_NOTEPAD == 1 && HAS_PERM(PERM_POST))
				notepad();
	}
	return Q_Goodbye();

}

void
report(s)
char   *s;
{
	char    buf[512];
	char    timestr[16], *thetime;
	time_t  dtime;
	time(&dtime);
	thetime = ctime(&dtime);
	strncpy(timestr, &(thetime[4]), 15);
	timestr[15] = '\0';
	snprintf(buf, 512, "%s %s %s\n", currentuser.userid, timestr, s);
	file_append("trace", buf);
}

void
board_usage(mode, usetime)
char   *mode;
time_t  usetime;
{
	time_t  now;
	char    buf[256];
	now = time(0);
	snprintf(buf, 256, "%s USE %-20.20s Stay: %5ld (%s)\n", Ctime(&now), mode, usetime, currentuser.userid);
	file_append("log/use_board", buf);
}


int
Info()
{
	modify_user_mode(XMENU);
	ansimore("Version.Info", YEA);
	clear();
	return 0;
}

int
Conditions()
{
	modify_user_mode(XMENU);
#ifdef Use_GNU_License
	/* edwardc.000706 We're no longer shipped this software with BSD License */
	/* please view include/config.h for details .. */
	ansimore("COPYING", YEA);		
#else
	ansimore("COPYRIGHT", YEA);
#endif
	clear();
	return 0;
}

int
Welcome()
{
	char    ans[3];
	modify_user_mode(XMENU);
	if (!dashf("Welcome2"))
		ansimore("Welcome", YEA);
	else {
		clear();
		stand_title("Æ[¬Ý¶i¯¸µe­±");
		for (;;) {
			getdata(1, 0, "(1)¯S®í¶i¯¸¤½§GÄæ  (2)¥»¯¸¶i¯¸µe­± ? : ",
				ans, 2, DOECHO, YEA);

/* skyo.990427 modify  «ö Enter ¸õ¥X  */
			if (ans[0] == '\0') {
				clear();
				return 0;
			}
			if (ans[0] == '1' || ans[0] == '2')
				break;
		}
		if (ans[0] == '1')
			ansimore("Welcome", YEA);
		else
			ansimore("Welcome2", YEA);
	}
	clear();
	return 0;
}

int
cmpbnames(bname, brec)
char   *bname;
struct fileheader *brec;
{
	if (!ci_strncmp(bname, brec->filename, sizeof(brec->filename)))
		return 1;
	else
		return 0;
}

void
cancelpost(board, userid, fh, owned)
char   *board, *userid;
struct fileheader *fh;
int     owned;
{
	struct fileheader postfile;
	FILE   *fin, *fout;
	char    from[STRLEN], path[STRLEN];
	char    fname[STRLEN], *ptr, *brd;
	int     len, count;
	time_t  now;
	setbfile(genbuf, board, fh->filename);
	if ((fin = fopen(genbuf, "r")) != NULL) {
		brd = (owned) ? "junk" : "deleted";
		now = time(0);
		snprintf(fname, STRLEN, "M.%d.A", now);
		setbfile(genbuf, brd, fname);
		count = 0;
		while (dashf(genbuf)) {
			now++;
			snprintf(fname, STRLEN, "M.%d.A", now);
			setbfile(genbuf, brd, fname);
			if (count++ > MAX_POSTRETRY) {
				return;
			}
		}
		if ((fout = fopen(genbuf, "w")) != NULL) {
			memset(&postfile, 0, sizeof(postfile));
			snprintf(genbuf, 1024, "%-32.32s - %s", fh->title, userid);
			strcpy(postfile.filename, fname);
			strncpy(postfile.owner, fh->owner, IDLEN + 2);
			strncpy(postfile.title, genbuf, STRLEN);
			postfile.filename[STRLEN - 1] = 'D';
			postfile.filename[STRLEN - 2] = 'D';
		}
		from[0] = '\0';
		while (fgets(genbuf, sizeof(genbuf), fin) != NULL) {
			if (fout != NULL) {
				fputs(genbuf, fout);
			}
			len = strlen(genbuf) - 1;
			genbuf[len] = '\0';
			if (len <= 8) {
				break;
			} else if (strncmp(genbuf, "µo«H¤H: ", 8) == 0) {
				if ((ptr = strrchr(genbuf, ')')) != NULL) {
					*ptr = '\0';
					if ((ptr = strrchr(genbuf, '(')) != NULL)
						strcpy(from, ptr + 1);
				}
			} else if (strncmp(genbuf, "Âà«H¯¸: ", 8) == 0) {
				strncpy(path, genbuf + 8, sizeof(path));
				path[sizeof(path) - 1] = '\0';
			}
		}
		if (fout != NULL) {
			while (fgets(genbuf, sizeof(genbuf), fin) != NULL)
				fputs(genbuf, fout);
		}
		fclose(fin);
		if (fout != NULL) {
			fclose(fout);
			setbdir(genbuf, brd);
			append_record(genbuf, &postfile, sizeof(postfile));
		}
		if (strrchr(fh->owner, '.'))
			return;
		if ((fh->filename[STRLEN - 1] == 'S') && (fh->filename[STRLEN - 2] == 'S')
			&& (atoi(fh->filename + 2) > now - 14 * 86400)) {
			snprintf(genbuf, 1024, "%s\t%s\t%s\t%s\t%s\n",
				board, fh->filename, fh->owner, from, fh->title);
			if ((fin = fopen("innd/cancel.bntp", "a")) != NULL) {
				fputs(genbuf, fin);
				fclose(fin);
			}
		}
	}
}

int 
thesis_mode()
{
	int     id, i;
	unsigned int pbits;
	i = 'W' - 'A';
	id = getuser(currentuser.userid);
	pbits = lookupuser.userdefine;
	pbits ^= (1 << i);
	lookupuser.userdefine = pbits;
	currentuser.userdefine = pbits;
	substitute_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
	update_utmp();
	return FULLUPDATE;
}
