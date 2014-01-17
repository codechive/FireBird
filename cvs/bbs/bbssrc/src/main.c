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
$Id: main.c,v 1.26 2002/09/13 01:50:34 edwardc Exp $
*/

#include "bbs.h"

#define DOTLOGINSBAD    "logins.bad"
#define BADLOGINFILE    "log/logins.bad"
int     ERROR_READ_SYSTEM_FILE = NA;
int     RMSG = YEA;
int     msg_num = 0;
int     nettyNN = 0;
int     count_friends = 0, count_users = 0;
int     iscolor = 1;
int     mailXX = 0;
char   *getenv();
int     friend_login_wall();
char   *sysconf_str();
char   *Ctime();
struct user_info *t_search();
void    r_msg();
void    count_msg();
void    c_recover();
void    tlog_recover();
int     listmode;
int     numofsig = 0;
jmp_buf byebye;
extern struct BCACHE *brdshm;
FILE   *ufp;
int     talkrequest = NA;
/* int ntalkrequest = NA ; */
int     enter_uflags;
time_t  lastnote;
extern int dumb_term;
int	ScrFlag = NA;
int	ISLOGIN = 1;

struct user_info uinfo;

char    netty_path[256];

#ifndef BBSD
char    tty_name[20];
#endif
char	fromhost[60];

char    BoardName[STRLEN];
char    ULIST[STRLEN];
int     utmpent = -1;
time_t  login_start_time;
int     showansi = 1;

void
log_usies(mode, mesg)
char   *mode, *mesg;
{
	time_t  now;
	char    buf[256], *fmt;
	now = time(0);
	fmt = currentuser.userid[0] ? "%s %s %-12s %s\n" : "%s %s %s%s\n";
	sprintf(buf, fmt, Ctime(&now), mode, currentuser.userid, mesg);
	file_append("usies", buf);
}

void
u_enter()
{
	char	buf[80], host[60], fhost[60];
	int	i, space;
	FILE	*fp;
	
	enter_uflags = currentuser.flags[0];
	memset(&uinfo, 0, sizeof(uinfo));
	uinfo.active = YEA;
	uinfo.pid = getpid();
	if (HAS_PERM(PERM_LOGINCLOAK) && (currentuser.flags[0] & CLOAK_FLAG))
		uinfo.invisible = YEA;
	uinfo.mode = LOGIN;
#ifdef BBSD
	uinfo.idle_time = time(0);
#endif

	if (strcmp(currentuser.userid, "guest")) {
		if (DEFINE(DEF_FRIENDCALL)) {
			uinfo.pager |= FRIEND_PAGER;
		}
		if (currentuser.flags[0] & PAGER_FLAG) {
			uinfo.pager |= ALL_PAGER;
			uinfo.pager |= FRIEND_PAGER;
		}

		if (DEFINE(DEF_FRIENDMSG)) {
			uinfo.pager |= FRIENDMSG_PAGER;
		}
		if (DEFINE(DEF_ALLMSG)) {
			uinfo.pager |= ALLMSG_PAGER;
			uinfo.pager |= FRIENDMSG_PAGER;
		}
	}
	
	uinfo.uid = usernum;
	strncpy(uinfo.from, fromhost, 60);

#ifdef FROM_TRANSLATE

	/* chinsan.20011231: ¤¤¤å¤W¯¸¨Ó·½¹ï·Ó port from bbs.ilc.edu.tw */
	strncpy(fhost, fromhost, 60);
	i = 0;
	while (i < strlen(fhost)) {
		fhost[i]= tolower(fhost[i]);
		i++;
	}
	i = 0;
	if ( (fp = fopen("etc/hosts","r"))!= NULL ) {
		while ( fgets(buf, 80, fp) ) {
			space = 0;
			while (buf[space] != ' ' && space < strlen(buf)) {
				space++;
			}
			memset(host, 0 , 60);
			strncpy(host, buf, space);
			/* chinsan.20010112: convert host to lower case (by kllau) */
			while (i < strlen(host)) {
				host[i] = tolower(host[i]);
				i++;
			}
			i = 0;
			if (strstr(fhost,host)) {
				while (buf[space] == ' ' && space < strlen(buf)) {
					space++;
				}
				memset(uinfo.from, 0, 60);
				strncpy(uinfo.from, buf+space, strlen(buf) - space -1 );
				break;
			}
		}
		fclose(fp);
	}

#endif

#if !defined(BBSD) && defined(SHOW_IDLE_TIME)
	strncpy(uinfo.tty, tty_name, 20);
#endif	
	iscolor = (DEFINE(DEF_COLOR)) ? 1 : 0;
	strncpy(uinfo.userid, currentuser.userid, 20);
	strncpy(uinfo.realname, currentuser.realname, 20);
	strncpy(uinfo.username, currentuser.username, 40);
	getfriendstr();
	getrejectstr();
	if (HAS_PERM(PERM_EXT_IDLE))
		uinfo.ext_idle = YEA;
		
	listmode = 0;	/* ­É¥Î¤@¤U, ¥Î¨Ó¬ö¿ý¨ì©³ utmpent ¥d¦ì¥¢±Ñ´X¦¸ */
	
	while ( 1 ) {
		utmpent = getnewutmpent(&uinfo);
		if ( utmpent >= 0 || utmpent == -1 )
			break;
		if ( utmpent == -2 && listmode <= 100 ) {
			listmode++;
			usleep(250);		/* ¥ð®§¥|¤À¤§¤@¬í¦A±µ¦bÀy */
			continue;	
		}
		if ( listmode > 100 ) {	/* ©ñ±ó§a */
			sprintf(genbuf, "getnewutmpent(): too much times, give up.");
			report(genbuf);
			prints("getnewutmpent(): ¥¢±Ñ¤Ó¦h¦¸, ©ñ±ó. ½Ð¦^³ø¯¸ªø.\n");
			sleep(3);
			exit(0);
		}
	}
	
	if (utmpent < 0) {
		sprintf(genbuf, "Fault: No utmpent slot for %s\n", uinfo.userid);
		report(genbuf);
	}
	listmode = 0;
	digestmode = NA;
}

void
setflags(mask, value)
int     mask, value;
{
	if (((currentuser.flags[0] & mask) && 1) != value) {
		if (value)
			currentuser.flags[0] |= mask;
		else
			currentuser.flags[0] &= ~mask;
	}
}

void
u_exit()
{
	setflags(PAGER_FLAG, (uinfo.pager & ALL_PAGER));
	if (HAS_PERM(PERM_LOGINCLOAK))
		setflags(CLOAK_FLAG, uinfo.invisible);

	if (currentuser.flags[0] != enter_uflags && !ERROR_READ_SYSTEM_FILE) {
		set_safe_record();
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}
	uinfo.active = NA;
	uinfo.pid = 0;
	uinfo.invisible = YEA;
	uinfo.sockactive = NA;
	uinfo.sockaddr = 0;
	uinfo.destuid = 0;
#if !defined(BBSD) && defined(SHOW_IDLE_TIME)
	strcpy(uinfo.tty, "NoTTY");
#endif
	update_utmp();
}

int
cmpuids(uid, up)
char   *uid;
struct userec *up;
{
	return !ci_strncmp(uid, up->userid, sizeof(up->userid));
}

int
dosearchuser(userid)
char   *userid;
{
	int     id;
	if ((id = getuser(userid)) != 0) {
		if (cmpuids(userid, &lookupuser)) {
			memcpy(&currentuser, &lookupuser, sizeof(currentuser));
			return usernum = id;
		}
	}
	memset(&currentuser, 0, sizeof(currentuser));
	return usernum = 0;
}

int     started = 0;

void
talk_request()
{
	signal(SIGUSR1, talk_request);
	talkrequest = YEA;
	bell();
	bell();
	bell();
	sleep(1);
	bell();
	bell();
	bell();
	bell();
	bell();
	return;
}

void
abort_bbs()
{
	time_t  stay;


	if (uinfo.mode == POSTING || uinfo.mode == SMAIL || uinfo.mode == EDIT
		|| uinfo.mode == EDITUFILE || uinfo.mode == EDITSFILE || uinfo.mode == EDITANN)
		keep_fail_post();
	if (started) {
		stay = time(0) - login_start_time;
		sprintf(genbuf, "Stay: %3ld (%s)", stay / 60, currentuser.username);
		log_usies("AXXED", genbuf);
		u_exit();
	}
	shm_deatch();
	exit(0);
}

int
cmpuids2(unum, urec)
int     unum;
struct user_info *urec;
{
	return (unum == urec->uid);
}

int
count_multi(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     num = count;
		count = 0;
		return num;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (uentp->uid == usernum)
		count++;
	return 1;
}

int
count_user()
{
	count_multi(NULL);
	apply_ulist(count_multi);
	return count_multi(NULL);
}

void
multi_user_check()
{
	struct user_info uin;
	char    buffer[40];
	int     logins;
	

	if ( HAS_PERM(PERM_SYSOP) && !trusted_host(fromhost) ) {
		sprintf(genbuf,"¥¼¨ü¤¹³\\ªº¥D¾÷ %s ¨Ï¥Î SYSOP Åv­­±b¸¹",fromhost);
		securityreport(genbuf);
		prints("«Ü©êºp!! ¾Ö¦³¯¸ªøÅv­­ªº¤H½Ð±q³Q¤¹³\\ªº¥D¾÷³s½u......\n");
		pressreturn();
		oflush();
		exit(1);
	}
                                        
	if (HAS_PERM(PERM_MULTILOG))
		return;		/* don't check sysops */

	/* allow multiple guest user */
	logins = count_user();

	if (heavyload() && logins) {
		prints("[1;33m©êºp, ¥Ø«e¨t²Î­t²ü¹L­«, ½Ð¤Å­«ÂÐ Login¡C[m\n");
		oflush();
		sleep(3);
		exit(1);
	}
	if (!strcasecmp("guest", currentuser.userid)) {
		if (logins > MAXGUEST) {
			prints("[1;33m©êºp, ¥Ø«e¤w¦³¤Ó¦h [1;36mguest[33m, ½Ðµy«á¦A¸Õ¡C[m\n");
			oflush();
			sleep(3);
			exit(1);
		}
		return;
	} else if (logins >= MULTI_LOGINS) {
		prints("[1;33m«Ü©êºp, ±z¤w Login ¬Û¦P±b¸¹ %d ¦¸, ¬°½T«O¥L¤H¤W¯¸Åv¯q,\n ¦¹³s½u±N³Q¨ú®ø¡C[m\n", MULTI_LOGINS);
		oflush();
		sleep(3);
		exit(1);
	}
	if (!search_ulist(&uin, cmpuids2, usernum))
		return;		/* user isn't logged in */

	if (!uin.active || (kill(uin.pid, 0) == -1))
		return;		/* stale entry in utmp file */
	if (askyn("[1;37m±z·Q§R°£­«½Æªº login ¶Ü", NA, YEA) == YEA) {
		kill(uin.pid, 9);
		sprintf(buffer, "kicked (multi-login)");
		report(buffer);
		log_usies("KICK ", currentuser.username);
	}
}

int
simplepasswd(str, check)
char   *str;
int     check;
{
	char    ch;
	while ((ch = *str++) != '\0') {
		if (check == 1) {
			if (!(ch >= 'a' && ch <= 'z'))
				return 0;
		} else if (!(ch >= '0' && ch <= '9'))
			return 0;
	}
	return 1;
}
#ifndef BBSD
void
system_init(argc, argv)
int     argc;
char  **argv;
#else
void
system_init()
#endif
{
#ifndef BBSD
	char   *rhost;
#endif

	gethostname(genbuf, 256);
#ifdef SINGLE
	if (strcmp(genbuf, SINGLE)) {
		printf("Not on a valid machine!\n");
		exit(-1);
	}
#endif
	sprintf(ULIST, "%s.%s", ULIST_BASE, genbuf);

#ifndef BBSD
	if (argc >= 3) {
		strncpy(fromhost, argv[2], 60);
	} else {
		fromhost[0] = '\0';
	}
	if ((rhost = getenv("REMOTEHOST")) != NULL)
		strncpy(fromhost, rhost, 60);
	fromhost[59] = '\0';
#if defined(SHOW_IDLE_TIME)
	if (argc >= 4) {
		strncpy(tty_name, argv[3], 20);
	} else {
		tty_name[0] = '\0';
	}
#endif
#endif

#ifdef DOTIMEOUT
	init_alarm();
	uinfo.mode = LOGIN;
	alarm(LOGIN_TIMEOUT);
#else
	signal(SIGALRM, SIG_SIG);
#endif

	signal(SIGHUP, abort_bbs);
	signal(SIGBUS, abort_bbs);
	signal(SIGSEGV, abort_bbs);
	signal(SIGTERM, abort_bbs);

	signal(SIGPIPE, abort_bbs);
	signal(SIGTTOU, count_msg);
	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, r_msg);
}

void
system_abort()
{
	if (started) {
		log_usies("ABORT", currentuser.username);
		u_exit();
	}
	clear();
	refresh();
	printf("ÁÂÁÂ¥úÁ{, °O±o±`¨Ó³á !\n");
	exit(0);
}

void
logattempt(uid, frm)
char   *uid, *frm;
{
	char    fname[STRLEN];
	
	login_start_time = time(0);
	sprintf(genbuf, "%-12.12s  %-30s %s\n",
		uid, Ctime(&login_start_time), frm);
	file_append(BADLOGINFILE, genbuf);
	sethomefile(fname, uid, DOTLOGINSBAD);
	file_append(fname, genbuf);
}

void
login_query()
{
	char    uid[IDLEN + 2], passbuf[PASSLEN];
	int     curr_login_num;
	int     attempts;
#ifdef MAGIC_PASS
	int     magic;
	char    buf[PASSLEN];
#endif
	char    genbuf[STRLEN];
#ifndef BBSNAME
	char   *ptr;
#endif
	extern struct UTMPFILE *utmpshm;


	dumb_term = YEA;
	
	curr_login_num = num_active_users();
	if (curr_login_num >= MAXACTIVE) {
		ansimore("etc/loginfull", NA);
		oflush();
		sleep(1);
		exit(1);
	}
#ifdef BBSNAME
	strcpy(BoardName, BBSNAME);
	/* edwardc.990508 copy from defined variable */
#else
	ptr = sysconf_str("BBSNAME");
	if (ptr == NULL)
		ptr = "©|¥¼©R¦W´ú¸Õ¯¸";
	strcpy(BoardName, ptr);
#endif

	if (fill_shmfile(1, "etc/issue", "ISSUE_SHMKEY")) {
		show_issue();	/* is anibanner ready, remark this and put
				 * \n\n */
	}
	prints("[1;35mÅwªï¥úÁ{[1;40;33m¡i %s ¡j[0;1;32m¡C ¥»¯¸¤@¦@¥i¥HÅý [1;36m%d[0;1;32m ¤Hµù¥U¨Ï¥Î¡C[m\n",
		BoardName, MAXUSERS);
	resolve_utmp();
	if (utmpshm->usersum == 0)
		utmpshm->usersum = allusers();
	if (utmpshm->max_login_num < curr_login_num)
		utmpshm->max_login_num = curr_login_num;

#ifndef VERSION_ID
	prints("[1;36m¥Ø«e¤W¯¸¤H¼Æ:[1;40;37m [%d/%d] [0;1;32m¡C ³Ì°ª¤H¼Æ°O¿ý: [[1;36m%d[1;32m]¡C ¥Ø«e¤w¦³ [1;36m%d[32m ­Óµù¥U±b¸¹¡C[m\n",
		curr_login_num, MAXACTIVE, utmpshm->max_login_num, utmpshm->usersum);
#else
	prints("[1;36m¥Ø«e¤W¯¸¤H¼Æ:[1;40;37m [%d/%d] [0;1;32m¡C ¥Ø«e¤w¦³ [1;36m%d[32m ­Óµù¥U±b¸¹¡C ¡e%s¡f[m\n",
		curr_login_num, MAXACTIVE, utmpshm->usersum, VERSION_ID);
#endif

#ifdef MUDCHECK_BEFORELOGIN

	prints("[1;33m¬°¨¾¤î¨Ï¥Îµ{¦¡¤W¯¸¡A½Ð«ö [1;36mCTRL + C[m : ");
	
	genbuf[0] = igetkey();
	
	if ( genbuf[0] != Ctrl('C') ) {
		prints("\n¹ï¤£°_¡A§A¨Ã¨S¦³«ö¤U CTRL+C Áä¡I\n");
		oflush();
		exit(1);
	} else {
		prints("[CTRL] + [C]\n");
	}

#endif
	
	attempts = 0;
	while (1) {
		if (attempts++ >= LOGINATTEMPTS) {
			ansimore("etc/goodbye", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		getdata(0, 0, "[1;33m½Ð¿é¤J±b¸¹[m(¸Õ¥Î½Ð¿é¤J `[1;36mguest[m', µù¥U½Ð¿é¤J`[1;31mnew[m'): ",
			uid, IDLEN + 1, DOECHO, YEA);
#ifdef Quick_LOGIN
		if( uid[0] == 0 )
			strcpy(uid, "guest");
#endif			
		/* ppfoong */
		if ((strcasecmp(uid, "guest") == 0) && (MAXACTIVE - curr_login_num < 10)) {
			ansimore("etc/loginfull", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		if (strcasecmp(uid, "new") == 0) {
#ifdef LOGINASNEW
			memset(&currentuser, 0, sizeof(currentuser));
			new_register();
			ansimore3("etc/firstlogin", YEA);
			break;
#else
			prints("[1;37m¥»¨t²Î¥Ø«eµLªk¥H [36mnew[37m µù¥U, ½Ð¥Î[36m guest[37m ¶i¤J...[m\n");
#endif
		} else if (*uid == '\0' || !dosearchuser(uid)) {
			prints("[1;31m«u§r¡A%s¨S¦³³o­Ó¤H­C...[m\n\n", BoardName);
		} else if (strcasecmp(uid, "guest") == 0) {
			currentuser.userlevel = 0;
			currentuser.flags[0] = CURSOR_FLAG;
#ifdef	_FreeBSD_
			setproctitle("user: guest (%s)", fromhost);
#endif
			break;
		} else {
			getdata(0, 0, "[1;37m½Ð¿é¤J±K½X: [m", passbuf, PASSLEN, NOECHO, YEA);
			passbuf[8] = '\0';
			if (!checkpasswd(currentuser.passwd, passbuf)) {
				logattempt(currentuser.userid, fromhost);
				prints("[1;31m­ü§r¡A±z«ç»ò§â±K½Xµ¹§Ñ¤F...[m\n\n");
			} else {
/* °±Åv¡]Åv­­¸ò guest ¤@¼Ë¡^¨ÌµM¥i¶i¤J modify by skyo 19990602*/
				if (simplepasswd(passbuf, 1) || simplepasswd(passbuf, 2)
					|| strstr(passbuf, currentuser.userid)) {
					prints("[1;33m* ±K½X¹L©óÂ²³æ, ½Ð¿ï¾Ü¤@­Ó¥H¤Wªº¯S®í¦r¤¸.[m\n");
					getdata(0, 0, "«ö <ENTER> Ä~Äò", genbuf, 5, NOECHO, YEA);
				}
#ifdef REFUSE_LESS60SEC
				/* edwardc.990420 committe by Harimau */
				if (time(0) - currentuser.lastlogin < 60 &&
					!HAS_PERM(PERM_SYSOP) &&
					strcasecmp(currentuser.userid, "guest") != 0) {
					prints("¬°¤F¨¾¤î¤£¥¿±`ªº¼W¥[¨t²Î­t²ü, ½Ð©ó¤@¤ÀÄÁ«á¦A¤W¯¸.\n");
					prints("­Y¦³ºÃ°Ý½Ð³qª¾¯¸°È¤H­û, ÁÂÁÂ.\n");
					oflush();
					sleep(3);
					exit(1);
				}
#endif

#ifdef MAGIC_PASS
				if (HAS_PERM(PERM_SYSOP)) {
					randomize();
					magic = rand() % 100;
					prints("¯S®í±b¸¹Ã±¤J¤f¥O: %d\n", magic * 4);
					getdata(4, 0, "¦^À³¤f¥O : ", buf, PASSLEN, NOECHO, YEA);
					if (*buf == '\0' || !(atoi(buf) == magic)) {
						sprintf(genbuf, "Ã±¤J¤f¥O¿ù»~ (%s)", fromhost);
						securityreport(genbuf);
						prints("¤f¥O¿ù»~, ¤£±oÃ±¤J ! !\n");
						oflush();
						sleep(2);
						exit(1);
					}
				}
#endif
				bzero(passbuf, PASSLEN - 1);
#ifdef	_FreeBSD_
				/* edwardc.010831 ¤£½T©w­n¤£­n©ñ³o»ò¤U­± */
				setproctitle("user: %s", currentuser.userid);
#endif
				break;
			}
		}
	}
	ISLOGIN = 0;
	multi_user_check();

#if 1
   if (!term_init(currentuser.termtype)) {
		prints("Bad terminal type.  Defaulting to 'vt100'\n");
		strcpy(currentuser.termtype, "vt100");
		term_init(currentuser.termtype);
	}
#endif

	sethomepath(genbuf, currentuser.userid);
	mkdir(genbuf, 0755);
	dumb_term = NA;
}

int
valid_ident(ident)
char   *ident;
{
	static char *invalid[] = {"unknown@", "root@", "gopher@", "bbs@",
	"bbsadm@", "guest@", "nobody@", "www@", "?@", NULL};
	int     i;
	if (ident[0] == '@')
		return 0;
	for (i = 0; invalid[i] != NULL; i++)
		if (strstr(ident, invalid[i]) != NULL)
			return 0;
	return 1;
}

void
write_defnotepad()
{
	currentuser.notedate = time(NULL);
	set_safe_record();
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	return;
}

void
notepad_init()
{
	FILE   *check;
	char    notetitle[STRLEN];
	char    tmp[STRLEN * 2];
	char   *fname, *bname, *ntitle;
	long int maxsec;
	time_t  now;
	maxsec = 86400;
	lastnote = 0;
	if ((check = fopen("etc/checknotepad", "r")) != NULL) {
		fgets(tmp, sizeof(tmp), check);
		lastnote = atol(tmp);
		fclose(check);
	} else
		lastnote = 0;
	if (lastnote == 0) {
		lastnote = time(NULL) - (time(NULL) % maxsec);
		check = fopen("etc/checknotepad", "w");
		fprintf(check, "%d", lastnote);
		fclose(check);
		sprintf(tmp, "¯d¨¥ªO¦b %s Login ¶}±Ò¡A¤º©w¶}±Ò®É¶¡¬° %s"
			,currentuser.userid, Ctime(&lastnote));
		report(tmp);
	}
	if ((time(NULL) - lastnote) >= maxsec) {
		move(t_lines - 1, 0);
		prints("¹ï¤£°_¡A¨t²Î¦Û°Êµo«H¡A½Ðµy­Ô.....");
		refresh();
		now = time(0);
		check = fopen("etc/checknotepad", "w");
		lastnote = time(NULL) - (time(NULL) % maxsec);
		fprintf(check, "%d", lastnote);
		fclose(check);
		if ((check = fopen("etc/autopost", "r")) != NULL) {
			while (fgets(tmp, STRLEN, check) != NULL) {
				fname = strtok(tmp, " \n\t:@");
				bname = strtok(NULL, " \n\t:@");
				ntitle = strtok(NULL, " \n\t:@");
				if (fname == NULL || bname == NULL || ntitle == NULL)
					continue;
				else {
					sprintf(notetitle, "%.10s¡n%s", Ctime(&now) + 11, ntitle);
					if (dashf(fname)) {
						postfile(fname, bname, notetitle, 1);
						sprintf(tmp, "%s ¦Û°Ê±i¶K", ntitle);
						report(tmp);
					}
				}
			}
			fclose(check);
		}
		sprintf(notetitle, "%.15s ¯d¨¥ªO°O¿ý", Ctime(&now));
		if (dashf("etc/notepad", "r")) {
			postfile("etc/notepad", "notepad", notetitle, 1);
			unlink("etc/notepad");
		}
		report("¦Û°Êµo«H®É¶¡§ó§ï");
	}
	return;
}


void
user_login()
{
	char    fname[STRLEN];
	char   *ruser;

	if (strcmp(currentuser.userid, "SYSOP") == 0) {
		currentuser.userlevel = ~0;	/* SYSOP gets all permission
						 * bits */
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}
	ruser = getenv("REMOTEUSER");
	sprintf(genbuf, "%s@%s", ruser ? ruser : "?", fromhost);
	log_usies("ENTER", genbuf);
	u_enter();
	if (ruser != NULL) {
		sprintf(genbuf, "%s@%s", ruser, fromhost);
		if (valid_ident(genbuf)) {
			strncpy(currentuser.ident, genbuf, NAMELEN);
			currentuser.ident[NAMELEN - 1] = '\0';
		}
	}
	sprintf(genbuf,"Enter - %s",fromhost);
	report(genbuf);
	started = 1;
	initscr();
	scrint = 1;
	if (USE_NOTEPAD == 1)
		notepad_init();
	if (strcasecmp(currentuser.userid, "guest") != 0 && USE_NOTEPAD == 1) {
		if (DEFINE(DEF_NOTEPAD)&& currentuser.numlogins != 0) {
			int     noteln;
			if (lastnote > currentuser.notedate)
				currentuser.noteline = 0;
			noteln = countln("etc/notepad");
			if (lastnote > currentuser.notedate || currentuser.noteline == 0) {
				shownotepad();
				currentuser.noteline = noteln;
				write_defnotepad();
			} else if ((noteln - currentuser.noteline) > 0) {
				move(0, 0);
				ansimore2("etc/notepad", NA, 0, noteln - currentuser.noteline + 1);
				igetkey();
				currentuser.noteline = noteln;
				write_defnotepad();
				clear();
			}
		}
	}
	if (show_statshm("0Announce/bbslist/countusr", 0) && DEFINE(DEF_GRAPH)) {
		refresh();
		pressanykey();
	}
	if ((vote_flag(NULL, '\0', 2 /* ÀË¬dÅª¹L·sªºWelcome ¨S */ ) == 0)) {
		if (dashf("Welcome")) {
			ansimore("Welcome", YEA);
			vote_flag(NULL, 'R', 2 /* ¼g¤JÅª¹L·sªºWelcome */ );
		}
	} else {
		if (fill_shmfile(3, "Welcome2", "WELCOME_SHMKEY"))
			show_welcomeshm();
	}
	show_statshm("etc/posts/day", 1);
	refresh();
	move(t_lines - 2, 0);
	clrtoeol();
	prints("[1;36m¡¸ ³o¬O±z²Ä [33m%d[36m ¦¸«ô³X¥»¯¸¡A¤W¦¸±z¬O±q [33m%s[36m ³s©¹¥»¯¸¡C\n",
		currentuser.numlogins + 1, currentuser.lasthost);
	prints("¡¸ ¤W¦¸³s½u®É¶¡¬° [33m%s[m ", ctime(&currentuser.lastlogin));
	igetkey();
	setuserfile(fname, DOTLOGINSBAD);
	if (ansimore(fname, NA) != -1) {
		if (askyn("±z­n§R°£¥H¤W±K½X¿é¤J¿ù»~ªº°O¿ý¶Ü", YEA, YEA) == YEA)
			unlink(fname);
	}
	if (currentuser.gender == 'X' && currentuser.numlogins > 1)
		check_gender();

	strncpy(currentuser.lasthost, fromhost, 16);
	currentuser.lasthost[15] = '\0';	/* dumb mistake on my part */
	currentuser.lastlogin = time(0);
	login_start_time = time(0);

	set_safe_record();
	if (HAS_PERM(PERM_LOGINOK) &&
/*	skyo.0507 modify ¾Ö¦³ SYSOP Åv­­ªÌ¡A¤£»Ý§ó·s¸ê®Æ */
/*  edwardc.990624 ÁöµM¤£»Ý­n§ó·s¸ê®Æ, ¦ý¬O­n¦Û°Ê©µªøµù¥U¦³®Ä®É¶¡ */
/*	!HAS_PERM(PERM_SYSOP) && */
		strcasecmp(currentuser.userid, "SYSOP") &&
		strcasecmp(currentuser.userid, "guest") &&
		(time(0) - currentuser.lastjustify >= REG_EXPIRED * 86400)) {
		if (HAS_PERM(PERM_SYSOP))
			currentuser.lastjustify = time(0);
		else {
			strcpy(currentuser.email, "");
			strcpy(currentuser.address, "");
			currentuser.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			mail_file("etc/expired", currentuser.userid, "§ó·s­Ó¤H¸ê®Æ»¡©ú¡C");
		}
	}
	
	if(HAS_PERM(PERM_BOARDS) && !HAS_PERM(PERM_OBOARDS)) {
		if ( !check_BM() ) {
			currentuser.userlevel &= ~(PERM_BOARDS);
			securityreport( "¤w¤£¬°ªO¥D¡A¦Û°Ê®³±¼ªO¥DÅv­­" );
		}
	}
	
	currentuser.numlogins++;
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	if (currentuser.firstlogin == 0) {
		currentuser.firstlogin = login_start_time - 7 * 86400;
	}
	check_register_info();
}


void
set_numofsig()
{
	int     sigln;
	char    signame[STRLEN];
	setuserfile(signame, "signatures");
	sigln = countln(signame);
	numofsig = sigln / MAXSIGLINES;
	if ((sigln % MAXSIGLINES) != 0)
		numofsig += 1;
}
int
chk_friend_book()
{
	FILE   *fp;
	int     idnum, n = 0;
	char    buf[STRLEN], *ptr;
	if ((fp = fopen("friendbook", "r")) == NULL)
		return 0;

	move(5, 0);
	prints("[1m¨t²Î´M¤H¦W¥U¦Cªí:[m\n\n");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char    uid[14];
		char    msg[STRLEN];
		struct user_info *uin;
		ptr = strstr(buf, "@");
		if (ptr == NULL)
			continue;
		ptr++;
		strcpy(uid, ptr);
		ptr = strstr(uid, "\n");
		*ptr = '\0';
		idnum = atoi(buf);
		if (idnum != usernum || idnum <= 0)
			continue;
		uin = t_search(uid, NA);
		sprintf(msg, "%s ¤w¸g¤W¯¸¡C", currentuser.userid);
		if (!uinfo.invisible && uin != NULL && !DEFINE(DEF_NOLOGINSEND)
			&& do_sendmsg(uin, msg, 2, uin->pid) == 1) {
			prints("[1m%s[m §ä§A¡A¨t²Î¤w¸g§i¶D¥L§A¤W¯¸ªº®ø®§¡C\n", uid);
		} else
			prints("[1m%s[m §ä§A¡A¨t²ÎµLªkÁpµ¸¨ì¥L¡A½Ð§A¸ò¥LÁpµ¸¡C\n", uid);
		n++;
		del_from_file("friendbook", buf);
		if (n > 15) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
	}
	fclose(fp);
	return n;
}

int
check_maxmail()
{
	extern char currmaildir[STRLEN];
	/* chinsan.20011231: ¼W¥[¹ï©ó¶l¥ó¹ê»Ú¤j¤p­­¨î, 
	 * ¨Ã¥Bµø "ªí²{­È" ¼u©Ê¹ï¤W­­¼W¥[ªí²{­Èªº¤@¥b, 
	 * ¦ÓKB»P¥ó¼ÆÄµ§Ù³ø§i«h¬O ³Ì«á¥[¤Wªí²{¼u©Ê¼W¥[­È¤§¤W­­ + 100
	 * ¥t¥~, ¨Ï¥ÎªÌ­Y¶W¹L¼u©Ê¤W­­ + 10 ªº¸Ü, «h·|¦b¿Ã¹õ³Ì¤W¤èÅã¥Ü"¶l¥ó¹L¶q"
	 * port from bbs.ilc.edu.tw 
	 */
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
	
	//currentuser.nummails = get_num_records(currmaildir, sizeof(struct fileheader));
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	if (mail_num > max_num) {
		prints("\n\n±zªº¨p¤H«H¥ó°ª¹F %d «Ê, ½Ð§R°£¹L´Á«H¥ó, ¾¨¶qºû«ù¦b %d «Ê¥H¤U¡C\n", mail_num, max_num);
		prints("·í«H¥ó¶W¹L %d «Ê®É, §A±NµLªk¨Ï¥Î¥»¯¸ªº°e«H¥\\¯à¡C\n", max_num + 10);
		if (mail_num > max_num + 100) {
			sprintf(genbuf, "¨p¤H«H¥ó¥ó¼Æ¨Ï¥Î¹L«×: %d «Ê", mail_num);
			securityreport(genbuf);
		}
	}
	if (mail_sum > max_sum) {
		prints("\n\n±zªº¨p¤H«H¥ó°ª¹F %d KB, ½Ð§R°£¹L´Á«H¥ó, ¾¨¶qºû«ù¦b %d KB¥H¤U¡C\n", mail_sum, max_sum);
		prints("·í«H¥ó¶W¹L %d «Ê®É, §A±NµLªk¨Ï¥Î¥»¯¸ªº°e«H¥\\¯à¡C\n", max_sum + 10);
		if (mail_sum > max_sum + 100) {
			sprintf(genbuf, "¨p¤H«H¥ó®e¶q¨Ï¥Î¹L«×: %d KB", mail_sum);
			securityreport(genbuf);
		}
	}
		
	if ((mail_num > max_num + 10) || (mail_sum > max_sum + 10)) {
		mailXX = 1;
		return (1);
	} else {
		mailXX = 0;
		return (0);
	}
}
#ifndef BBSD
int
main(argc, argv)
int     argc;
char  **argv;
#else
void
start_client()
#endif
{
#ifdef BBS_INFOD
	if (strstr(argv[0], "bbsinfo") != NULL) {
		load_sysconf();
		bbsinfod_main(argc, argv);
		exit(0);
	}
#endif
	load_sysconf();

#ifndef BBSD
	if (argc < 2 || *argv[1] != 'h') {
		printf("You cannot execute this program directly.\n");
		exit(-1);
	}
	system_init(argc, argv);
#else
	system_init();
#endif

	if (setjmp(byebye)) {
		system_abort();
	}
#ifndef BBSD
	get_tty();
	init_tty();
#endif

	login_query();
	user_login();
	m_init();
	RMSG = NA;
	clear();
	c_recover();
#ifdef TALK_LOG
	tlog_recover();		/* 990713.edwardc for talk_log recover */
#endif

	if (strcasecmp(currentuser.userid, "guest")) {
		if (HAS_PERM(PERM_ACCOUNTS) && dashf("new_register")) {
			prints("[1;33m¦³·s¨Ï¥ÎªÌ¥¿¦bµ¥±z³q¹Lµù¥U¸ê®Æ¡C[m");
		}
		check_maxmail();
		if (chk_friend_book())
			pressanykey();
		move(7, 0);
		clrtobot();
		if (!DEFINE(DEF_NOLOGINSEND))
			if (!uinfo.invisible)
				apply_ulist(friend_login_wall);
		pressanykey();
		clear();
		set_numofsig();
	}
	memset(netty_path, 0, sizeof(netty_path));
	nettyNN = NNread_init();

	fill_date();		/* ÀË¬d¬ö©À¤é */
	b_closepolls();		/* Ãö³¬§ë²¼ */

	num_alcounter();
	if (count_friends > 0 && DEFINE(DEF_LOGFRIEND))
		t_friends();
	while (1) {
		if (DEFINE(DEF_NORMALSCR))
			domenu("TOPMENU");
		else
			domenu("TOPMENU2");
		Goodbye();
	}
}

int     refscreen = NA;

int
egetch()
{
	int     rval;
	check_calltime();
	if (talkrequest) {
		talkreply();
		refscreen = YEA;
		return -1;
	}
	while (1) {
		rval = igetkey();
		if (talkrequest) {
			talkreply();
			refscreen = YEA;
			return -1;
		}
		if (rval != Ctrl('L'))
			break;
		redoscr();
	}
	refscreen = NA;
	return rval;
}

char   *
boardmargin()
{
	static char buf[STRLEN];
	if (selboard)
		sprintf(buf, "°Q½×°Ï [%s]", currboard);
	else {
		brc_initial(DEFAULTBOARD);
		if (getbnum(currboard)) {
			selboard = 1;
			sprintf(buf, "°Q½×°Ï [%s]", currboard);
		} else
			sprintf(buf, "¥Ø«e¨Ã¨S¦³³]©w°Q½×°Ï");
	}
	return buf;
}
/*Add by SmallPig*/
void
update_endline()
{
	char    buf[STRLEN];
	time_t  now;
	int     allstay;
	if (!DEFINE(DEF_ENDLINE)) {
		move(t_lines - 1, 0);
		clrtoeol();
		return;
	}
	now = time(0);
	allstay = (now - login_start_time) / 60;
	move(t_lines - 1, 0);
	clrtoeol();
	sprintf(buf, "[[36m%.12s[33m]", currentuser.userid);
	num_alcounter();
	resolve_boards();
	if (strlen(brdshm->date) == 0) {
		strcpy(brdshm->date, "  ¨SÔ£¤j¤é¤l  ");
	}
	prints("[1;44;33m®É¶¡:[[36m%16s[37m%14s[44;33m] Á`¤H¼Æ/¦n¤Í:[[36m%4d[33m/[1;36m%3d[33m] ±b¸¹:%-22s[m", 
		Ctime(&now) + 5, (is_birth(currentuser)) ? "  ¥Í¤é­n½Ð«È  " : brdshm->date, count_users, count_friends, buf);

}


/*ReWrite by SmallPig*/
void
showtitle(title, mid)
char   *title, *mid;
{
	char    buf[STRLEN], *note;
	int     spc1, spc2;
	note = boardmargin();
	spc1 = 39 - strlen(title) - strlen(mid) / 2;
	spc2 = 40 - strlen(note) - strlen(mid) / 2;
	if (spc1 < 2)
		spc1 = 2;
	if (spc2 < 2)
		spc2 = 2;
	move(0, 0);
	clrtoeol();
	sprintf(buf, "%*s", spc1, "");
	if (!strcmp(mid, BoardName))
		prints("[1;44;33m%s%s[37m%s[1;44m", title, buf, mid);
	else if (mid[0] == '[')
		prints("[1;44;33m%s%s[5;36m%s[m[1;44m", title, buf, mid);
	else
		prints("[1;44;33m%s%s[36m%s", title, buf, mid);
	sprintf(buf, "%*s", spc2, "");
	prints("%s[33m%s[m\n", buf, note);
	update_endline();
	move(1, 0);
}

void
docmdtitle(title, prompt)
char   *title, *prompt;
{
	char    middoc[30];
	if (chkmail())
		strcpy(middoc, "[±z¦³«H¥ó]");
	else if (mailXX == 1)
		strcpy(middoc, "[«H¥ó¹L¶q!!!]");
	else
		strcpy(middoc, BoardName);

	showtitle(title, middoc);
	move(1, 0);
	clrtoeol();
	prints("%s", prompt);
	clrtoeol();
}
/* this is standard edwardc's style function :) */
void
c_recover()
{
	char    fname[STRLEN], buf[STRLEN];
	int     a;
	sprintf(fname, "home/%c/%s/%s.deadve", toupper(currentuser.userid[0]), currentuser.userid, currentuser.userid);
	if ( dashf(fname) ) {
		clear();
		strcpy(genbuf, "");
		if (strcasecmp(currentuser.userid, "guest") == 0) {
			/*
			 * edwardc.990630 well, guest ¨ÃµLªk¦¬«H,
			 * ©Ò¥H§âÂ_½u³Æ¥÷±H¨ì«H½c¬O¨S¦³ ®ÄªGªº,
			 * ©Ò¥H§Ú­Ì§â¥¦¦s¨ì©T©wªº¤@­Ó¼È¦sÀÉ, »Ý­nªº¤H¦A¥h¨ú¥X¨Ï¥Î
			 */

			prints("[1;32m±z¦³¤@­Ó½s¿è§@·~¤£¥¿±`¤¤Â_, ¨t²Î±N³Æ¥÷ÀÉ¦s¦Ü ¼È¦sÀÉ 1 [m\n");
			prints("(½Ð¦b½s¿è®É«ö ESC + I + 1 ¨ú¥X¼È¦sÀÉ¤º®e)\n");
			sprintf(buf, "home/%c/%s/clip_1", toupper(currentuser.userid[0]), currentuser.userid);
			unlink(buf);	/* discard old file */
			rename(fname, buf);
			pressanykey();
			return;	/* bye bye */

		}
		getdata(0, 0, "[1;32m±z¦³¤@­Ó½s¿è§@·~¤£¥¿±`¤¤Â_¡A(S) ¼g¤J¼È¦sÀÉ (M) ±H¦^«H½c (Q) ºâ¤F¡H[Q]¡G[m", genbuf, 2, DOECHO, YEA);

		switch (genbuf[0]) {

		case 'M':
		case 'm':
			mail_file(fname, currentuser.userid, "¤£¥¿±`Â_½u©Ò«O¯dªº³¡¥÷...");
			unlink(fname);
			break;

		case 'S':
		case 's':
			while (1) {
				strcpy(genbuf, "");
				getdata(2, 0, "[1;33m½Ð¿ï¾Ü¼È¦sÀÉ [0-7] [0]¡G[m", genbuf, 2, DOECHO, YEA);
				if (genbuf[0] == '\0')
					a = 0;
				else
					a = atoi(genbuf);

				if (a >= 0 && a <= 7) {
					sprintf(buf, "home/%c/%s/clip_%d", toupper(currentuser.userid[0]), currentuser.userid, a);
					if (dashf(buf)) {
						getdata(3, 0, "[1;31m¼È¦sÀÉ¤w¦s¦b¡AÂÐ»\\©Îªþ¥[? (O)ÂÐ»\\ (A)ªþ¥[ [O]¡G[m", genbuf, 2, DOECHO, YEA);
						switch (genbuf[0]) {
						case 'A':
						case 'a':
							f_cp(fname, buf, O_APPEND);
							uname(fname);
							break;
						default:
							unlink(buf);
							rename(fname, buf);
							break;
						}
					} else {
						rename(fname, buf);
					}
					break;
				}
			}
			break;

		case 'Q':
		case 'q':
		default:
			unlink(fname);
			break;

		}
	}
}

#ifdef TALK_LOG
void
tlog_recover()
{
	char    buf[256];
	sprintf(buf, "home/%c/%s/talk_log", toupper(currentuser.userid[0]), currentuser.userid);

	if (strcasecmp(currentuser.userid, "guest") == 0 || !dashf(buf))
		return;

	clear();
	strcpy(genbuf, "");

	getdata(0, 0, "[1;32m±z¦³¤@­Ó¤£¥¿±`Â_½u©Ò¯d¤U¨Óªº²á¤Ñ°O¿ý, ±z­n .. (M) ±H¦^«H½c (Q) ºâ¤F¡H[Q]¡G[m", genbuf, 2, DOECHO, YEA);

	if (genbuf[0] == 'M' || genbuf[0] == 'm')
		mail_file(buf, currentuser.userid, "²á¤Ñ°O¿ý");

	unlink(buf);
	return;

}
#endif

int
check_BM()
{
	int i, numboards;
	struct boardheader fh;
	char *str;
	
	numboards = get_num_records(BOARDS, sizeof(fh));
	
	for (i = 0; i < numboards; i++) {
		get_record(BOARDS, &fh, sizeof(fh), i) ;
		for ( str = strtok(fh.BM, ",: ;|&()\0\n");
			str != NULL;
			str = strtok( NULL, ",: ;|&()\0\n") )
			if ( !strcmp(currentuser.userid, str) )
				return YEA;
	}

	return NA;
}
