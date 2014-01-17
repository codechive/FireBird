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
$Id: mail.c,v 1.19 2002/09/05 06:04:10 edwardc Exp $
*/

extern struct postheader header;
#include "bbs.h"

/*For read.c*/
int     auth_search_down();
int     auth_search_up();
int     do_cross();
int     edit_post();
int     Import_post();
int     Save_post();
int     t_search_down();
int     t_search_up();
int     post_search_down();
int     post_search_up();
int     thread_up();
int     thread_down();
/*int     deny_user();*/
int     into_announce();
int     show_user_notes();
int     show_allmsgs();
int     show_author();
int     SR_first_new();
int     SR_last();
int     SR_first();
int     SR_read();
int     SR_read();
int     SR_author();
int     Q_Goodbye();
int     t_friends();
int     s_msg();
int     G_SENDMODE = NA;

extern int numofsig;
extern char quote_file[], quote_user[];
char    currmaildir[STRLEN];
char	*Ctime();

int
chkmail()
{
	static long lasttime = 0;
	static  ismail = 0;
	struct fileheader fh;
	struct stat st;
	int     fd;
	register int i, offset;
	register long numfiles;
	unsigned char ch;
	extern char currmaildir[STRLEN];
	if (!HAS_PERM(PERM_BASIC)) {
		return 0;
	}
	offset = (int) ((char *) &(fh.accessed[0]) - (char *) &(fh));
	if ((fd = open(currmaildir, O_RDONLY)) < 0)
		return (ismail = 0);
	fstat(fd, &st);
	if (lasttime >= st.st_mtime) {
		close(fd);
		return ismail;
	}
	lasttime = st.st_mtime;
	numfiles = st.st_size;
	numfiles = numfiles / sizeof(fh);
	if (numfiles <= 0) {
		close(fd);
		return (ismail = 0);
	}
	lseek(fd, (off_t) (st.st_size - (sizeof(fh) - offset)), SEEK_SET);
	for (i = 0; i < numfiles; i++) {
		read(fd, &ch, 1);
		if (!(ch & FILE_READ)) {
			close(fd);
			return (ismail = 1);
		}
		lseek(fd, (off_t) (-sizeof(fh) - 1), SEEK_CUR);
	}
	close(fd);
	return (ismail = 0);
}

int
check_query_mail(qry_mail_dir)
char    qry_mail_dir[STRLEN];
{
	struct fileheader fh;
	struct stat st;
	int     fd;
	register int offset;
	register long numfiles;
	unsigned char ch;
	offset = (int) ((char *) &(fh.accessed[0]) - (char *) &(fh));
	if ((fd = open(qry_mail_dir, O_RDONLY)) < 0)
		return 0;
	fstat(fd, &st);
	numfiles = st.st_size;
	numfiles = numfiles / sizeof(fh);
	if (numfiles <= 0) {
		close(fd);
		return 0;
	}
	lseek(fd, (off_t) (st.st_size - (sizeof(fh) - offset)), SEEK_SET);
/*    for(i = 0 ; i < numfiles ; i++) {
        read(fd,&ch,1) ;
        if(!(ch & FILE_READ)) {
            close(fd) ;
            return YEA ;
        }
        lseek(fd,-sizeof(fh)-1,SEEK_CUR);
    }*/
/*Â÷½u¬d¸ß·s«H¥u­n¬d¸ß³Ì«á¤@«Ê¬O§_¬°·s«H¡A¨ä¥L¨Ã¤£­«­n*/
/*Modify by SmallPig*/
	read(fd, &ch, 1);
	if (!(ch & FILE_READ)) {
		close(fd);
		return YEA;
	}
	close(fd);
	return NA;
}

int
mailall()
{
	char    ans[4], fname[STRLEN], title[STRLEN];
	char    doc[5][STRLEN], buf[STRLEN];
	int     i;
	
	strcpy(title, "¨S¥DÃD");
	modify_user_mode(SMAIL);
	clear();
	move(0, 0);
	sprintf(fname, "tmp/mailall.%s", currentuser.userid);
	prints("§A­n±Hµ¹©Ò¦³ªº¡G\n");
	prints("(0) ©ñ±ó\n");
	strcpy(doc[0], "(1) ©|¥¼³q¹L¨­¥÷½T»{ªº¨Ï¥ÎªÌ");
	strcpy(doc[1], "(2) ©Ò¦³³q¹L¨­¥÷½T»{ªº¨Ï¥ÎªÌ");
	strcpy(doc[2], "(3) ©Ò¦³ªºªO¥D");
	strcpy(doc[3], "(4) ¥»¯¸´¼Ån¹Î");
	strcpy(doc[4], "(5) ©Ò¦³¥»²ÕÂ´·|­û");
	for (i = 0; i < 5; i++)
		prints("%s\n", doc[i]);
	getdata(8, 0, "½Ð¿é¤J¼Ò¦¡ (0~5)? [0]: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' < 1 || ans[0] - '0' > 5) {
		return NA;
	}
	sprintf(buf, "¬O§_½T©w±Hµ¹%s ", doc[ans[0] - '0' - 1]);
	move(9, 0);
	if (askyn(buf, NA, NA) == NA)
		return NA;
	in_mail = YEA;
	header.reply_mode = NA;
	strcpy(header.title, "¨S¥DÃD");
	strcpy(header.ds, doc[ans[0] - '0' - 1]);
	header.postboard = NA;
	
	if ( post_header(&header)!= NA )
		sprintf(save_title, "[Type %c ¤½§i] %.60s", ans[0], header.title);
	else
		return -2;
		
	setquotefile("");
	do_quote(fname, header.include_mode);
	if (vedit(fname, YEA) == -1) {
		in_mail = NA;
		unlink(fname);
		clear();
		return -2;
	}
	add_loginfo(fname);
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[5;1;32;44m¥¿¦b±H¥ó¤¤¡A½Ðµy­Ô.....                                                        [m");
	refresh();
	mailtoall(ans[0] - '0');
	move(t_lines - 1);
	clrtoeol();
	unlink(fname);
	in_mail = NA;
	return 0;
}


#ifdef INTERNET_EMAIL

void
m_internet()
{
	char    receiver[68];
	if (check_maxmail()) {
		pressreturn();
		return;
	}
	modify_user_mode(SMAIL);

	getdata(1, 0, "¦¬«H¤HE-mail¡G", receiver, 65, DOECHO, YEA);
	strtolower(genbuf, receiver);
	if (strstr(genbuf, ".bbs@msia.pine.ncu.")
		|| strstr(genbuf, ".bbs@localhost")) {
		move(3, 0);
		prints("¯¸¤º«H¥ó, ½Ð¥Î (S)end «ü¥O¨Ó±H\n");
		pressreturn();
	} else if (!invalidaddr(receiver)) {
		*quote_file = '\0';
		clear();
		do_send(receiver, NULL);
	} else {
		move(3, 0);
		prints("¦¬«H¤H¤£¥¿½T, ½Ð­«·s¿ï¨ú«ü¥O\n");
		pressreturn();
	}
	clear();
	refresh();
}
#endif

void
m_init()
{
	sprintf(currmaildir, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, DOT_DIR);
}

int
do_send(userid, title)
char   *userid, *title;
{
	struct fileheader newmessage;
	struct stat st;
	char    filepath[STRLEN], fname[STRLEN], *ip;
	char    save_title2[STRLEN];
	int     fp, count;
	int     internet_mail = 0;
	char    tmp_fname[STRLEN];
	/* I hate go to , but I use it again for the noodle code :-) */
	if (strchr(userid, '@')) {
		internet_mail = YEA;
		sprintf(tmp_fname, "tmp/imail.%s.%05d", currentuser.userid, uinfo.pid);
		strcpy(filepath, tmp_fname);
		goto edit_mail_file;
	}
	/* end of kludge for internet mail */

	if (!getuser(userid))
		return -1;
	if (!(lookupuser.userlevel & PERM_READMAIL))
		return -3;
	sprintf(filepath, "mail/%c/%s", toupper(userid[0]), userid);
	if (stat(filepath, &st) == -1) {
		if (mkdir(filepath, 0755) == -1)
			return -1;
	} else {
		if (!(st.st_mode & S_IFDIR))
			return -1;
	}
	memset(&newmessage, 0, sizeof(newmessage));
	sprintf(fname, "M.%d.A", time(NULL));
	sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);
	ip = strrchr(fname, 'A');
	count = 0;
	while ((fp = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
		if (*ip == 'Z')
			ip++, *ip = 'A', *(ip + 1) = '\0';
		else
			(*ip)++;
		sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);
		if (count++ > MAX_POSTRETRY) {
			return -1;
		}
	}
	close(fp);
	strcpy(newmessage.filename, fname);
#if defined(MAIL_REALNAMES)
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.realname);
#else
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.username);
#endif
	strncpy(newmessage.owner, genbuf, STRLEN);
	sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);

edit_mail_file:
	if (title == NULL) {
		header.reply_mode = NA;
		strcpy(header.title, "¨S¥DÃD");
	} else {
		header.reply_mode = YEA;
		strcpy(header.title, title);
	}
	header.postboard = NA;
	in_mail = YEA;

	setuserfile(genbuf, "signatures");
	ansimore2(genbuf, NA, 0, 18);
	strcpy(header.ds, userid);
	if ( post_header(&header) != NA ) {
		strcpy(newmessage.title, header.title);
		strncpy(save_title, newmessage.title, STRLEN);
		sprintf(save_title2, "(%.16s) %.60s", userid, newmessage.title);
		strncpy(save_filename, fname, 4096);
	} else {
		return -2;		/* edwardc.010719 ¨Ï¥ÎªÌ«ö¤U¤F Q/q ©ñ±ó±H¦^«H */
	}
	
	do_quote(filepath, header.include_mode);

	if (internet_mail) {
		int     res;
		if (vedit(filepath, NA) == -1) {
			unlink(filepath);
			clear();
			return -2;
		}
		add_loginfo(filepath);
		clear();
		prints("«H¥ó§Y±N±Hµ¹ %s \n", userid);
		prints("¼ÐÃD¬°¡G %s \n", header.title);
		if (askyn("½T©w­n±H¥X¶Ü", YEA, NA) == NA) {
			prints("\n«H¥ó¤w¨ú®ø...\n");
			res = -2;
		} else {
#ifdef SENDMAIL_MIME_AUTOCONVERT
			int     ans;
			if (askyn("¬O§_³Æ¥÷µ¹¦Û¤v", NA, NA) == YEA)
				mail_file(tmp_fname, currentuser.userid, save_title2);
			ans = askyn("¥H MIME ®æ¦¡°e«H", NA, NA);
			prints("½Ðµy­Ô, «H¥ó¶Ç»¼¤¤...\n");
			refresh();
			res = bbs_sendmail(tmp_fname, header.title, userid, ans);
#else
			if (askyn("¬O§_³Æ¥÷µ¹¦Û¤v", NA, NA) == YEA)
				mail_file(tmp_fname, currentuser.userid, save_title2);
			prints("½Ðµy­Ô, «H¥ó¶Ç»¼¤¤...\n");
			refresh();
			res = bbs_sendmail(tmp_fname, header.title, userid);
#endif
		}
		unlink(tmp_fname);
		sprintf(genbuf, "mailed %s", userid);
		report(genbuf);
		return res;
	} else {
		if (vedit(filepath, YEA) == -1) {
			unlink(filepath);
			clear();
			return -2;
		}
		add_loginfo(filepath);
		clear();
		if (askyn("¬O§_³Æ¥÷µ¹¦Û¤v", NA, NA) == YEA)
			mail_file(filepath, currentuser.userid, save_title2);
		sprintf(genbuf, "mail/%c/%s/%s", toupper(userid[0]), userid, DOT_DIR);
		if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
			return -1;
		sprintf(genbuf, "mailed %s", userid);
		report(genbuf);
		return 0;
	}
}

int
m_send(userid)
char    userid[];
{
	char    uident[STRLEN];
	if (check_maxmail()) {
		return 0;
	}
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND
		&& uinfo.mode != GMENU) {
		move(1, 0);
		clrtoeol();
		modify_user_mode(SMAIL);
		usercomplete("¦¬«H¤H¡G ", uident);
		if (uident[0] == '\0') {
			return FULLUPDATE;
		}
	} else
		strcpy(uident, userid);
	clear();
	*quote_file = '\0';
	switch (do_send(uident, NULL)) {
	case -1:
		prints("¦¬«HªÌ¤£¥¿½T\n");
		break;
	case -2:
		prints("¨ú®ø\n");
		break;
	case -3:
		prints("[%s] µLªk¦¬«H\n", uident);
		break;
	default:
		prints("«H¥ó¤w±H¥X\n");
	}
	pressreturn();
	return FULLUPDATE;
}

int
read_mail(fptr)
struct fileheader *fptr;
{
	sprintf(genbuf, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, fptr->filename);
	ansimore(genbuf, NA);
	fptr->accessed[0] |= FILE_READ;
	return 0;
}

int     mrd;

int     delmsgs[1024];
int     delcnt;

int
read_new_mail(fptr)
struct fileheader *fptr;
{
	static int idc;
	char    done = NA, delete_it;
	char    fname[256];
	if (fptr == NULL) {
		delcnt = 0;
		idc = 0;
		return 0;
	}
	idc++;
	if (fptr->accessed[0])
		return 0;
	prints("Åª¨ú %s ±H¨Óªº '%s' ?\n", fptr->owner, fptr->title);
	prints("(Yes, or No): ");
	getdata(1, 0, "(Y)Åª¨ú (N)¤£Åª (Q)Â÷¶} [Y]: ", genbuf, 3, DOECHO, YEA);
	if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
		clear();
		return QUIT;
	}
	if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
		clear();
		return 0;
	}
	read_mail(fptr);
	strncpy(fname, genbuf, sizeof(fname));
	mrd = 1;
	if (substitute_record(currmaildir, fptr, sizeof(*fptr), idc))
		return -1;
	delete_it = NA;
	while (!done) {
		move(t_lines - 1, 0);
		prints("(R)¦^«H, (D)§R°£, (G)Ä~Äò ? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			mail_reply(idc, fptr, currmaildir);
			break;
		case 'D':
		case 'd':
			delete_it = YEA;
		default:
			done = YEA;
		}
		if (!done)
			ansimore(fname, NA);	/* re-read */
	}
	if (delete_it) {
		clear();
		sprintf(genbuf, "§R°£«H¥ó [%-.55s]", fptr->title);
		if (askyn(genbuf, NA, NA) == YEA) {
			sprintf(genbuf, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, fptr->filename);
			unlink(genbuf);
			delmsgs[delcnt++] = idc;
		}
	}
	clear();
	return 0;
}

int
m_new()
{
	clear();
	mrd = 0;
	modify_user_mode(RMAIL);
	read_new_mail(NULL);
	if (apply_record(currmaildir, read_new_mail, sizeof(struct fileheader)) == -1) {
		clear();
		move(0, 0);
		prints("No new messages\n\n\n");
		return -1;
	}
	if (delcnt) {
		while (delcnt--)
			delete_record(currmaildir, sizeof(struct fileheader), delmsgs[delcnt]);
	}
	clear();
	move(0, 0);
	if (mrd)
		prints("No more messages.\n\n\n");
	else
		prints("No new messages.\n\n\n");
	return -1;
}

extern char BoardName[];

void
mailtitle()
{
	extern char currmaildir[ STRLEN ];
	int max_num, max_sum, mail_num, mail_sum;
	max_num = (HAS_PERM(PERM_SYSOP)) ?
                MAX_SYSOPMAIL_HOLD : (HAS_PERM(PERM_BOARDS)) ? 
                MAX_BMMAIL_HOLD : MAX_MAIL_HOLD;
	max_sum = (HAS_PERM(PERM_SYSOP)) ?
		MAX_MAIL_SYSOP : (HAS_PERM(PERM_BOARDS)) ?
		MAX_MAIL_BM : MAX_MAIL_SUM;
		
	set_safe_record();
	mail_num = get_num_records(currmaildir, sizeof(struct fileheader));
	mail_sum = get_sum_records(currmaildir, sizeof(struct fileheader));
	
	max_num += (countperf(&currentuser)) / 2;
	max_sum += (countperf(&currentuser)) / 2;	
	
	showtitle("¶l¥ó¿ï³æ    ", BoardName);
	prints("Â÷¶}[[1;32m¡ö[m,[1;32me[m]  ¿ï¾Ü[[1;32m¡ô[m,[1;32m¡õ[m]  ¾\\Åª«H¥ó[[1;32m¡÷[m,[1;32mRtn[m]  ¦^«H[[1;32mR[m]  ¬å«H¡þ²M°£ÂÂ«H[[1;32md[m,[1;32mD[m] ¨D§U[[1;32mh[m][m\n");
	prints("[37;44m ½s¸¹   %-12s %5s %4s %-17s[1;33m(Used:%s%4d[33m/%4dKB %s%4d[33m/%4d«Ê)[m\n", 
		"µo«HªÌ", "¤é ´Á", "¤j¤p", "¼Ð  ÃD", (mail_sum > max_sum) ? "[31m" :"[36m", mail_sum, max_sum, (mail_num > max_num) ? "[31m" : "[36m", mail_num, max_num);
	clrtobot();
}

char   *
maildoent(num, ent)
int     num;
struct fileheader *ent;
{
	static char buf[512];
	char    b2[512];
	time_t  filetime;
	char    status;
	char   *date, color[10];
	char   *t;
	extern char ReadPost[];
	extern char ReplyPost[];
	char    c1[8];
	char    c2[8];
	int     same = NA;
	int	size;
	struct stat st;
#ifdef COLOR_POST_DATE
	struct tm *mytm;
#endif

	filetime = atoi(ent->filename + 2);
	if (filetime > 740000000) {
		date = Ctime(&filetime) + 5;
	} else {
		date = "";
	}

#ifdef COLOR_POST_DATE
	mytm = localtime(&filetime);
	strftime(buf, 5, "%w", mytm);
	sprintf(color, "[1;%dm", 30 + atoi(buf) + 1);
#else
	strcpy(color, "");
#endif

	strcpy(c1, "[1;33m");
	strcpy(c2, "[1;36m");
	if (!strcmp(ReadPost, ent->title) || !strcmp(ReplyPost, ent->title))
		same = YEA;
	strncpy(b2, ent->owner, STRLEN);
	if ((b2[strlen(b2) - 1] == '>') && strchr(b2, '<')) {
		t = strtok(b2, "<>");
		if (invalidaddr(t))
			t = strtok(NULL, "<>");
		if (t != NULL)
			strcpy(b2, t);
	}
	if ((t = strchr(b2, ' ')) != NULL)
		*t = '\0';
	if (ent->accessed[0] & FILE_READ) {
		if (ent->accessed[0] & FILE_MARKED)
			status = 'm';
		else if (ent->accessed[0] & FILE_REPLIED) 
			status = 'r';
		else
			status = ' ';
	} else {
		if (ent->accessed[0] & FILE_MARKED)
			status = 'M';
		else if (ent->accessed[0] & FILE_REPLIED) 
			status = 'R';
		else
			status = 'N';
	}
	
	size = 0;
	sprintf(buf, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, ent->filename);
	if (stat(buf, &st) == 0 && S_ISREG(st.st_mode) && st.st_nlink == 1)
		size = st.st_size / 1024;
		
	if (!strncmp("Re:", ent->title, 3)) {
		sprintf(buf, " %s%4d[m %c %-12.12s %s%5.5s[m %3dK %s%.44s[m", same ? c1 : ""
			,num, status, b2, color, date, size, same ? c1 : "", ent->title);
	} else {
		sprintf(buf, " %s%4d[m %c %-12.12s %s%5.5s[m %3dK ¢E %s%.44s[m"
			,same ? c2 : "", num, status, b2, color, date, size, same ? c2 : "", ent->title);
	}
	return buf;
}
#ifdef POSTBUG
extern int bug_possible;
#endif

int
mail_read(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[512], notgenbuf[128];
	char   *t;
	int     readnext;
	char    done = NA, delete_it, replied;
	clear();
	readnext = NA;
	setqtitle(fileinfo->title);
	strcpy(buf, direct);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	sprintf(notgenbuf, "%s/%s", buf, fileinfo->filename);
	delete_it = replied = NA;
	while (!done) {
		ansimore(notgenbuf, NA);
		move(t_lines - 1, 0);
		prints("(R)¦^«H, (D)§R°£, (G)Ä~Äò? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			replied = YEA;
			mail_reply(ent, fileinfo, direct);
			break;
		case ' ':
		case 'j':
		case KEY_RIGHT:
		case KEY_DOWN:
		case KEY_PGDN:
			done = YEA;
			readnext = YEA;
			break;
		case 'D':
		case 'd':
			delete_it = YEA;
		default:
			done = YEA;
		}
	}
	if (delete_it)
		return mail_del(ent, fileinfo, direct);	/* ­×§ï«H¥ó¤§bug
							 * ¥[¤Freturn */
	else {
		fileinfo->accessed[0] |= FILE_READ;
#ifdef POSTBUG
		if (replied)
			bug_possible = YEA;
#endif
		substitute_record(currmaildir, fileinfo, sizeof(*fileinfo), ent);
#ifdef POSTBUG
		bug_possible = NA;
#endif
	}
	if (readnext == YEA)
		return READ_NEXT;
	return FULLUPDATE;
}
/*ARGSUSED*/
int
mail_reply(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    uid[STRLEN];
	char    title[STRLEN];
	char   *t;
	sprintf(genbuf, "MAILER-DAEMON@%s", BBSHOST);
	if (strstr(fileinfo->owner, genbuf)) {
		ansimore(F_HELP_MAILERR, YEA);
		return FULLUPDATE;
	}
	if (check_maxmail()) {
		pressreturn();
		return 0;
	}
	clear();
	modify_user_mode(SMAIL);
	strncpy(uid, fileinfo->owner, STRLEN);
	if ((uid[strlen(uid) - 1] == '>') && strchr(uid, '<')) {
		t = strtok(uid, "<>");
		if (invalidaddr(t))
			t = strtok(NULL, "<>");
		if (t != NULL)
			strcpy(uid, t);
		else {
			prints("µLªk§ë»¼\n");
			pressreturn();
			return FULLUPDATE;
		}
	}
	if ((t = strchr(uid, ' ')) != NULL)
		*t = '\0';
	if (toupper(fileinfo->title[0]) != 'R' || toupper(fileinfo->title[1]) != 'E' ||
		fileinfo->title[2] != ':')
		strcpy(title, "Re: ");
	else
		title[0] = '\0';
	strncat(title, fileinfo->title, STRLEN - 5);

	sprintf(quote_file, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, fileinfo->filename);
	strcpy(quote_user, fileinfo->owner);
	switch (do_send(uid, title)) {
	case -1:
		prints("µLªk§ë»¼\n");
		break;
	case -2:
		prints("¨ú®ø¦^«H\n");
		break;
	case -3:
		prints("[%s] µLªk¦¬«H\n", uid);
		break;
	default:
		prints("«H¥ó¤w±H¥X\n");
		fileinfo->accessed[0] |= FILE_REPLIED;
		substitute_record(currmaildir, fileinfo, sizeof(*fileinfo), ent);
	}
	pressreturn();
	return FULLUPDATE;
}

int
mail_del(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[512];
	char   *t;
	extern int cmpfilename();
	extern char currfile[];
	sprintf(genbuf, "§R°£«H¥ó [%-.55s]", fileinfo->title);
	if (askyn(genbuf, NA, YEA) == NA) {
		move(t_lines - 1, 0);
		prints("©ñ±ó§R°£«H¥ó...");
		clrtoeol();
		egetch();
		return PARTUPDATE;
	}
	strcpy(buf, direct);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	strncpy(currfile, fileinfo->filename, STRLEN);
	if (!delete_file(direct, sizeof(*fileinfo), ent, cmpfilename)) {
		sprintf(genbuf, "%s/%s", buf, fileinfo->filename);
		unlink(genbuf);
		return DIRCHANGED;
	}
	move(t_lines - 1, 0);
	prints("§R°£¥¢±Ñ...");
	clrtoeol();
	egetch();
	return PARTUPDATE;
}
#ifdef INTERNET_EMAIL

int
mail_forward(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[STRLEN];
	char   *p;
	if (!HAS_PERM(PERM_FORWARD)) {
		return DONOTHING;
	}
	strncpy(buf, direct, STRLEN);
	buf[STRLEN - 1] = '\0';
	if ((p = strrchr(buf, '/')) != NULL)
		*p = '\0';
	switch (doforward(buf, fileinfo, 0)) {
	case 0:
		prints("¤å³¹Âà±H§¹¦¨!\n");
		break;
	case -1:
		prints("Âà±H¥¢±Ñ: ¨t²Îµo¥Í¿ù»~.\n");
		break;
	case -2:
		prints("Âà±H¥¢±Ñ: ¤£¥¿½Tªº¦¬«H¦a§}.\n");
		break;
	default:
		prints("¨ú®øÂà±H...\n");
	}
	pressreturn();
	clear();
	return FULLUPDATE;
}

int
mail_u_forward(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	char    buf[STRLEN];
	char   *p;
	if (!HAS_PERM(PERM_FORWARD)) {
		return DONOTHING;
	}
	strncpy(buf, direct, STRLEN);
	buf[STRLEN - 1] = '\0';
	if ((p = strrchr(buf, '/')) != NULL)
		*p = '\0';
	switch (doforward(buf, fileinfo, 1)) {
	case 0:
		prints("¤å³¹Âà±H§¹¦¨!\n");
		break;
	case -1:
		prints("Âà±H¥¢±Ñ: ¨t²Îµo¥Í¿ù»~.\n");
		break;
	case -2:
		prints("Âà±H¥¢±Ñ: ¤£¥¿½Tªº¦¬«H¦a§}.\n");
		break;
	default:
		prints("¨ú®øÂà±H...\n");
	}
	pressreturn();
	clear();
	return FULLUPDATE;
}
#endif

int
mail_del_range(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	return (del_range(ent, fileinfo, direct));
}

int
mail_mark(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	if (fileinfo->accessed[0] & FILE_MARKED)
		fileinfo->accessed[0] &= ~FILE_MARKED;
	else
		fileinfo->accessed[0] |= FILE_MARKED;
	substitute_record(currmaildir, fileinfo, sizeof(*fileinfo), ent);
	return (PARTUPDATE);
}

extern int mailreadhelp();

struct one_key mail_comms[] = {
	'd', mail_del,
	'D', mail_del_range,
	Ctrl('P'), m_send,
	'E', edit_post,
	'r', mail_read,
	'R', mail_reply,
	'm', mail_mark,
	'i', Save_post,
	'I', Import_post,
	'x', into_announce,
	KEY_TAB, show_user_notes,
#ifdef INTERNET_EMAIL
	'F', mail_forward,
	'U', mail_u_forward,
#endif
	'a', auth_search_down,
	'A', auth_search_up,
	'/', t_search_down,
	'?', t_search_up,
	'\'', post_search_down,
	'\"', post_search_up,
	']', thread_down,
	'[', thread_up,
	Ctrl('A'), show_author,
	Ctrl('N'), SR_first_new,
	'\\', SR_last,
	'=', SR_first,
	'L', show_allmsgs,
	Ctrl('C'), do_cross,
	Ctrl('S'), SR_read,
	'n', SR_first_new,
	'p', SR_read,
	Ctrl('X'), SR_read,
	Ctrl('U'), SR_author,
	'h', mailreadhelp,
	'!', Q_Goodbye,
	'S', s_msg,
	'f', t_friends,
	'\0', NULL
};

int
m_read()
{
	if (!strcmp(currentuser.userid, "guest") || !HAS_PERM(PERM_READMAIL))
		return;
	in_mail = YEA;
	i_read(RMAIL, currmaildir, mailtitle, maildoent, &mail_comms[0], sizeof(struct fileheader));
	in_mail = NA;
	return 0;
}

int
invalidaddr(addr)
char   *addr;
{
	if (*addr == '\0' || !strchr(addr, '@'))
		return 1;
	while (*addr) {
		if (!isalnum(*addr) && !strchr(".!@:-_", *addr))
			return 1;
		addr++;
	}
	return 0;
}
#ifdef INTERNET_EMAIL

#ifdef SENDMAIL_MIME_AUTOCONVERT
int
bbs_sendmail(fname, title, receiver, mime)
char   *fname, *title, *receiver;
int     mime;
#else
int
bbs_sendmail(fname, title, receiver)
char   *fname, *title, *receiver;
#endif
{
	FILE   *fin, *fout;
	sprintf(genbuf, "%s -f %s.bbs@%s %s", SENDMAIL,
		currentuser.userid, BBSHOST, receiver);
	fout = popen(genbuf, "w");
	fin = fopen(fname, "r");
	if (fin == NULL || fout == NULL)
		return -1;

	fprintf(fout, "Return-Path: %s.bbs@%s\n", currentuser.userid, BBSHOST);
	fprintf(fout, "Reply-To: %s.bbs@%s\n", currentuser.userid, BBSHOST);
	fprintf(fout, "From: %s.bbs@%s\n", currentuser.userid, BBSHOST);
	fprintf(fout, "To: %s\n", receiver);
	fprintf(fout, "Subject: %s\n", title);
	fprintf(fout, "X-Forwarded-By: %s (%s)\n",
		currentuser.userid,
#ifdef MAIL_REALNAMES
		currentuser.realname);
#else
		currentuser.username);
#endif

	fprintf(fout, "X-Disclaimer: %s ¹ï¥»«H¤º®e®¤¤£­t³d¡C\n", BoardName);
#ifdef SENDMAIL_MIME_AUTOCONVERT
	if (mime) {
		fprintf(fout, "MIME-Version: 1.0\n");
		fprintf(fout, "Content-Type: text/plain; charset=US-ASCII\n");
		fprintf(fout, "Content-Transfer-Encoding: 8bit\n");
	}
#endif
	fprintf(fout, "Precedence: junk\n\n");

	while (fgets(genbuf, 255, fin) != NULL) {
		if (genbuf[0] == '.' && genbuf[1] == '\n')
			fputs(". \n", fout);
		else
			fputs(genbuf, fout);
	}

	fprintf(fout, ".\n");

	fclose(fin);
	pclose(fout);
	return 0;
}
#endif

int
g_send()
{
	char    uident[13], tmp[3];
	int     cnt, i, n, fmode = NA;
	char    maillists[STRLEN];
	if (check_maxmail()) {
		pressreturn();
		return 0;
	}
	modify_user_mode(SMAIL);
	*quote_file = '\0';
	clear();
	sethomefile(maillists, currentuser.userid, "maillist");
	cnt = listfilecontent(maillists);
	while (1) {
		if (cnt > maxrecp - 10) {
			move(2, 0);
			prints("¥Ø«e­­¨î±H«Hµ¹ [1m%d[m ¤H", maxrecp);
		}
		getdata(0, 0, "(A)¼W¥[ (D)§R°£ (I)¤Þ¤J¦n¤Í (C)²M°£¥Ø«e¦W³æ (E)©ñ±ó (S)±H¥X? [S]¡G ",
			tmp, 2, DOECHO, YEA);
		if (tmp[0] == '\n' || tmp[0] == '\0' || tmp[0] == 's' || tmp[0] == 'S') {
			break;
		}
		if (tmp[0] == 'a' || tmp[0] == 'd' || tmp[0] == 'A' || tmp[0] == 'D') {
			move(1, 0);
			if (tmp[0] == 'a' || tmp[0] == 'A')
				usercomplete("½Ð¨Ì¦¸¿é¤J¨Ï¥ÎªÌ¥N¸¹(¥u«ö ENTER µ²§ô¿é¤J): ", uident);
			else
				namecomplete("½Ð¨Ì¦¸¿é¤J¨Ï¥ÎªÌ¥N¸¹(¥u«ö ENTER µ²§ô¿é¤J): ", uident);
			move(1, 0);
			clrtoeol();
			if (uident[0] == '\0')
				continue;
			if (!getuser(uident)) {
				move(2, 0);
				prints("³o­Ó¨Ï¥ÎªÌ¥N¸¹¬O¿ù»~ªº.\n");
			}
		}
		switch (tmp[0]) {
		case 'A':
		case 'a':
			if (!(lookupuser.userlevel & PERM_READMAIL)) {
				move(2, 0);
				prints("µLªk°e«Hµ¹: [1m%s[m\n", lookupuser.userid);
				break;
			} else if (seek_in_file(maillists, uident)) {
				move(2, 0);
				prints("¤w¸g¦C¬°¦¬¥ó¤H¤§¤@ \n");
				break;
			}
			addtofile(maillists, uident);
			cnt++;
			break;
		case 'E':
		case 'e':
		case 'Q':
		case 'q':
			cnt = 0;
			break;
		case 'D':
		case 'd':
			{
				if (seek_in_file(maillists, uident)) {
					del_from_file(maillists, uident);
					cnt--;
				}
				break;
			}
		case 'I':
		case 'i':
			n = 0;
			clear();
			for (i = cnt; i < maxrecp && n < uinfo.fnum; i++) {
				int     key;
				move(2, 0);
				getuserid(uident, uinfo.friend[n]);
				prints("%s\n", uident);
				move(3, 0);
				n++;
				prints("(A)¥þ³¡¥[¤J (Y)¥[¤J (N)¤£¥[¤J (Q)µ²§ô? [Y]:");
				if (!fmode)
					key = igetkey();
				else
					key = 'Y';
				if (key == 'q' || key == 'Q')
					break;
				if (key == 'A' || key == 'a') {
					fmode = YEA;
					key = 'Y';
				}
				if (key == '\0' || key == '\n' || key == 'y' || key == 'Y') {
					if (!getuser(uident)) {
						move(4, 0);
						prints("³o­Ó¨Ï¥ÎªÌ¥N¸¹¬O¿ù»~ªº.\n");
						i--;
						continue;
					} else if (!(lookupuser.userlevel & PERM_READMAIL)) {
						move(4, 0);
						prints("µLªk°e«Hµ¹: [1m%s[m\n", lookupuser.userid);
						i--;
						continue;
					} else if (seek_in_file(maillists, uident)) {
						i--;
						continue;
					}
					addtofile(maillists, uident);
					cnt++;
				}
			}
			fmode = NA;
			clear();
			break;
		case 'C':
		case 'c':
			unlink(maillists);
			cnt = 0;
			break;
		}
		if (strchr("EeQq", tmp[0]))
			break;
		move(5, 0);
		clrtobot();
		if (cnt > maxrecp)
			cnt = maxrecp;
		move(3, 0);
		clrtobot();
		listfilecontent(maillists);
	}
	if (cnt > 0) {
		G_SENDMODE = 2;
		switch (do_gsend(NULL, NULL, cnt)) {
		case -1:
			prints("«H¥ó¥Ø¿ý¿ù»~\n");
			break;
		case -2:
			prints("¨ú®ø\n");
			break;
		default:
			prints("«H¥ó¤w±H¥X\n");
		}
		G_SENDMODE = 0;
		pressreturn();
	}
	return 0;
}
/*Add by SmallPig*/

int
do_gsend(userid, title, num)
char   *userid[], *title;
int     num;
{
	struct stat st;
	char    filepath[STRLEN], tmpfile[STRLEN], fname[STRLEN];
	int     cnt;
	FILE   *mp;
	in_mail = YEA;
#if defined(MAIL_REALNAMES)
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.realname);
#else
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.username);
#endif
	header.reply_mode = NA;
	strcpy(header.title, "¨S¥DÃD");
	strcpy(header.ds, "±H«Hµ¹¤@¸s¤H");
	header.postboard = NA;
	sprintf(tmpfile, "tmp/gsend.%s.%05d", currentuser.userid, uinfo.pid);

	if ( post_header(&header) != NA ) {
		sprintf(save_title, "[¸sÅé«H¥ó] %.60s", header.title);
		strncpy(save_filename, fname, 4096);
	} else {
		return -2;	/* someone has abondon in the post_header() system call */
	}
		
	do_quote(tmpfile, header.include_mode);
	if (vedit(tmpfile, YEA) == -1) {
		unlink(tmpfile);
		clear();
		return -2;
	}
	add_loginfo(tmpfile);
	clear();
	prints("[5;1;32m¥¿¦b±H¥ó¤¤¡A½Ðµy­Ô...[m");
	if (G_SENDMODE == 2) {
		char    maillists[STRLEN];
		setuserfile(maillists, "maillist");
		if ((mp = fopen(maillists, "r")) == NULL) {
			return -3;
		}
	}
	for (cnt = 0; cnt < num; cnt++) {
		char    uid[13];
		char    buf[STRLEN];
		if (G_SENDMODE == 1)
			getuserid(uid, uinfo.friend[cnt]);
		else if (G_SENDMODE == 2) {
			if (fgets(buf, STRLEN, mp) != NULL) {
				if (strtok(buf, " \n\r\t") != NULL)
					strcpy(uid, buf);
				else
					continue;
			} else {
				cnt = num;
				continue;
			}
		} else
			strcpy(uid, userid[cnt]);
		sprintf(filepath, "mail/%c/%s", toupper(uid[0]), uid);
		if (stat(filepath, &st) == -1) {
			if (mkdir(filepath, 0755) == -1) {
				if (G_SENDMODE == 2)
					fclose(mp);
				return -1;
			}
		} else {
			if (!(st.st_mode & S_IFDIR)) {
				if (G_SENDMODE == 2)
					fclose(mp);
				return -1;
			}
		}
		mail_file(tmpfile, uid, save_title);
	}
	unlink(tmpfile);
	clear();
	if (G_SENDMODE == 2)
		fclose(mp);
	return 0;
}

int
mail_file(tmpfile, userid, title)
char    tmpfile[STRLEN], userid[STRLEN], title[STRLEN];
{
	struct fileheader newmessage;
	struct stat st;
	char    fname[STRLEN], filepath[STRLEN], *ip;
	int     fp, count;
	memset(&newmessage, 0, sizeof(newmessage));
#if defined(MAIL_REALNAMES)
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.realname);
#else
	sprintf(genbuf, "%s (%s)", currentuser.userid, currentuser.username);
#endif
	strncpy(newmessage.owner, genbuf, STRLEN);
	strncpy(newmessage.title, title, STRLEN);
	strncpy(save_title, newmessage.title, STRLEN);

	sprintf(filepath, "mail/%c/%s", toupper(userid[0]), userid);
	if (stat(filepath, &st) == -1) {
		if (mkdir(filepath, 0755) == -1)
			return -1;
	} else {
		if (!(st.st_mode & S_IFDIR))
			return -1;
	}
	sprintf(fname, "M.%d.A", time(NULL));
	sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);
	ip = strrchr(fname, 'A');
	count = 0;
	while ((fp = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
		if (*ip == 'Z')
			ip++, *ip = 'A', *(ip + 1) = '\0';
		else
			(*ip)++;
		sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);
		if (count++ > MAX_POSTRETRY) {
			return -1;
		}
	}
	close(fp);
	strcpy(newmessage.filename, fname);
	strncpy(save_filename, fname, 4096);
	sprintf(filepath, "mail/%c/%s/%s", toupper(userid[0]), userid, fname);

	f_cp(tmpfile, filepath, O_CREAT);

	sprintf(genbuf, "mail/%c/%s/%s", toupper(userid[0]), userid, DOT_DIR);
	if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
		return -1;
	sprintf(genbuf, "mailed %s ", userid);
	report(genbuf);
	return 0;
}
/*Add by SmallPig*/
int
ov_send()
{
	int     all, i;
	if (check_maxmail()) {
		pressreturn();
		return 0;
	}
	modify_user_mode(SMAIL);
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("±H«Hµ¹¦n¤Í¦W³æ¤¤ªº¤H¡A¥Ø«e¥»¯¸­­¨î¶È¥i¥H±Hµ¹ [1m%d[m ¦ì¡C\n", maxrecp);
	if (uinfo.fnum <= 0) {
		prints("§A¨Ã¨S¦³³]©w¦n¤Í¡C\n");
		pressanykey();
		clear();
		return 0;
	} else {
		prints("¦W³æ¦p¤U¡G\n");
	}
	G_SENDMODE = 1;
	all = (uinfo.fnum >= maxrecp) ? maxrecp : uinfo.fnum;
	for (i = 0; i < all; i++) {
		char    uid[IDLEN + 2];
		getuserid(uid, uinfo.friend[i]);
		prints("%-12s ", uid);
		if ((i + 1) % 6 == 0)
			outc('\n');
	}
	pressanykey();
	switch (do_gsend(NULL, NULL, all)) {
	case -1:
		prints("«H¥ó¥Ø¿ý¿ù»~\n");
		break;
	case -2:
		prints("«H¥ó¨ú®ø\n");
		break;
	default:
		prints("«H¥ó¤w±H¥X\n");
	}
	pressreturn();
	G_SENDMODE = 0;
	return 0;
}

int
in_group(uident, cnt)
char    uident[maxrecp][STRLEN];
int     cnt;
{
	int     i;
	for (i = 0; i < cnt; i++)
		if (!strcmp(uident[i], uident[cnt])) {
			return i + 1;
		}
	return 0;
}
#ifdef INTERNET_EMAIL

int
doforward(direct, fh, mode)
char   *direct;
struct shortfile *fh;
int     mode;
{
	static char address[STRLEN];
	char    fname[STRLEN], tmpfname[STRLEN];
	char    receiver[STRLEN];
	char    title[STRLEN];
	int     return_no;
	clear();
	if (address[0] == '\0') {
		strncpy(address, currentuser.email, STRLEN);
	}
	if (HAS_PERM(PERM_SETADDR)) {
		prints("½Ðª½±µ«ö Enter ±µ¨ü¬A¸¹¤º´£¥Üªº¦a§}, ©ÎªÌ¿é¤J¨ä¥L¦a§}\n");
		prints("§â«H¥óÂà±Hµ¹ [%s]\n", address);
		getdata(2, 0, "==> ", receiver, 70, DOECHO, YEA);
	}
	if (receiver[0] != '\0') {
		strncpy(address, receiver, STRLEN);
	}
	sprintf(genbuf, ".bbs@%s", BBSHOST);
	if (strstr(receiver, genbuf)
		|| strstr(receiver, ".bbs@localhost")) {
		char   *pos;
		pos = strchr(address, '.');
		*pos = '\0';
	}
	sprintf(genbuf, "½T©w±N¤å³¹±Hµ¹ %s ¶Ü", address);
	if (askyn(genbuf, YEA, NA) == 0)
		return 1;
	if (invalidaddr(address))
		if (check_maxmail() || !getuser(address))
			return -2;
	sprintf(tmpfname, "tmp/forward.%s.%05d", currentuser.userid, uinfo.pid);

	sprintf(genbuf, "%s/%s", direct, fh->filename);
	f_cp(genbuf, tmpfname, O_CREAT);

	if (askyn("¬O§_­×§ï¤å³¹¤º®e", NA, NA) == 1) {
		if (vedit(tmpfname, NA) == -1) {
			if (askyn("¬O§_±H¥X¥¼­×§ïªº¤å³¹", YEA, NA) == 0) {
				unlink(tmpfname);
				clear();
				return 1;
			}
		} else if (ADD_EDITMARK)
			add_edit_mark(tmpfname, 1, NULL);
		clear();
	}
	add_crossinfo(tmpfname, 2);
	prints("Âà±H«H¥óµ¹ %s, ½Ðµy­Ô....\n", address);
	refresh();

	if (mode == 0)
		strcpy(fname, tmpfname);
	else if (mode == 1) {
		sprintf(fname, "tmp/file.uu%05d", uinfo.pid);
		sprintf(genbuf, "uuencode %s fb-bbs.%05d > %s",
			tmpfname, uinfo.pid, fname);
		system(genbuf);
	}
	sprintf(title, "[Âà±H] %.70s", fh->title);
	if (!strpbrk(address, "@."))
		return_no = mail_file(fname, lookupuser.userid, title);
	else {
#ifdef SENDMAIL_MIME_AUTOCONVERT
		if (askyn("¥H MIME ®æ¦¡°e«H", NA, NA) == YEA)
			return_no = bbs_sendmail(fname, title, address, YEA);
		else
			return_no = bbs_sendmail(fname, title, address, NA);
#else
		return_no = bbs_sendmail(fname, title, address);
#endif
	}
	if (mode == 1) {
		unlink(fname);
	}
	unlink(tmpfname);
	return (return_no);
}
#endif

#if 0
int
setguard()
{
	FILE   *fp;
	clear();
	move(3, 0);
	setuserfile(genbuf, "mailguard");
	if (dashf(genbuf)) {
		unlink(genbuf);
		prints("³]¦¨±µ¦¬©Ò¦³¨Ó·½ªº Internet E-mail...");
	} else if ((fp = fopen(genbuf, "w")) != NULL) {
		fclose(fp);
		prints("³]¦¨¥u±µ¦¬±q¾Ç³Nºô¸ô¡BBBS¡B¥H¤Î¤j°¨©Òµo¥Xªº Internet E-mail...");
	} else
		prints("³]©w¥¢±Ñ...");
	pressreturn();
	return 0;
}
#endif