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
$Id: xyz.c,v 1.3 2001/08/30 10:03:33 edwardc Exp $
*/

#define EXTERN
#include "bbs.h"
int     use_define = 0;
extern int iscolor;

extern struct BCACHE *brdshm;
#define TH_LOW	10
#define TH_HIGH	15

int
modify_user_mode(mode)
int     mode;
{
	uinfo.mode = mode;
	update_ulist(&uinfo, utmpent);
	return 0;
}

int
showperminfo(pbits, i)
int     pbits, i;
{
	char    buf[STRLEN];
	sprintf(buf, "%c. %-30s %2s", 'A' + i, (use_define) ? user_definestr[i] : permstrings[i],
		((pbits >> i) & 1 ? "£¾" : "¡Ñ"));
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints(buf);
	refresh();
	return YEA;
}

unsigned int
setperms(pbits, prompt, numbers, showfunc)
unsigned int pbits;
char   *prompt;
int     numbers;
int     (*showfunc) ();
{
	int     lastperm = numbers - 1;
	int     i, done = NA;
	char    choice[3], buf[80];
	move(4, 0);
	prints("½Ð«ö¤U§A­nªº¥N½X¨Ó³]©w%s¡A«ö Enter µ²§ô.\n", prompt);
	move(6, 0);
	clrtobot();
	for (i = 0; i <= lastperm; i++) {
		(*showfunc) (pbits, i, NA);
	}
	while (!done) {
		sprintf(buf, "¿ï¾Ü(ENTER µ²§ô%s): ", ((strcmp(prompt, "Åv­­") != 0)) ? "" : "¡A0 °±Åv");
		getdata(t_lines - 1, 0, buf, choice, 2, DOECHO, YEA);
		*choice = toupper(*choice);
		if (*choice == '0')
			return (0);
		else if (*choice == '\n' || *choice == '\0')
			done = YEA;
		else if (*choice < 'A' || *choice > 'A' + lastperm)
			bell();
		else {
			i = *choice - 'A';
			pbits ^= (1 << i);
			if ((*showfunc) (pbits, i, YEA) == NA) {
				pbits ^= (1 << i);
			}
		}
	}
	return (pbits);
}

int
x_userdefine()
{
	int     id;
	unsigned int newlevel;
	extern int nettyNN;
	modify_user_mode(USERDEF);
	if (!(id = getuser(currentuser.userid))) {
		move(3, 0);
		prints("¿ù»~ªº¨Ï¥ÎªÌ ID...");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	use_define = 1;
	newlevel = setperms(lookupuser.userdefine, "°Ñ¼Æ", NUMDEFINES, showperminfo);
	move(2, 0);
	if (newlevel == lookupuser.userdefine)
		prints("°Ñ¼Æ¨S¦³­×§ï...\n");
	else {
		lookupuser.userdefine = newlevel;
		currentuser.userdefine = newlevel;
		substitute_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
		uinfo.pager |= FRIEND_PAGER;
		if (!(uinfo.pager & ALL_PAGER)) {
			if (!DEFINE(DEF_FRIENDCALL))
				uinfo.pager &= ~FRIEND_PAGER;
		}
		uinfo.pager &= ~ALLMSG_PAGER;
		uinfo.pager &= ~FRIENDMSG_PAGER;
		if (DEFINE(DEF_FRIENDMSG)) {
			uinfo.pager |= FRIENDMSG_PAGER;
		}
		if (DEFINE(DEF_ALLMSG)) {
			uinfo.pager |= ALLMSG_PAGER;
			uinfo.pager |= FRIENDMSG_PAGER;
		}
		update_utmp();
		if (DEFINE(DEF_ACBOARD))
			nettyNN = NNread_init();
		prints("·sªº°Ñ¼Æ³]©w§¹¦¨...\n\n");
	}
	iscolor = (DEFINE(DEF_COLOR)) ? 1 : 0;
	pressreturn();
	clear();
	use_define = 0;
	return 0;
}

int
x_cloak()
{
	modify_user_mode(GMENU);
	report("toggle cloak");
	uinfo.invisible = (uinfo.invisible) ? NA : YEA;
	update_utmp();
	if (!uinfo.in_chat) {
		move(1, 0);
		clrtoeol();
		prints("Áô¨­³N (cloak) ¤w¸g%s¤F!",
			(uinfo.invisible) ? "±Ò°Ê" : "°±¤î");
		pressreturn();
	}
	return 0;
}

int
x_listdownline()
{
	modify_user_mode(XMENU);
	move(1, 0);
	clrtobot();
	prints("¥H¤U¨Ï¥ÎªÌ¸g±zªº¤¶²Ð¦Ó¨Ó:");
	setuserfile(genbuf, "downline");
	listfilecontent(genbuf);
	pressreturn();
	return 0;
}

int
x_state()
{
#if defined(LINUX)
	char    buf[80];
	modify_user_mode(XMENU);
	sprintf(buf, "tmp/procinfo.%s", currentuser.userid);
	sprintf(genbuf, "/usr/local/bin/procinfo > %s", buf);
	system(genbuf);
	ansimore(buf, YEA);
	unlink(buf);
#endif
	return FULLUPDATE;
}

void
x_edits()
{
	int     aborted;
	char    ans[7], buf[STRLEN];
	int     ch, num, confirm;
	static char *e_file[] =
	{"plans", "signatures", "notes", "logout", NULL};
	static char *explain_file[] =
	{"­Ó¤H»¡©úÀÉ", "Ã±¦WÀÉ", "¦Û¤vªº³Æ§Ñ¿ý", "Â÷¯¸ªºµe­±", NULL};
	modify_user_mode(GMENU);
	clear();
	move(1, 0);
	prints("½s­×­Ó¤HÀÉ®×\n\n");
	for (num = 0; e_file[num] != NULL && explain_file[num] != NULL; num++) {
		prints("[[1;32m%d[m] %s\n", num + 1, explain_file[num]);
	}
	prints("[[1;32m%d[m] ³£¤£·Q§ï\n", num + 1);

	getdata(num + 5, 0, "§A­n½s­×­þ¤@¶µ­Ó¤HÀÉ®×: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' <= 0 || ans[0] - '0' > num || ans[0] == '\n' || ans[0] == '\0')
		return;

	ch = ans[0] - '0' - 1;
	setuserfile(genbuf, e_file[ch]);
	move(3, 0);
	clrtobot();
	sprintf(buf, "(E)½s¿è (D)§R°£ %s? [E]: ", explain_file[ch]);
	getdata(3, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		confirm = askyn("§A½T©w­n§R°£³o­ÓÀÉ®×", NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("¨ú®ø§R°£¦æ°Ê\n");
			pressreturn();
			clear();
			return;
		}
		unlink(genbuf);
		move(5, 0);
		prints("%s ¤w§R°£\n", explain_file[ch]);
		sprintf(buf, "delete %s", explain_file[ch]);
		report(buf);
		pressreturn();
		clear();
		return;
	}
	modify_user_mode(EDITUFILE);
	aborted = vedit(genbuf, NA);
	clear();
	if (!aborted) {
		prints("%s §ó·s¹L\n", explain_file[ch]);
		sprintf(buf, "edit %s", explain_file[ch]);
		if (!strcmp(e_file[ch], "signatures")) {
			set_numofsig();
			prints("¨t²Î­«·s³]©w¥H¤ÎÅª¤J§AªºÃ±¦WÀÉ...");
		}
		report(buf);
	} else
		prints("%s ¨ú®ø­×§ï\n", explain_file[ch]);
	pressreturn();
}

void
x_lockscreen()
{
	char    buf[PASSLEN + 1];
	time_t  now;
	modify_user_mode(LOCKSCREEN);
	move(9, 0);
	clrtobot();
	update_endline();
	buf[0] = '\0';
	now = time(0);
	move(9, 0);
	prints("\n[1;37m       _       _____   ___     _   _   ___     ___       __");
	prints("\n      ( )     (  _  ) (  _`\\  ( ) ( ) (  _`\\  (  _`\\    |  |");
	prints("\n      | |     | ( ) | | ( (_) | |/'/' | (_(_) | | ) |   |  |");
	prints("\n      | |  _  | | | | | |  _  | , <   |  _)_  | | | )   |  |");
	prints("\n      | |_( ) | (_) | | (_( ) | |\\`\\  | (_( ) | |_) |   |==|");
	prints("\n      (____/' (_____) (____/' (_) (_) (____/' (____/'   |__|[m\n");
	prints("\n[1;36m¿Ã¹õ¤w¦b[33m %19s[36m ®É³Q[32m %-12s [36m¼È®ÉÂê¦í¤F...[m", ctime(&now), currentuser.userid);
	while (*buf == '\0' || !checkpasswd(currentuser.passwd, buf)) {
		move(18, 0);
		clrtobot();
		update_endline();
		getdata(19, 0, "½Ð¿é¤J§Aªº±K½X¥H¸ÑÂê: ", buf, PASSLEN, NOECHO, YEA);
	}
}
heavyload()
{
#ifndef BBSD
	double  cpu_load[3];
	get_load(cpu_load);
	if (cpu_load[0] > 15)
		return 1;
	else
		return 0;
#else
#ifdef chkload
	register int load;
	register time_t uptime;
	if (time(0) > uptime) {
		load = chkload(load ? TH_LOW : TH_HIGH);
		uptime = time(0) + load + 45;
	}
	return load;
#else
	return 0;
#endif
#endif
}

void
exec_cmd(umode, pager, cmdfile, param1)
int     umode, pager;
char   *cmdfile, *param1;
{
	char    buf[STRLEN * 2];
	int     save_pager;
	if (num_useshell() >= 20) {
		clear();
		prints("¤Ó¦h¤H¨Ï¥Î¥~³¡µ{¦¡¤F¡A§Aµ¥¤@¤U¦A¥Î§a...");
		pressanykey();
		return;
	}
	if (!HAS_PERM(PERM_SYSOP) && heavyload()) {
		clear();
		prints("©êºp¡A¥Ø«e¨t²Î­t²ü¹L­«¡A¦¹¥\\¯à¼È®É¤£¯à°õ¦æ...");
		pressanykey();
		return;
	}
	if (!dashf(cmdfile)) {
		move(2, 0);
		prints("no %s\n", cmdfile);
		pressreturn();
		return;
	}
	save_pager = uinfo.pager;
	if (pager == NA) {
		uinfo.pager = 0;
	}
	modify_user_mode(umode);
	sprintf(buf, "/bin/sh %s %s %s %d", cmdfile, param1, currentuser.userid, getpid());
	report(buf);
	reset_tty();
	do_exec(buf, NULL);
	restore_tty();
	uinfo.pager = save_pager;
	clear();
}
#ifdef IRC
void
t_irc()
{
	exec_cmd(IRCCHAT, NA, "bin/irc.sh", "");
}
#endif				/* IRC */

void
x_www()
{
	exec_cmd(WWW, NA, "bin/www.sh", "");
}

void
x_hytelnet()
{
	exec_cmd(HYTELNET, NA, "bin/hytelnet.sh", "");
}
/* ppfoong */
void
x_excearchie()
{
	char    buf[STRLEN];
	char   *s;
	if (heavyload()) {
		clear();
		prints("©êºp¡A¥Ø«e¨t²Î­t²ü¹L­«¡A¦¹¥\\¯à¼È®É¤£¯à°õ¦æ...");
		pressanykey();
		return;
	}
	modify_user_mode(ARCHIE);
	clear();
	prints("\n\n[1;33m       _______ ______ ______ _______ _______ _______");
	prints("\n      |   _   |   __ \\      |   |   |_     _|    ___|");
	prints("\n      |       |      <   ---|       |_|   |_|    ___|");
	prints("\n      |___|___|___|__|______|___|___|_______|_______|[0m");
	prints("\n\n\nÅwªï¨Ï¥Î ARCHIE ¥\\¯à");
	prints("\n\n¥»¥\\¯à±N¬°±z¦C¥X¦b­þ­Ó FTP ¯¸¦s¦³±z±ý´M§äªºÀÉ®×.");
	prints("\n\n½Ð¿é¤J±ý·j´Mªº¦r¦ê, ©Îª½±µ«ö <ENTER> ¨ú®ø¡C");
	getdata(13, 0, ">", buf, 20, DOECHO, YEA);
	if (buf[0] == '\0') {
		prints("\n¨ú®ø·j´M.....\n");
		pressanykey();
		return;
	}
	for (s = buf; *s != '\0'; s++) {
		if (isspace(*s)) {
			prints("\n¤@¦¸¥u¯à·j´M¤@­Ó¦r¦ê°Õ, ¤£¯à¤Ó³g¤ß³á!!");
			pressanykey();
			return;
		}
	}
	exec_cmd(ARCHIE, YEA, "bin/archie.sh", buf);
	sprintf(buf, "tmp/archie.%s.%05d", currentuser.userid, uinfo.pid);
	if (1 /* dashf(buf) */ ) {
		if (askyn("­n±Nµ²ªG±H¦^«H½c¶Ü", NA, NA) == YEA)
			mail_file(buf, currentuser.userid, "ARCHIE ·j´Mµ²ªG");
		ansimore(buf, YEA);
		unlink(buf);
	}
}
/* ppfoong */
void
x_dict()
{
	char    buf[STRLEN];
	char   *s;
	int     whichdict;
	if (heavyload()) {
		clear();
		prints("©êºp¡A¥Ø«e¨t²Î­t²ü¹L­«¡A¦¹¥\\¯à¼È®É¤£¯à°õ¦æ...");
		pressanykey();
		return;
	}
	modify_user_mode(DICT);
	clear();
	prints("\n[1;32m     _____  __        __   __");
	prints("\n    |     \\|__|.----.|  |_|__|.-----.-----.---.-.----.--.--.");
	prints("\n    |  --  |  ||  __||   _|  ||  _  |     |  _  |   _|  |  |");
	prints("\n    |_____/|__||____||____|__||_____|__|__|___._|__| |___  |");
	prints("\n                                                     |_____|[m");
	prints("\n\n\nÅwªï¨Ï¥Î¥»¯¸ªº¦r¨å¡C");
	prints("\n¥»¦r¨å¥D­n¬°[1;33m¡u­^º~¡v[m³¡¤À, ¦ý¥ç¥i§@[1;33m¡uº~­^¡v[m¬d¸ß¡C");
	prints("\n\n¨t²Î±N®Ú¾Ú±z©Ò¿é¤Jªº¦r¦ê, ¦Û°Ê§PÂ_±z©Ò­nÂ½¬dªº¬O­^¤å¦rÁÙ¬O¤¤¤å¦r¡C");
	prints("\n\n\n½Ð¿é¤J±z±ýÂ½¬dªº­^¤å¦r©Î¤¤¤å¦r, ©Îª½±µ«ö <ENTER> ¨ú®ø¡C");
	getdata(15, 0, ">", buf, 30, DOECHO, YEA);
	if (buf[0] == '\0') {
		prints("\n±z¤£·Q¬d¤F³á...");
		pressanykey();
		return;
	}
	for (s = buf; *s != '\0'; s++) {
		if (isspace(*s)) {
			prints("\n¤@¦¸¥u¯à¬d¤@­Ó¦r°Õ, ¤£¯à¤Ó³g¤ß³á!!");
			pressanykey();
			return;
		}
	}
	whichdict = YEA;
	for (s = buf; *s != '\0'; s++) {
		if (!(isalpha(*s) || *s == '-')) {
			whichdict = NA;
			break;
		}
	}
	if (whichdict)
		exec_cmd(DICT, YEA, "bin/cdict.sh", buf);
	else
		exec_cmd(DICT, YEA, "bin/edict.sh", buf);
	sprintf(buf, "tmp/dict.%s.%05d", currentuser.userid, uinfo.pid);
	if (dashf(buf)) {
		if (askyn("­n±Nµ²ªG±H¦^«H½c¶Ü", NA, NA) == YEA)
			mail_file(buf, currentuser.userid, "¦r¨å¬d¸ßµ²ªG");
		ansimore(buf, YEA);
		unlink(buf);
	}
}
/* Add By P.P.Foong */
void
x_sysinfo()
{
	char    buf[STRLEN];
	sprintf(buf, "tmp/sysinfo.%s", currentuser.userid);
	if (!dashf(buf)) {
		exec_cmd(SYSINFO, YEA, "bin/sysinfo.sh", "");
		ansimore(buf, YEA);
	}
	unlink(buf);
}
/* Add By P.P.Foong */
void
x_game()
{
	exec_cmd(GAME, YEA, "bin/game.sh", "");
}

void
x_showuser()
{
	char    buf[STRLEN];
	modify_user_mode(SYSINFO);
	clear();
	stand_title("¥»¯¸¨Ï¥ÎªÌ¸ê®Æ¬d¸ß");
	ansimore("etc/showuser.msg", NA);
	getdata(20, 0, "Parameter: ", buf, 30, DOECHO, YEA);
	if ((buf[0] == '\0') || dashf("tmp/showuser.result"))
		return;
	securityreport("¬d¸ß¨Ï¥ÎªÌ¸ê®Æ");
	exec_cmd(SYSINFO, YEA, "bin/showuser.sh", buf);
	sprintf(buf, "tmp/showuser.result");
	if (dashf(buf)) {
		mail_file(buf, currentuser.userid, "¨Ï¥ÎªÌ¸ê®Æ¬d¸ßµ²ªG");
		unlink(buf);
	}
}

void
ent_bnet()
{
	clear();
	exec_cmd(BBSNET, YEA, "bin/bbsnet.sh", "");
}

void
fill_date()
{
	time_t  now, next;
	char   *buf, *buf2, *index, index_buf[10];
	char    d_value[20];
	char    h[3], m[3], s[3];
	int     foo, foo2, i, flag = 0;
	struct tm *mytm;
	FILE   *fp;
	now = time(0);
	resolve_boards();

	if (now < brdshm->fresh_date && strlen(brdshm->date) != 0)
		return;

	mytm = localtime(&now);
	strftime(h, 3, "%H", mytm);
	strftime(m, 3, "%M", mytm);
	strftime(s, 3, "%S", mytm);

	next = (time_t) time(0) - ((atoi(h) * 3600) + (atoi(m) * 60) + atoi(s)) +
		86400;		/* ºâ¥X¤µ¤Ñ 0:0:00 ªº®É¶¡, µM«á¦A©¹«á¥[¤@¤Ñ */

	sprintf(genbuf, "¬ö©À¤é§ó·s, ¤U¤@¦¸§ó·s®É¶¡ %s", Cdate(&next));
	report(genbuf);

	buf = (char *)malloc(80);
	buf2 = (char *)malloc(80);
	index = (char *)malloc(10);
	bzero(d_value, 20);

	fp = fopen(DEF_FILE, "r");

	if (fp == NULL)
		return;

	now = time(0);
	mytm = localtime(&now);
	strftime(index_buf, 10, "%m%d", mytm);

	while (fgets(buf, 80, fp) != NULL) {
		if (buf[0] == ';' || buf[0] == '#' || buf[0] == ' ')
			continue;

		sscanf(buf, "%s %s", index, buf2);

		if (*index == 0 || *buf2 == 0)
			continue;

		if (strcmp(index, "0000") == 0 || strcmp(index_buf, index) == 0) {

			foo = strlen(buf2);

			if (foo < 14 && foo > 0) {

				foo2 = (14 - foo) / 2;
				if (foo2 > 1) {

					strcpy(brdshm->date, "");

					for (i = 0; i < foo2; i++)
						strcat(brdshm->date, " ");

					strcat(brdshm->date, buf2);

					for (i = 0; i < 14 - (foo + foo2); i++)
						strcat(brdshm->date, " ");

				} else {
					strcpy(brdshm->date, buf2);
				}

			} else {
				strcpy(brdshm->date, buf2);
			}

			if (strcmp(index, "0000") == 0) {
				strcpy(d_value, brdshm->date);
			} else {
				flag = 1;
			}

		}
	}

	fclose(fp);

	if (flag == 0) {
		if (d_value[0] == '\0')
			strcpy(brdshm->date, DEF_VALUE);
		else
			strcpy(brdshm->date, d_value);
	}
	brdshm->fresh_date = next;

	free(buf);
	free(buf2);
	free(index);

	return;
}

int
is_birth(user)
struct userec user;
{
	struct tm *tm;
	time_t  now;
	now = time(0);
	tm = localtime(&now);

	if (strcasecmp(user.userid, "guest") == 0)
		return NA;

	if (user.birthmonth == (tm->tm_mon + 1) && user.birthday == tm->tm_mday)
		return YEA;
	else
		return NA;
}

int 
trusted_host(char *address)
{
	FILE   *fp;
	char    buf[STRLEN], *ptr;

	if ( !dashf(F_TRUSTED_HOST) )
		return 1;		/* ¨S¦³³]©w´NµL­­¨î°Õ */
			
	if ((fp = fopen(F_TRUSTED_HOST, "r")) != NULL ) {
		while (fgets(buf, STRLEN, fp)) {
			ptr = strtok(buf, "\n\t\r");
			if (ptr != NULL && *ptr != '#' && strcasecmp_match(address, ptr) == 0) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}
