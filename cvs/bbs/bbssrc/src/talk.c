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
$Id: talk.c,v 1.16 2002/09/11 09:51:47 edwardc Exp $
*/

#include "bbs.h"
#ifdef lint
#include <sys/uio.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define M_INT 8			/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
extern char BoardName[];
extern int iscolor;
extern int numf, friendmode;

int     talkidletime = 0;
int     ulistpage;
int     friendflag = 1;
int     friend_query();
int     friend_mail();
int     friend_dele();
int     friend_add();
int     friend_edit();
int     friend_help();
int     reject_query();
int     reject_dele();
int     reject_add();
int     reject_edit();
int     reject_help();
#ifdef TALK_LOG
void    do_log();
int     talkrec = -1;
char    partner[IDLEN + 1];
#endif

struct one_key friend_list[] = {
	'r', friend_query,
	'm', friend_mail,
	'M', friend_mail,
	'a', friend_add,
	'A', friend_add,
	'd', friend_dele,
	'D', friend_dele,
	'E', friend_edit,
	'h', friend_help,
	'H', friend_help,
	'\0', NULL
};

struct one_key reject_list[] = {
	'r', reject_query,
	'a', reject_add,
	'A', reject_add,
	'd', reject_dele,
	'D', reject_dele,
	'E', reject_edit,
	'h', reject_help,
	'H', reject_help,
	'\0', NULL
};

struct talk_win {
	int     curcol, curln;
	int     sline, eline;
};

int     nowmovie;
int     bind();

static char *refuse[] = {
	"©êºp¡A§Ú²{¦b·Q±M¤ß¬Ý Board¡C    ", "§Ú¤µ¤Ñ«Ü²Ö¡A¤£·Q¸ò§O¤H²á¤Ñ¡C    ",
	"§Ú²{¦b¦³¨Æ¡Aµ¥¤@¤U¦A Call §A¡C  ", "§Ú°¨¤W­nÂ÷¶}¤F¡A¤U¦¸¦A²á§a¡C    ",
	"½Ð§A¤£­n¦A Page¡A§Ú¤£·Q¸ò§A²á¡C ", "½Ð¥ý¼g¤@«Ê¦Û§Ú¤¶²Ðµ¹§Ú¡A¦n¶Ü¡H  ",
	"¹ï¤£°_¡A§Ú²{¦b¦bµ¥¤H¡C          ", "½Ð¤£­n§n§Ú¡A¦n¶Ü¡H..... :)      ",
NULL};

extern int t_columns;

char    save_page_requestor[STRLEN];
int     t_cmpuids();
int     cmpfnames();
char	*ModeType();

int
ishidden(user)
char   *user;
{
	int     tuid;
	struct user_info uin;
	if (!(tuid = getuser(user)))
		return 0;
	if (!search_ulist(&uin, t_cmpuids, tuid))
		return 0;
	return (uin.invisible);
}

char   *
modestring(mode, towho, complete, chatid)
int     mode, towho, complete;
char   *chatid;
{
	static char modestr[STRLEN];
	struct userec urec;
	if (chatid) {
		if (complete)
			sprintf(modestr, "%s as '%s'", ModeType(mode), chatid);
		else
			return (ModeType(mode));
		return (modestr);
	}
	if (mode != TALK && mode != PAGE && mode != QUERY)
		return (ModeType(mode));
	if (get_record(PASSFILE, &urec, sizeof(urec), towho) == -1)
		return (ModeType(mode));

	if (mode != QUERY && !HAS_PERM(PERM_SYSOP | PERM_SEECLOAK) &&
		ishidden(urec.userid))
		return (ModeType(TMENU));
	if (complete)
		sprintf(modestr, "%s '%s'", ModeType(mode), urec.userid);
	else
		return (ModeType(mode));
	return (modestr);
}

char
pagerchar(friend, pager)
int     friend, pager;
{
	if (pager & ALL_PAGER)
		return ' ';
	if ((friend)) {
		if (pager & FRIEND_PAGER)
			return 'O';
		else
			return '#';
	}
	return '*';
}

char
canpage(friend, pager)
int     friend, pager;
{
	if ((pager & ALL_PAGER) || HAS_PERM(PERM_SYSOP | PERM_FORCEPAGE))
		return YEA;
	if ((pager & FRIEND_PAGER)) {
		if (friend)
			return YEA;
	}
	return NA;
}
#ifdef SHOW_IDLE_TIME
char   *
idle_str(uent)
struct user_info *uent;
{
	static char hh_mm_ss[32];
#ifndef BBSD
	struct stat buf;
	char    tty[128];
#endif
	time_t  now, diff;
	int     limit, hh, mm;
	if (uent == NULL) {
		strcpy(hh_mm_ss, "¤£¸Ô");
		return hh_mm_ss;
	}
#ifndef BBSD
	strcpy(tty, uent->tty);

#ifndef SOLARIS
	if ((stat(tty, &buf) != 0) ||
		(strstr(tty, "tty") == NULL)) {
		strcpy(hh_mm_ss, "¤£¸Ô");
		return hh_mm_ss;
	}
#else
	if ((stat(tty, &buf) != 0) ||
		(strstr(tty, "pts") == NULL)) {
		strcpy(hh_mm_ss, "¤£¸Ô");
		return hh_mm_ss;
	}
#endif
#endif

	now = time(0);
	
#ifndef BBSD
	diff = now - buf.st_atime;
#else
	if ( uent->mode == TALK )
	    diff = talkidletime;	/* ²á¤Ñ¥t¦³¦Û¤vªº idle kick ¾÷¨î */
	else
		diff = now - uent->idle_time;
#endif

#ifdef DOTIMEOUT
	/*
	 * the 60 * 60 * 24 * 5 is to prevent fault /dev mount from kicking
	 * out all users
	 */

	if (uent->ext_idle)
		limit = IDLE_TIMEOUT * 3;
	else
		limit = IDLE_TIMEOUT;

	if ((diff > limit) && (diff < 86400 * 5) && !HAS_PERM(PERM_SYSOP))
		kill(uent->pid, SIGHUP);
#endif

	hh = diff / 3600;
	mm = (diff / 60) % 60;

	if (hh > 0)
		sprintf(hh_mm_ss, "%d:%02d", hh, mm);
	else if (mm > 0)
		sprintf(hh_mm_ss, "%d", mm);
	else
		sprintf(hh_mm_ss, "   ");

	return hh_mm_ss;
}
#endif


int
listcuent(uentp)
struct user_info *uentp;
{
	if (uentp == NULL) {
		CreateNameList();
		return 0;
	}
	if (uentp->uid == usernum)
		return 0;
	if (!uentp->active || !uentp->pid || isreject(uentp))
		return 0;
	if (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK) && uentp->invisible)
		return 0;
	AddNameList(uentp->userid);
	return 0;
}

void
creat_list()
{
	listcuent(NULL);
	apply_ulist(listcuent);
}

int
t_pager()
{

	if (uinfo.pager & ALL_PAGER) {
		uinfo.pager &= ~ALL_PAGER;
		if (DEFINE(DEF_FRIENDCALL))
			uinfo.pager |= FRIEND_PAGER;
		else
			uinfo.pager &= ~FRIEND_PAGER;
	} else {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}

	if (!uinfo.in_chat && uinfo.mode != TALK) {
		move(1, 0);
		prints("±zªº©I¥s¾¹ (pager) ¤w¸g[1m%s[m¤F!",
			(uinfo.pager & ALL_PAGER) ? "¥´¶}" : "Ãö³¬");
		pressreturn();
	}
	update_utmp();
	return 0;
}




/*Add by SmallPig*/
/*¦¹¨ç¼Æ¥u­t³d¦C¦L»¡©úÀÉ¡A¨Ã¤£ºÞ²M°£©Î©w¦ìªº°ÝÃD¡C*/
int
show_user_plan(userid)
char   *userid;
{
	int     i, j, ci;
	char    pfile[STRLEN], pbuf[256];
	FILE   *pf;
	sethomefile(pfile, userid, "plans");
	if ((pf = fopen(pfile, "r")) == NULL) {
		prints("[1;36m¨S¦³­Ó¤H»¡©úÀÉ[m\n");
		return NA;
	} else {
		prints("[1;36m­Ó¤H»¡©úÀÉ¦p¤U¡G[m\n");
		for (i = 1; i <= MAXQUERYLINES; i++) {
			if (fgets(pbuf, sizeof(pbuf), pf)) {
				for (j = 0; j < strlen(pbuf); j++)
					if (!strncasecmp(&pbuf[j], "$username", 9) ) {
						j += 8;
						outs(currentuser.userid);
					} else if (!strncasecmp(&pbuf[j], "$nickname", 9) ) {
						j += 8;
						outs(currentuser.username);          
					} else if (pbuf[j] != '\033')
						outc(pbuf[j]);
					else {
						ci = strspn(&pbuf[j], "\033[0123456789;");
						if (pbuf[ci + j] != 'm')
							j += ci;
						else
							outc(pbuf[j]);
					}
			} else
				break;
		}
		fclose(pf);
		prints("[m");
		return YEA;
	}
}



/* Modified By Excellent*/
int
t_query(q_id)
char    q_id[IDLEN + 2];
{
	char    uident[STRLEN], *newline;
	int     tuid = 0, clr = 0;
	int     exp, perf;	/* Add by SmallPig */
	struct user_info uin;
	char    qry_mail_dir[STRLEN];
	char    planid[IDLEN + 2], buf[50];
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND && uinfo.mode != READING
		&& uinfo.mode != MAIL && uinfo.mode != RMAIL && uinfo.mode != GMENU) {
		modify_user_mode(QUERY);
		refresh();
		move(1, 0);
		clrtobot();
		prints("¬d¸ß½Ö:\n<¿é¤J¨Ï¥ÎªÌ¥N¸¹, «öªÅ¥ÕÁä¥i¦C¥X²Å¦X¦r¦ê>\n");
		move(1, 8);
		usercomplete(NULL, uident);
		if (uident[0] == '\0') {
			return 0;
		}
	} else {
		if (*q_id == '\0')
			return 0;
		if (strchr(q_id, ' '))
			strtok(q_id, " ");
		strncpy(uident, q_id, sizeof(uident));
		uident[sizeof(uident) - 1] = '\0';
	}
	if (!(tuid = getuser(uident))) {
		move(2, 0);
		clrtoeol();
		prints("[1m¤£¥¿½Tªº¨Ï¥ÎªÌ¥N¸¹[m\n");
		pressanykey();
		return -1;
	}
	uinfo.destuid = tuid;
	update_utmp();

	move(1, 0);
	clrtobot();
	sprintf(qry_mail_dir, "mail/%c/%s/%s", toupper(lookupuser.userid[0]), lookupuser.userid, DOT_DIR);

	exp = countexp(&lookupuser);
	perf = countperf(&lookupuser);
	prints("[1;37m%s [m([1;33m%s[m) ¦@¤W¯¸ [1;32m%d[m ¦¸¡Aµoªí¹L [1;32m%d[m ½g¤å³¹",
		lookupuser.userid, lookupuser.username, lookupuser.numlogins, lookupuser.numposts);
	strcpy(planid, lookupuser.userid);
	//strcpy(genbuf, Ctime(&(lookupuser.lastlogin)));
	if ((newline = strchr(genbuf, '\n')) != NULL)
		*newline = '\0';

	if ( HAS_DEFINE(lookupuser.userdefine, DEF_COLOREDSEX) )
		clr = (lookupuser.gender == 'F') ? 5 : 6;
	else
		clr = 2;

	if ( strcasecmp(lookupuser.userid, "guest") != 0 )
		sprintf(buf, "[[1;3%dm%s[m] ", clr, horoscope(lookupuser.birthmonth, lookupuser.birthday));
	else
		sprintf(buf, "");
		
	prints("\n%s¤W¦¸¦b [[1;32m%s[m] ±q [[1;32m%s[m] ¨ì¥»¯¸¤@¹C¡C\n",
		( HAS_DEFINE(lookupuser.userdefine, DEF_S_HOROSCOPE) ) ? buf : "", Ctime(&(lookupuser.lastlogin)),
		(lookupuser.lasthost[0] == '\0' ? "(¤£¸Ô)" : lookupuser.lasthost));
	prints("«H½c¡G[[1;5;32m%2s[m]¡A¸gÅç­È¡G[[1;32m%d[m]([1;33m%s[m) ªí²{­È¡G[[1;32m%d[m]([1;33m%s[m) ¥Í©R¤O¡G[[1;32m%d[m]¡C\n"
		,(check_query_mail(qry_mail_dir) == 1) ? "¡ó" : "  ", exp, cexp(exp), perf,
		cperf(perf), compute_user_value(&lookupuser));
	t_search_ulist(&uin, t_cmpuids, tuid);
#if defined(QUERY_REALNAMES)
	if (HAS_PERM(PERM_SYSOP))
		prints("¯u¹ê©m¦W: %s \n", lookupuser.realname);
#endif
	show_user_plan(planid);
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND && uinfo.mode != GMENU)
		pressanykey();

	uinfo.destuid = 0;
	return 0;
}

int
count_active(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     c = count;
		count = 0;
		return c;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	count++;
	return 1;
}

int
count_useshell(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     c = count;
		count = 0;
		return c;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (uentp->mode == WWW || uentp->mode == SYSINFO || uentp->mode == HYTELNET
		|| uentp->mode == DICT || uentp->mode == ARCHIE
		|| uentp->mode == IRCCHAT || uentp->mode == BBSNET || uentp->mode == GAME)
		count++;
	return 1;
}

int
count_user_logins(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     c = count;
		count = 0;
		return c;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (!strcmp(uentp->userid, save_page_requestor))
		count++;
	return 1;
}

int
count_visible_active(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     c = count;
		count = 0;
		return c;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	count++;
	if (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK) && uentp->invisible)
		count--;
	return 1;
}

int
alcounter(uentp)
struct user_info *uentp;
{
	static int vi_users, vi_friends;
	int     canseecloak;
	if (uentp == NULL) {
		count_friends = vi_friends;
		count_users = vi_users;
		vi_users = vi_friends = 0;
		return 1;
	}
	if (!uentp->active || !uentp->pid || isreject(uentp))
		return 0;

	canseecloak = (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK) && uentp->invisible) ? 0 : 1;
	if (myfriend(uentp->uid)) {
		vi_friends++;
		if (!canseecloak)
			vi_friends--;
	}
	vi_users++;
	if (!canseecloak)
		vi_users--;
	return 1;
}

int
num_alcounter()
{
	alcounter(NULL);
	apply_ulist(alcounter);
	alcounter(NULL);
	return;
}

int
num_useshell()
{
	count_useshell(NULL);
	apply_ulist(count_useshell);
	return count_useshell(NULL);
}

int
num_active_users()
{
	count_active(NULL);
	apply_ulist(count_active);
	return count_active(NULL);
}
int
num_user_logins(uid)
char   *uid;
{
	strcpy(save_page_requestor, uid);
	count_active(NULL);
	apply_ulist(count_user_logins);
	return count_user_logins(NULL);
}
int
num_visible_users()
{
	count_visible_active(NULL);
	apply_ulist(count_visible_active);
	return count_visible_active(NULL);
}

int
cmpfnames(userid, uv)
char   *userid;
struct override *uv;
{
	return !strcmp(userid, uv->id);
}

int
t_cmpuids(uid, up)
int     uid;
struct user_info *up;
{
	return (up->active && uid == up->uid);
}

int
t_talk()
{
	int     netty_talk;
#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	refresh();
	netty_talk = ttt_talk();
	clear();
	return (netty_talk);
}

int
ttt_talk(userinfo)
struct user_info *userinfo;
{
	char    uident[STRLEN];
	char    reason[STRLEN];
	int     tuid, ucount, unum, tmp;
	struct user_info uin;
	time_t	pgtime;
	
	move(1, 0);
	clrtobot();
	if (uinfo.invisible) {
		move(2, 0);
		prints("©êºp, ¦¹¥\\¯à¦bÁô¨­ª¬ºA¤U¤£¯à°õ¦æ...\n");
		pressreturn();
		return 0;
	}
	if (uinfo.mode != LUSERS && uinfo.mode != FRIEND) {
		move(2, 0);
		prints("<¿é¤J¨Ï¥ÎªÌ¥N¸¹>\n");
		move(1, 0);
		clrtoeol();
		prints("¸ò½Ö²á¤Ñ: ");
		creat_list();
		namecomplete(NULL, uident);
		if (uident[0] == '\0') {
			clear();
			return 0;
		}
		if (!(tuid = searchuser(uident)) || tuid == usernum) {
	wrongid:
			move(2, 0);
			prints("¿ù»~¥N¸¹\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		ucount = count_logins(&uin, t_cmpuids, tuid, 0);
		move(3, 0);
		prints("¥Ø«e %s ªº %d logins ¦p¤U: \n", uident, ucount);
		clrtobot();
		if (ucount > 1) {
	list:		move(5, 0);
			prints("(0) ºâ¤Fºâ¤F¡A¤£²á¤F¡C\n");
			ucount = count_logins(&uin, t_cmpuids, tuid, 0);
			count_logins(&uin, t_cmpuids, tuid, 1);
			clrtobot();
			tmp = ucount + 8;
			getdata(tmp, 0, "½Ð¿ï¤@­Ó§A¬Ýªº¤ñ¸û¶¶²´ªº [0]: ",
				genbuf, 4, DOECHO, YEA);
			unum = atoi(genbuf);
			if (unum == 0) {
				clear();
				return 0;
			}
			if (unum > ucount || unum < 0) {
				move(tmp, 0);
				prints("²Â²Â¡I§A¿ï¿ù¤F°Õ¡I\n");
				clrtobot();
				pressreturn();
				goto list;
			}
			if (!search_ulistn(&uin, t_cmpuids, tuid, unum))
				goto wrongid;
		} else if (!search_ulist(&uin, t_cmpuids, tuid))
			goto wrongid;
	} else {
/*     memcpy(&uin,userinfo,sizeof(uin));     */
		uin = *userinfo;
		tuid = uin.uid;
		strcpy(uident, uin.userid);
		move(1, 0);
		clrtoeol();
		prints("¸ò½Ö²á¤Ñ: %s", uin.userid);
	}

	/* youzi : check guest */
	if (!strcmp(uin.userid, "guest") && !HAS_PERM(PERM_FORCEPAGE))
		return -1;
		
	/* check if pager on/off       --gtv */
	if (!canpage(hisfriend(&uin), uin.pager)) {
		move(2, 0);
		prints("¹ï¤è©I¥s¾¹¤wÃö³¬.\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	}

	if (uin.mode == SYSINFO || uin.mode == IRCCHAT || uin.mode == BBSNET ||
		uin.mode == DICT || uin.mode == ADMIN || uin.mode == ARCHIE
		|| uin.mode == LOCKSCREEN || uin.mode == GAME || uin.mode == WWW
		|| uin.mode == HYTELNET || uin.mode == PAGE) {
		move(2, 0);
		prints("¥Ø«eµLªk©I¥s.\n");
		clrtobot();
		pressreturn();
		return -1;
	}

	if (!uin.active || (kill(uin.pid, 0) == -1)) {
		move(2, 0);
		prints("¹ï¤è¤wÂ÷¶}\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	} else {
		int     sock, msgsock, length;
		struct sockaddr_in server;
		char    c;
		char    buf[512];
		move(3, 0);
		clrtobot();
		show_user_plan(uident);
		move(2, 0);
		if (askyn("½T©w­n©M¥L/¦o½Í¤Ñ¶Ü", NA, NA) == NA) {
			clear();
			return 0;
		}
		sprintf(buf, "Talk to '%s'", uident);
		report(buf);
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("socket err\n");
			return -1;
		}
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = 0;
		if (bind(sock, (struct sockaddr *) & server, sizeof(server)) < 0) {
			perror("bind err");
			return -1;
		}
		length = sizeof(server);
		if (getsockname(sock, (struct sockaddr *) & server, &length) < 0) {
			perror("socket name err");
			return -1;
		}
		uinfo.sockactive = YEA;
		uinfo.sockaddr = server.sin_port;
		uinfo.destuid = tuid;
		modify_user_mode(PAGE);
		kill(uin.pid, SIGUSR1);
		clear();
		prints("©I¥s %s ¤¤...\n¿é¤J Ctrl-D µ²§ô\n", uident);

		listen(sock, 1);
		add_io(sock, 20);
		pgtime = time(0);
		while (YEA) {
			int     ch;
			
			ch = igetkey();
			
			if ( (time(0) - pgtime) > MAX_PAGEWAIT ) {
				move(0, 0);
				prints("©I¥s¹O®É¡A½Ð­«·s©I¥s¡C\n");
				add_io(0, 0);
				close(sock);
				uinfo.sockactive = NA;
				uinfo.destuid = 0;
				pressreturn();
				return -1;
			}

			if (ch == I_TIMEOUT) {
				move(0, 0);
				prints("¦A¦¸©I¥s.\n");
				bell();
				if (kill(uin.pid, 0) == -1) {
					move(0, 0);
					prints("¹ï¤è¤wÂ÷½u\n");
					add_io(0, 0);
					close(sock);
					uinfo.sockactive = NA;
					uinfo.destuid = 0;
					pressreturn();
					return -1;
				}
				continue;
			}
			if (ch == I_OTHERDATA)
				break;
			if (ch == '\004') {
				add_io(0, 0);
				close(sock);
				uinfo.sockactive = NA;
				uinfo.destuid = 0;
				clear();
				return 0;
			}
		}

		msgsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
		if (msgsock == -1) {
			perror("accept");
			return -1;
		}
		add_io(0, 0);
		close(sock);
		uinfo.sockactive = NA;
/*      uinfo.destuid = 0 ;*/
		read(msgsock, &c, sizeof(c));

		clear();

		switch (c) {
		case 'y':
		case 'Y':
			sprintf(save_page_requestor, "%s (%s)", uin.userid, uin.username);
			do_talk(msgsock);
			break;
		case 'a':
		case 'A':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[0]);
			pressreturn();
			break;
		case 'b':
		case 'B':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[1]);
			pressreturn();
			break;
		case 'c':
		case 'C':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[2]);
			pressreturn();
			break;
		case 'd':
		case 'D':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[3]);
			pressreturn();
			break;
		case 'e':
		case 'E':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[4]);
			pressreturn();
			break;
		case 'f':
		case 'F':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[5]);
			pressreturn();
			break;
		case 'g':
		case 'G':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[6]);
			pressreturn();
			break;
		case 'n':
		case 'N':
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, refuse[7]);
			pressreturn();
			break;
		case 'm':
		case 'M':
			read(msgsock, reason, sizeof(reason));
			prints("%s (%s)»¡¡G%s\n", uin.userid, uin.username, reason);
			pressreturn();
			break;
		default:
			sprintf(save_page_requestor, "%s (%s)", uin.userid, uin.username);
#ifdef TALK_LOG
			strcpy(partner, uin.userid);
#endif
			do_talk(msgsock);
			break;
		}
		close(msgsock);
		clear();
		refresh();
		uinfo.destuid = 0;
	}
	return 0;
}

extern int talkrequest;
struct user_info ui;
char    page_requestor[STRLEN];
char    page_requestorid[STRLEN];

int
cmpunums(unum, up)
int     unum;
struct user_info *up;
{
	if (!up->active)
		return 0;
	return (unum == up->destuid);
}

int
cmpmsgnum(unum, up)
int     unum;
struct user_info *up;
{
	if (!up->active)
		return 0;
	return (unum == up->destuid && up->sockactive == 2);
}

int
setpagerequest(mode)
int     mode;
{
	int     tuid;
	if (mode == 0)
		tuid = search_ulist(&ui, cmpunums, usernum);
	else
		tuid = search_ulist(&ui, cmpmsgnum, usernum);
	if (tuid == 0)
		return 1;
	if (!ui.sockactive)
		return 1;
	uinfo.destuid = ui.uid;
	sprintf(page_requestor, "%s (%s)", ui.userid, ui.username);
	strcpy(page_requestorid, ui.userid);
	return 0;
}

int
servicepage(line, mesg)
int     line;
char   *mesg;
{
	static time_t last_check;
	time_t  now;
	char    buf[STRLEN];
	int     tuid = search_ulist(&ui, cmpunums, usernum);
	if (tuid == 0 || !ui.sockactive)
		talkrequest = NA;
	if (!talkrequest) {
		if (page_requestor[0]) {
			switch (uinfo.mode) {
			case TALK:
				move(line, 0);
				printdash(mesg);
				break;
			default:	/* a chat mode */
				sprintf(buf, "** %s ¤w°±¤î©I¥s.", page_requestor);
				printchatline(buf);
			}
			memset(page_requestor, 0, STRLEN);
			last_check = 0;
		}
		return NA;
	} else {
		now = time(0);
		if (now - last_check > P_INT) {
			last_check = now;
			if (!page_requestor[0] && setpagerequest(0 /* For Talk */ ))
				return NA;
			else
				switch (uinfo.mode) {
				case TALK:
					move(line, 0);
					sprintf(buf, "** %s ¥¿¦b©I¥s§A", page_requestor);
					printdash(buf);
					break;
				default:	/* chat */
					sprintf(buf, "** %s ¥¿¦b©I¥s§A", page_requestor);
					printchatline(buf);
				}
		}
	}
	return YEA;
}

int
talkreply()
{
	int     a;
	struct hostent *h;
	char    buf[512];
	char    reason[51];
	char    hostname[STRLEN];
	struct sockaddr_in sin;
	char    inbuf[STRLEN * 2];
	talkrequest = NA;
	if (setpagerequest(0 /* For Talk */ ))
		return 0;
#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	clear();

	move(5, 0);
	clrtobot();
	show_user_plan(page_requestorid);
	move(1, 0);
	prints("(A)¡i%s¡j(B)¡i%s¡j\n", refuse[0], refuse[1]);
	prints("(C)¡i%s¡j(D)¡i%s¡j\n", refuse[2], refuse[3]);
	prints("(E)¡i%s¡j(F)¡i%s¡j\n", refuse[4], refuse[5]);
	prints("(G)¡i%s¡j(N)¡i%s¡j\n", refuse[6], refuse[7]);
	prints("(M)¡i¯d¨¥µ¹ %-13s            ¡j\n", page_requestorid);
	sprintf(inbuf, "§A·Q¸ò %s ²á²á¤Ñ¶Ü? (Y N A B C D E F G M)[Y]: ", page_requestor);
	strcpy(save_page_requestor, page_requestor);
#ifdef TALK_LOG
	strcpy(partner, page_requestorid);
#endif
	memset(page_requestor, 0, sizeof(page_requestor));
	memset(page_requestorid, 0, sizeof(page_requestorid));
	getdata(0, 0, inbuf, buf, 2, DOECHO, YEA);
	gethostname(hostname, STRLEN);
	if (!(h = gethostbyname(hostname))) {
		perror("gethostbyname");
		return -1;
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = h->h_addrtype;
	memcpy(&sin.sin_addr, h->h_addr, h->h_length);
	sin.sin_port = ui.sockaddr;
	a = socket(sin.sin_family, SOCK_STREAM, 0);
	if ((connect(a, (struct sockaddr *) & sin, sizeof(sin)))) {
		perror("connect err");
		return -1;
	}
	if (buf[0] != 'A' && buf[0] != 'a' && buf[0] != 'B' && buf[0] != 'b'
		&& buf[0] != 'C' && buf[0] != 'c' && buf[0] != 'D' && buf[0] != 'd'
		&& buf[0] != 'e' && buf[0] != 'E' && buf[0] != 'f' && buf[0] != 'F'
		&& buf[0] != 'g' && buf[0] != 'G' && buf[0] != 'n' && buf[0] != 'N'
		&& buf[0] != 'm' && buf[0] != 'M')
		buf[0] = 'y';
	if (buf[0] == 'M' || buf[0] == 'm') {
		move(1, 0);
		clrtobot();
		getdata(1, 0, "¯d¸Ü¡G", reason, 50, DOECHO, YEA);
	}
	write(a, buf, 1);
	if (buf[0] == 'M' || buf[0] == 'm')
		write(a, reason, sizeof(reason));
	if (buf[0] != 'y') {
		close(a);
		report("page refused");
		clear();
		refresh();
		return 0;
	}
	report("page accepted");
	clear();
	do_talk(a);
	close(a);
	clear();
	refresh();
	return 0;
}

void
do_talk_nextline(twin)
struct talk_win *twin;
{

	twin->curln = twin->curln + 1;
	if (twin->curln > twin->eline)
		twin->curln = twin->sline;
	if (twin->curln != twin->eline) {
		move(twin->curln + 1, 0);
		clrtoeol();
	}
	move(twin->curln, 0);
	clrtoeol();
	twin->curcol = 0;
}

void
do_talk_char(twin, ch)
struct talk_win *twin;
int     ch;
{
	extern int dumb_term;
	if (isprint2(ch)) {
		if (twin->curcol < 79) {
			move(twin->curln, (twin->curcol)++);
			prints("%c", ch);
			return;
		}
		do_talk_nextline(twin);
		twin->curcol++;
		prints("%c", ch);
		return;
	}
	switch (ch) {
	case Ctrl('H'):
	case '\177':
		if (dumb_term)
			ochar(Ctrl('H'));
		if (twin->curcol == 0) {
			return;
		}
		(twin->curcol)--;
		move(twin->curln, twin->curcol);
		if (!dumb_term)
			prints(" ");
		move(twin->curln, twin->curcol);
		return;
	case Ctrl('M'):
	case Ctrl('J'):
		if (dumb_term)
			prints("\n");
		do_talk_nextline(twin);
		return;
	case Ctrl('G'):
		bell();
		return;
	default:
		break;
	}
	return;
}

void
do_talk_string(twin, str)
struct talk_win *twin;
char   *str;
{
	while (*str) {
		do_talk_char(twin, *str++);
	}
}

char    talkobuf[80];
int     talkobuflen;
int     talkflushfd;

void
talkflush()
{
	if (talkobuflen)
		write(talkflushfd, talkobuf, talkobuflen);
	talkobuflen = 0;
}

int
moveto(mode, twin)
int     mode;
struct talk_win *twin;
{
	if (mode == 1)
		twin->curln--;
	if (mode == 2)
		twin->curln++;
	if (mode == 3)
		twin->curcol++;
	if (mode == 4)
		twin->curcol--;
	if (twin->curcol < 0) {
		twin->curln--;
		twin->curcol = 0;
	} else if (twin->curcol > 79) {
		twin->curln++;
		twin->curcol = 0;
	}
	if (twin->curln < twin->sline) {
		twin->curln = twin->eline;
	}
	if (twin->curln > twin->eline) {
		twin->curln = twin->sline;
	}
	move(twin->curln, twin->curcol);
}

void
endmsg()
{
	int     x, y;
	int     tmpansi;
	tmpansi = showansi;
	showansi = 1;
	talkidletime += 60;
	if (talkidletime >= IDLE_TIMEOUT)
		kill(getpid(), SIGHUP);
	if (uinfo.in_chat == YEA)
		return;
	getyx(&x, &y);
	update_endline();
	signal(SIGALRM, endmsg);
	move(x, y);
	refresh();
	alarm(60);
	showansi = tmpansi;
	return;
}

int
do_talk(int fd)
{
	struct talk_win mywin, itswin;
	char    mid_line[256];
	int     page_pending = NA;
	int     i, i2;
	int     previous_mode;
#ifdef TALK_LOG
	char    mywords[80], itswords[80], buf[80];
	int     mlen = 0, ilen = 0;
	time_t  now;
	mywords[0] = itswords[0] = '\0';
#endif

	signal(SIGALRM, SIG_IGN);
	endmsg();
	refresh();
	previous_mode = uinfo.mode;
	modify_user_mode(TALK);
	sprintf(mid_line, " %s (%s) ©M %s ¥¿¦bºZ½Í¤¤",
		currentuser.userid, currentuser.username, save_page_requestor);

	memset(&mywin, 0, sizeof(mywin));
	memset(&itswin, 0, sizeof(itswin));
	i = (t_lines - 1) / 2;
	mywin.eline = i - 1;
	itswin.curln = itswin.sline = i + 1;
	itswin.eline = t_lines - 2;
	move(i, 0);
	printdash(mid_line);
	move(0, 0);

	talkobuflen = 0;
	talkflushfd = fd;
	add_io(fd, 0);
	add_flush(talkflush);

	while (YEA) {
		int     ch;
		if (talkrequest)
			page_pending = YEA;
		if (page_pending)
			page_pending = servicepage((t_lines - 1) / 2, mid_line);
		ch = igetkey();
		talkidletime = 0;
		if (ch == '') {
			igetkey();
			igetkey();
			continue;
		}
		if (ch == I_OTHERDATA) {
			char    data[80];
			int     datac;
			register int i;
			datac = read(fd, data, 80);
			if (datac <= 0)
				break;
			for (i = 0; i < datac; i++) {
				if (data[i] >= 1 && data[i] <= 4) {
					moveto(data[i] - '\0', &itswin);
					continue;
				}
#ifdef TALK_LOG
				/*
				 * Sonny.990514 add an robust and fix some
				 * logic problem
				 */
				/*
				 * Sonny.990606 change to different algorithm
				 * and fix the
				 */
				/* existing do_log() overflow problem       */
				else if (isprint2(data[i])) {
					if (ilen >= 80) {
						itswords[80] = '\0';
						(void) do_log(itswords, 2);
						ilen = 0;
					} else {
						itswords[ilen] = data[i];
						ilen++;
					}
				} else if ((data[i] == Ctrl('H') || data[i] == '\177') && !ilen) {
					itswords[ilen--] = '\0';
				} else if (data[i] == Ctrl('M') || data[i] == '\r' ||
					data[i] == '\n') {
					itswords[ilen] = '\0';
					(void) do_log(itswords, 2);
					ilen = 0;
				}
#endif
				do_talk_char(&itswin, data[i]);
			}
		} else {
			if (ch == Ctrl('D') || ch == Ctrl('C'))
				break;
			if (isprint2(ch) || ch == Ctrl('H') || ch == '\177'
				|| ch == Ctrl('G')) {
				talkobuf[talkobuflen++] = ch;
				if (talkobuflen == 80)
					talkflush();
#ifdef TALK_LOG
				if (mlen < 80) {
					if ((ch == Ctrl('H') || ch == '\177') && mlen != 0) {
						mywords[mlen--] = '\0';
					} else {
						mywords[mlen] = ch;
						mlen++;
					}
				} else if (mlen >= 80) {
					mywords[80] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
#endif
				do_talk_char(&mywin, ch);
			} else if (ch == '\n' || ch == Ctrl('M') || ch == '\r') {
#ifdef TALK_LOG
				if (mywords[0] != '\0') {
					mywords[mlen++] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
#endif
				talkobuf[talkobuflen++] = '\r';
				talkflush();
				do_talk_char(&mywin, '\r');
			} else if (ch >= KEY_UP && ch <= KEY_LEFT) {
				moveto(ch - KEY_UP + 1, &mywin);
				talkobuf[talkobuflen++] = ch - KEY_UP + 1;
				if (talkobuflen == 80)
					talkflush();
			} else if (ch == Ctrl('E')) {
				for (i2 = 0; i2 <= 10; i2++) {
					talkobuf[talkobuflen++] = '\r';
					talkflush();
					do_talk_char(&mywin, '\r');
				}
			} else if (ch == Ctrl('P') && HAS_PERM(PERM_BASIC)) {
				t_pager();
				update_utmp();
				update_endline();
			}
		}
	}
	add_io(0, 0);
	talkflush();
	signal(SIGALRM, SIG_IGN);
	add_flush(NULL);
	modify_user_mode(previous_mode);

#ifdef TALK_LOG
	/* edwardc.990106 ²á¤Ñ¬ö¿ý */
	mywords[mlen] = '\0';
	itswords[ilen] = '\0';

	if (mywords[0] != '\0')
		do_log(mywords, 1);
	if (itswords[0] != '\0')
		do_log(itswords, 2);

	now = time(0);
	sprintf(buf, "\n[1;34m³q¸Üµ²§ô, ®É¶¡: %s [m\n", Cdate(&now));
	write(talkrec, buf, strlen(buf));

	close(talkrec);

	sethomefile(buf, currentuser.userid, "talklog");


	if ( dashf(buf) ) {
		getdata(23, 0, "¬O§_±H¦^²á¤Ñ¬ö¿ý [Y/n]: ", genbuf, 2, DOECHO, YEA);

		switch (genbuf[0]) {
			case 'n':
			case 'N':
				break;
			default:
				sprintf(mywords, "¸ò %s ªº²á¤Ñ°O¿ý [%s]", partner, Cdate(&now) + 5);
				mail_file(buf, currentuser.userid, mywords);
		}
	}
	sethomefile(buf, currentuser.userid, "talklog");
	unlink(buf);
	sethomefile(buf, currentuser.userid, "talklog");
	unlink(buf);
#endif

	return 0;
}

int
shortulist(uentp)
struct user_info *uentp;
{
	int     i;
	int     pageusers = 60;
	extern struct user_info *user_record[];
	extern int range;
	fill_userlist();
	if (ulistpage > ((range - 1) / pageusers))
		ulistpage = 0;
	if (ulistpage < 0)
		ulistpage = (range - 1) / pageusers;
	move(1, 0);
	clrtoeol();
	prints("¨C¹j [1;32m%d[m ¬í§ó·s¤@¦¸¡A[1;32mCtrl-C[m ©Î [1;32mCtrl-D[m Â÷¶}¡A[1;32mF[m §ó´«¼Ò¦¡ [1;32m¡ô¡õ[m ¤W¡B¤U¤@­¶ ²Ä[1;32m %1d[m ­¶", M_INT, ulistpage + 1);
	clrtoeol();
	move(3, 0);
	clrtobot();
	for (i = ulistpage * pageusers; i < (ulistpage + 1) * pageusers && i < range; i++) {
		char    ubuf[STRLEN];
		int     ovv;
		if (i < numf || friendmode)
			ovv = YEA;
		else
			ovv = NA;
		sprintf(ubuf, "%s%-12.12s %s%-10.10s[m", (ovv) ? "[1;32m¡D" : "  ", user_record[i]->userid, (user_record[i]->invisible == YEA) ? "[1;34m" : "",
			modestring(user_record[i]->mode, user_record[i]->destuid, 0, NULL));
		prints("%s", ubuf);
		if ((i + 1) % 3 == 0)
			outc('\n');
		else
			prints(" |");
	}
	return range;
}

int
do_list(modestr)
char   *modestr;
{
	char    buf[STRLEN];
	int     count;
	extern int RMSG;
	if (RMSG != YEA) {	/* ¦pªG¦¬¨ì Msg ²Ä¤@¦æ¤£Åã¥Ü¡C */
		move(0, 0);
		clrtoeol();
		if (chkmail())
			showtitle(modestr, "[±z¦³«H¥ó]");
		else
			showtitle(modestr, BoardName);
	}
	move(2, 0);
	clrtoeol();
	sprintf(buf, "  %-12s %-10s", "¨Ï¥ÎªÌ¥N¸¹", "¥Ø«e°ÊºA");
	prints("[1;33;44m%s |%s |%s[m", buf, buf, buf);
	count = shortulist();
	if (uinfo.mode == MONITOR) {
		time_t  thetime = time(0);
		move(t_lines - 1, 0);
		prints("[1;44;33m¥Ø«e¦³ [32m%3d[33m %6s¤W½u, ®É¶¡: [32m%s [33m, ¥Ø«eª¬ºA¡G[36m%10s   [m"
			,count, friendmode ? "¦nªB¤Í" : "¨Ï¥ÎªÌ", Ctime(&thetime), friendmode ? "§Aªº¦nªB¤Í" : "©Ò¦³¨Ï¥ÎªÌ");
	}
	refresh();
	return 0;
}

int
t_list()
{
	modify_user_mode(LUSERS);
	report("t_list");
	do_list("¨Ï¥ÎªÌª¬ºA");
	pressreturn();
	refresh();
	clear();
	return 0;
}


void
sig_catcher()
{
	ulistpage++;
	if (uinfo.mode != MONITOR) {
#ifdef DOTIMEOUT
		init_alarm();
#else
		signal(SIGALRM, SIG_IGN);
#endif
		return;
	}
	if (signal(SIGALRM, sig_catcher) == SIG_ERR) {
		perror("signal");
		exit(1);
	}
	do_list("±´µø¥Á±¡");
	alarm(M_INT);
}

int
t_monitor()
{
	int     i;
	char    modestr[] = "±´µø¥Á±¡";
	alarm(0);
	signal(SIGALRM, sig_catcher);
/*    idle_monitor_time = 0; */
	report("monitor");
	modify_user_mode(MONITOR);
	ulistpage = 0;
	do_list(modestr);
	alarm(M_INT);
	while (YEA) {
		i = egetch();
		if (i == 'f' || i == 'F') {
			if (friendmode == YEA)
				friendmode = NA;
			else
				friendmode = YEA;
			do_list(modestr);
		}
		if (i == KEY_DOWN) {
			ulistpage++;
			do_list(modestr);
		}
		if (i == KEY_UP) {
			ulistpage--;
			do_list(modestr);
		}
		if (i == Ctrl('D') || i == Ctrl('C') || i == KEY_LEFT)
			break;
/*        else if (i == -1) {
            if (errno != EINTR) { perror("read"); exit(1); }
        } else idle_monitor_time = 0;*/
	}
	move(2, 0);
	clrtoeol();
	clear();
	return 0;
}
/*Add by SmallPig*/
int
seek_in_file(filename, seekstr)
char    filename[STRLEN], seekstr[STRLEN];
{
	FILE   *fp;
	char    buf[STRLEN];
	char   *namep;
	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = (char *) strtok(buf, ": \n\r\t");
		if (namep != NULL && ci_strcmp(namep, seekstr) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

int
listfilecontent(fname)
char   *fname;
{
	FILE   *fp;
	int     x = 0, y = 3, cnt = 0, max = 0, len;
	char    u_buf[20], line[STRLEN], *nick;
	move(y, x);
	CreateNameList();
	strcpy(genbuf, fname);
	if ((fp = fopen(genbuf, "r")) == NULL) {
		prints("(none)\n");
		return 0;
	}
	while (fgets(genbuf, STRLEN, fp) != NULL) {
		strtok(genbuf, " \n\r\t");
		strncpy(u_buf, genbuf, 20);
		u_buf[19] = '\0';
		AddNameList(u_buf);
		nick = (char *) strtok(NULL, "\n\r\t");
		if (nick != NULL) {
			while (*nick == ' ')
				nick++;
			if (*nick == '\0')
				nick = NULL;
		}
		if (nick == NULL) {
			strcpy(line, u_buf);
		} else {
			sprintf(line, "%-12s%s", u_buf, nick);
		}
		if ((len = strlen(line)) > max)
			max = len;
		if (x + len > 78)
			line[78 - x] = '\0';
		prints("%s", line);
		cnt++;
		if ((++y) >= t_lines - 1) {
			y = 3;
			x += max + 2;
			max = 0;
			if (x > 70)
				break;
		}
		move(y, x);
	}
	fclose(fp);
	if (cnt == 0)
		prints("(none)\n");
	return cnt;
}

/* chinsan.20020108: ¾¨¶q§ï¥Î addtofile() ¨Ó¨ú¥N file_append()  
 * port from bbs.ilc.edu.tw
 */
int
addtofile(filename, str)
char filename[STRLEN], str[STRLEN];
{
	FILE *fp;
	int rc;

	if ((fp = fopen(filename, "a")) == NULL)
		return -1;
	flock(fileno(fp), LOCK_EX);
	rc = fprintf( fp, "%s\n", str);
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	return(rc == EOF ? -1 : 1);
}

int
addtooverride(uident)
char   *uident;
{
	struct override tmp;
	int     n;
	char    buf[STRLEN];
	char    desc[5];
	memset(&tmp, 0, sizeof(tmp));
	if (friendflag) {
		setuserfile(buf, "friends");
		n = MAXFRIENDS;
		strcpy(desc, "¦n¤Í");
	} else {
		setuserfile(buf, "rejects");
		n = MAXREJECTS;
		strcpy(desc, "Ãa¤H");
	}
	if (get_num_records(buf, sizeof(struct override)) >= n) {
		move(t_lines - 2, 0);
		clrtoeol();
		prints("©êºp¡A¥»¯¸¥Ø«e¶È¥i¥H³]©w %d ­Ó%s, ½Ð«ö¥ô¦ó¥óÄ~Äò...", n, desc);
		igetkey();
		move(t_lines - 2, 0);
		clrtoeol();
		return -1;
	} else {
		if (friendflag) {
			if (myfriend(searchuser(uident))) {
				sprintf(buf, "%s ¤w¦b¦n¤Í¦W³æ", uident);
				show_message(buf);
				return -1;
			}
		} else if (search_record(buf, &tmp, sizeof(tmp), cmpfnames, uident) > 0) {
			sprintf(buf, "%s ¤w¦bÃa¤H¦W³æ", uident);
			show_message(buf);
			return -1;
		}
	}
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND)
		n = 2;
	else
		n = t_lines - 2;

	strcpy(tmp.id, uident);
	move(n, 0);
	clrtoeol();
	refresh();
	sprintf(genbuf, "½Ð¿é¤Jµ¹%s¡i%s¡jªº»¡©ú: ", desc, tmp.id);
	getdata(n, 0, genbuf, tmp.exp, 40, DOECHO, YEA);

	n = append_record(buf, &tmp, sizeof(struct override));
	if (n != -1)
		(friendflag) ? getfriendstr() : getrejectstr();
	else
		report("append override error");
	return n;
}

int
del_from_file(filename, str)
char    filename[STRLEN], str[STRLEN];
{
	FILE   *fp, *nfp;
	int     deleted = NA;
	char    fnnew[STRLEN];
	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	sprintf(fnnew, "%s.%d", filename, getuid());
	if ((nfp = fopen(fnnew, "w")) == NULL)
		return -1;
	while (fgets(genbuf, STRLEN, fp) != NULL) {
		if (strncmp(genbuf, str, strlen(str)) == 0)
			deleted = YEA;
		else if (*genbuf > ' ')
			fputs(genbuf, nfp);
	}
	fclose(fp);
	fclose(nfp);
	if (!deleted)
		return -1;
	return (rename(fnnew, filename) + 1);
}


int
deleteoverride(uident, filename)
char   *uident;
char   *filename;
{
	int     deleted;
	struct override fh;
	char    buf[STRLEN];
	setuserfile(buf, filename);
	deleted = search_record(buf, &fh, sizeof(fh), cmpfnames, uident);
	if (deleted > 0) {
		if (delete_record(buf, sizeof(fh), deleted) != -1) {
			(friendflag) ? getfriendstr() : getrejectstr();
		} else {
			deleted = -1;
			report("delete override error");
		}
	}
	return (deleted > 0) ? 1 : -1;
}
override_title()
{
	char    desc[5];
	if (chkmail())
		strcpy(genbuf, "[±z¦³«H¥ó]");
	else
		strcpy(genbuf, BoardName);
	if (friendflag) {
		showtitle("[½s¿è¦n¤Í¦W³æ]", genbuf);
		strcpy(desc, "¦n¤Í");
	} else {
		showtitle("[½s¿èÃa¤H¦W³æ]", genbuf);
		strcpy(desc, "Ãa¤H");
	}
	prints(" [[1;32m¡ö[m,[1;32me[m] Â÷¶} [[1;32mh[m] ¨D§U [[1;32m¡÷[m,[1;32mRtn[m] %s»¡©úÀÉ [[1;32m¡ô[m,[1;32m¡õ[m] ¿ï¾Ü [[1;32ma[m] ¼W¥[%s [[1;32md[m] §R°£%s\n", desc, desc, desc);
	prints("[1;44m ½s¸¹  %s¥N¸¹      %s»¡©ú                                                   [m\n", desc, desc);
}

char   *
override_doentry(ent, fh)
int     ent;
struct override *fh;
{
	static char buf[STRLEN];
	sprintf(buf, " %4d  %-12.12s  %s", ent, fh->id, fh->exp);
	return buf;
}

int
override_edit(ent, fh, direc)
int     ent;
struct override *fh;
char   *direc;
{
	struct override nh;
	char    buf[STRLEN / 2];
	int     pos;
	pos = search_record(direc, &nh, sizeof(nh), cmpfnames, fh->id);
	move(t_lines - 2, 0);
	clrtoeol();
	if (pos > 0) {
		sprintf(buf, "½Ð¿é¤J %s ªº·s%s»¡©ú: ", fh->id, (friendflag) ? "¦n¤Í" : "Ãa¤H");
		getdata(t_lines - 2, 0, buf, nh.exp, 40, DOECHO, NA);
	}
	if (substitute_record(direc, &nh, sizeof(nh), pos) <= 0)
		report("Override files subs err");
	move(t_lines - 2, 0);
	clrtoeol();
	return NEWDIRECT;
}


int
override_add(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	char    uident[13];
	clear();
	move(1, 0);
	usercomplete("½Ð¿é¤J­n¼W¥[ªº¥N¸¹: ", uident);
	if (uident[0] != '\0' || uident[0] != ' ') {
		if (getuser(uident) <= 0) {
			move(2, 0);
			prints("¿ù»~ªº¨Ï¥ÎªÌ¥N¸¹...");
			pressanykey();
			return FULLUPDATE;
		} else
			addtooverride(uident);
	}
	prints("\n§â %s ¥[¤J%s¦W³æ¤¤...", uident, (friendflag) ? "¦n¤Í" : "Ãa¤H");
	pressanykey();
	return FULLUPDATE;
}

int
override_dele(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	char    buf[STRLEN];
	char    desc[5];
	char    fname[10];
	int     deleted = NA;
	if (friendflag) {
		strcpy(desc, "¦n¤Í");
		strcpy(fname, "friends");
	} else {
		strcpy(desc, "Ãa¤H");
		strcpy(fname, "rejects");
	}
	saveline(t_lines - 2, 0);
	move(t_lines - 2, 0);
	sprintf(buf, "¬O§_§â¡i%s¡j±q%s¦W³æ¤¤¥h°£", fh->id, desc);
	if (askyn(buf, NA, NA) == YEA) {
		move(t_lines - 2, 0);
		clrtoeol();
		if (deleteoverride(fh->id, fname) == 1) {
			prints("¤w±q%s¦W³æ¤¤²¾°£¡i%s¡j,«ö¥ô¦óÁäÄ~Äò...", desc, fh->id);
			deleted = YEA;
		} else
			prints("§ä¤£¨ì¡i%s¡j,«ö¥ô¦óÁäÄ~Äò...", fh->id);
	} else {
		move(t_lines - 2, 0);
		clrtoeol();
		prints("¨ú®ø§R°£%s...", desc);
	}
	igetkey();
	move(t_lines - 2, 0);
	clrtoeol();
	saveline(t_lines - 2, 1);
	return (deleted) ? PARTUPDATE : DONOTHING;
}

int
friend_edit(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = YEA;
	return override_edit(ent, fh, direct);
}

int
friend_add(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = YEA;
	return override_add(ent, fh, direct);
}

int
friend_dele(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = YEA;
	return override_dele(ent, fh, direct);
}

int
friend_mail(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	if (!HAS_PERM(PERM_POST))
		return DONOTHING;
	m_send(fh->id);
	return FULLUPDATE;
}

int
friend_query(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	int     ch;
	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[0;1;44;31m[Åª¨ú¦n¤Í»¡©úÀÉ][33m ±H«Hµ¹¦n¤Í m ¢x µ²§ô Q,¡ö ¢x¤W¤@¦ì ¡ô¢x¤U¤@¦ì <Space>,¡õ      [m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case 'm':
	case 'M':
		m_send(fh->id);
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

int
friend_help()
{
	show_help(F_HELP_FRIENDS);
	return FULLUPDATE;
}

int
reject_edit(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = NA;
	return override_edit(ent, fh, direct);
}

int
reject_add(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = NA;
	return override_add(ent, fh, direct);
}

int
reject_dele(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	friendflag = NA;
	return override_dele(ent, fh, direct);
}

int
reject_query(ent, fh, direct)
int     ent;
struct override *fh;
char   *direct;
{
	int     ch;
	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[0;1;44;31m[Åª¨úÃa¤H»¡©úÀÉ][33m µ²§ô Q,¡ö ¢x¤W¤@¦ì ¡ô¢x¤U¤@¦ì <Space>,¡õ                      [m");
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
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

int
reject_help()
{
	show_help(F_HELP_REJECTS);
	return FULLUPDATE;
}

void
t_friend()
{
	char    buf[STRLEN];
	friendflag = YEA;
	setuserfile(buf, "friends");
	i_read(GMENU, buf, override_title, override_doentry, friend_list, sizeof(struct override));
	clear();
	return;
}

void
t_reject()
{
	char    buf[STRLEN];
	friendflag = NA;
	setuserfile(buf, "rejects");
	i_read(GMENU, buf, override_title, override_doentry, reject_list, sizeof(struct override));
	clear();
	return;
}

struct user_info *
t_search(sid, pid)
char   *sid;
int     pid;
{
	int     i;
	extern struct UTMPFILE *utmpshm;
	struct user_info *cur, *tmp = NULL;
	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		cur = &(utmpshm->uinfo[i]);
		if (!cur->active || !cur->pid)
			continue;
		if (!strcasecmp(cur->userid, sid)) {
			if (pid == 0)
				return isreject(cur) ? NULL : cur;
			tmp = cur;
			if (pid == cur->pid)
				break;
		}
	}
	if (tmp != NULL) {
		if (tmp->invisible && !HAS_PERM(PERM_SEECLOAK | PERM_SYSOP))
			return NULL;
	}
	return isreject(cur) ? NULL : tmp;
}

int
cmpfuid(a, b)
unsigned short *a, *b;
{
	return *a - *b;
}

int
getfriendstr()
{
	int     i;
	struct override *tmp;
	memset(uinfo.friend, 0, sizeof(uinfo.friend));
	setuserfile(genbuf, "friends");
	uinfo.fnum = get_num_records(genbuf, sizeof(struct override));
	if (uinfo.fnum <= 0)
		return 0;
	uinfo.fnum = (uinfo.fnum >= MAXFRIENDS) ? MAXFRIENDS : uinfo.fnum;
	tmp = (struct override *) calloc(sizeof(struct override), uinfo.fnum);
	get_records(genbuf, tmp, sizeof(struct override), 1, uinfo.fnum);
	for (i = 0; i < uinfo.fnum; i++) {
		uinfo.friend[i] = searchuser(tmp[i].id);
		if (uinfo.friend[i] == 0)
			deleteoverride(tmp[i].id, "friends");
		/* ¶¶«K§R°£¤w¤£¦s¦b±b¸¹ªº¦n¤Í */
	}
	free(tmp);
	qsort(&uinfo.friend, uinfo.fnum, sizeof(uinfo.friend[0]), cmpfuid);
	update_ulist(&uinfo, utmpent);
}

int
getrejectstr()
{
	int     nr, i;
	struct override *tmp;
	memset(uinfo.reject, 0, sizeof(uinfo.reject));
	setuserfile(genbuf, "rejects");
	nr = get_num_records(genbuf, sizeof(struct override));
	if (nr <= 0)
		return 0;
	nr = (nr >= MAXREJECTS) ? MAXREJECTS : nr;
	tmp = (struct override *) calloc(sizeof(struct override), nr);
	get_records(genbuf, tmp, sizeof(struct override), 1, nr);
	for (i = 0; i < nr; i++) {
		uinfo.reject[i] = searchuser(tmp[i].id);
		if (uinfo.reject[i] == 0)
			deleteoverride(tmp[i].id, "rejects");
	}
	free(tmp);
	update_ulist(&uinfo, utmpent);
}

int
wait_friend()
{
	FILE   *fp;
	int     tuid;
	char    buf[STRLEN];
	char    uid[13];
	modify_user_mode(WFRIEND);
	clear();
	move(1, 0);
	usercomplete("½Ð¿é¤J¨Ï¥ÎªÌ¥N¸¹¥H¥[¤J¨t²Îªº´M¤H¦W¥U: ", uid);
	if (uid[0] == '\0') {
		clear();
		return 0;
	}
	if (!(tuid = getuser(uid))) {
		move(2, 0);
		prints("[1m¤£¥¿½Tªº¨Ï¥ÎªÌ¥N¸¹[m\n");
		pressanykey();
		clear();
		return -1;
	}
	sprintf(buf, "§A½T©w­n§â [1m%s[m ¥[¤J¨t²Î´M¤H¦W³æ¤¤", uid);
	move(2, 0);
	if (askyn(buf, YEA, NA) == NA) {
		clear();
		return;
	}
	if ((fp = fopen("friendbook", "a")) == NULL) {
		prints("¨t²Îªº´M¤H¦W¥UµLªk¶}±Ò¡A½Ð³qª¾¯¸ªø...\n");
		pressanykey();
		return NA;
	}
	sprintf(buf, "%d@%s", tuid, currentuser.userid);
	if (!seek_in_file("friendbook", buf))
		fprintf(fp, "%s\n", buf);
	fclose(fp);
	move(3, 0);
	prints("¤w¸gÀ°§A¥[¤J´M¤H¦W¥U¤¤¡A[1m%s[m ¤W¯¸¨t²Î¤@©w·|³qª¾§A...\n", uid);
	pressanykey();
	clear();
	return;
}
#ifdef TALK_LOG
/* edwardc.990106 ¤À§O¬°¨â¦ì²á¤Ñªº¤H§@¬ö¿ý */
/* -=> ¦Û¤v»¡ªº¸Ü */
/* --> ¹ï¤è»¡ªº¸Ü */

void
do_log(char *msg, int who)
{
/* Sonny.990514 ¸ÕµÛ§ì overflow ªº°ÝÃD... */
/* Sonny.990606 overflow °ÝÃD¸Ñ¨M. buf[100] ¬O¥¿½Tªº. °Ñ¦Ò man sprintf() */
	time_t  now;
	char    buf[100];
	now = time(0);
	if (msg[strlen(msg)] == '\n')
		msg[strlen(msg)] = '\0';

	if (strlen(msg) < 1 || msg[0] == '\r' || msg[0] == '\n')
		return;

	/* ¥uÀ°¦Û¤v°µ */
	sethomefile(buf, currentuser.userid, "talklog");

	if (!dashf(buf) || talkrec == -1) {
		talkrec = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
		sprintf(buf, "[1;32m»P %s ªº±¡¸Üºøºø, ¤é´Á: %s [m\n", save_page_requestor, Cdate(&now));
		write(talkrec, buf, strlen(buf));
		sprintf(buf, "\tÃC¦â¤À§O¥Nªí: [1;33m%s[m [1;36m%s[m \n\n", currentuser.userid, partner);
		write(talkrec, buf, strlen(buf));
	}
	if (who == 1) {		/* ¦Û¤v»¡ªº¸Ü */
		sprintf(buf, "[1;33m-=> %s [m\n", msg);
		write(talkrec, buf, strlen(buf));
	} else if (who == 2) {	/* §O¤H»¡ªº¸Ü */
		sprintf(buf, "[1;36m--> %s [m\n", msg);
		write(talkrec, buf, strlen(buf));
	}
}
#endif
