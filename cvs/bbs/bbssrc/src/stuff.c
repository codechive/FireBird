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
$Id: stuff.c,v 1.2 2002/09/05 06:04:10 edwardc Exp $
*/

#include "bbs.h"
#include <sys/param.h>

extern char *getenv();
extern char fromhost[];

int
pressanykey()
{
	extern int showansi;
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[m                                [5;1;33m«ö¥ô¦óÁäÄ~Äò...[m");
	egetch();
	move(t_lines - 1, 0);
	clrtoeol();
	return 0;
}

int
pressreturn()
{
	extern int showansi;
	char    buf[3];
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, "                              [1;33m½Ð«ö ¡»[5;36mEnter[m[1;33m¡» Ä~Äò\033[m", buf, 2, NOECHO, YEA);
	move(t_lines - 1, 0);
	clrtoeol();
	refresh();
	return 0;
}

int
msgmorebar(char *filename)
{
	extern int showansi;
	char    title[256];
	int     ch;
	time_t  now;
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();

	prints("[0m[1;44;32m°T®§ÂsÄý¾¹   «O¯d <[1;37mr[32m>    ²M°£ <[1;37mc[1;32m>   ±H¦^«H½c<[1;37mm[1;32m>                                [m");
	move(t_lines - 1, 0);

	ch = morekey();
	if (ch == 'C') {
		unlink(filename);
		return ch;
	} else if (ch == 'M') {
		now = time(0);
		sprintf(title, "[%12.12s] ©Ò¦³°T®§³Æ¥÷", ctime(&now) + 4);
		mail_file(filename, currentuser.userid, title);
		unlink(filename);
		return ch;
	} else if (ch == 'H') {
		show_help(F_HELP_MSG);
	}
	clrtoeol();
	refresh();
	return ch;
}
askyn(str, defa, gobottom)
char    str[STRLEN];
int     defa, gobottom;
{
	int     x, y;
	char    realstr[100];
	char    ans[3];
	sprintf(realstr, "%s (Y/N)? [%c]: ", str, (defa) ? 'Y' : 'N');
	if (gobottom)
		move(t_lines - 1, 0);
	getyx(&x, &y);
	clrtoeol();
	getdata(x, y, realstr, ans, 2, DOECHO, YEA);
	if (ans[0] != 'Y' && ans[0] != 'y' &&
		ans[0] != 'N' && ans[0] != 'n') {
		return defa;
	} else if (ans[0] == 'Y' || ans[0] == 'y')
		return 1;
	else if (ans[0] == 'N' || ans[0] == 'n')
		return 0;
}

void
printdash(mesg)
char   *mesg;
{
	char    buf[80], *ptr;
	int     len;
	memset(buf, '=', 79);
	buf[79] = '\0';
	if (mesg != NULL) {
		len = strlen(mesg);
		if (len > 76)
			len = 76;
		ptr = &buf[40 - len / 2];
		ptr[-1] = ' ';
		ptr[len] = ' ';
		strncpy(ptr, mesg, len);
	}
	prints("%s\n", buf);
}

/* 990807.edwardc fix beep sound in bbsd .. */

void
bell()
{
#ifndef BBSD
	char    sound[3], *ptr;
	ptr = sound;
	memset(ptr, Ctrl('G'), sizeof(sound));
	write(1, ptr, sizeof(sound));
#else
	static char sound[1] = {Ctrl('G')};
  
    send(0, sound, sizeof(sound), 0);
#endif
}

void
touchnew()
{
	sprintf(genbuf, "touch by: %s", currentuser.userid);
	file_append(FLUSH, genbuf);
}
/* rrr - Snagged from pbbs 1.8 */

#define LOOKFIRST  (0)
#define LOOKLAST   (1)
#define QUOTEMODE  (2)
#define MAXCOMSZ (1024)
#define MAXARGS (40)
#define MAXENVS (20)
#define BINDIR "/bin/"

char   *bbsenv[MAXENVS];
int     numbbsenvs = 0;

int
deltree(dst)
char   *dst;
{
	if (strstr(dst, "//") || strstr(dst, "..") || strchr(dst, ' '))
		return 0;	/* precaution */
	if (dst[strlen(dst) - 1] == '/')
		return 0;
	if (dashd(dst)) {
		sprintf(genbuf, "/bin/rm -rf %s", dst);
		system(genbuf);
		return 1;
	} else
		return 0;
}

int
bbssetenv(env, val)
char   *env, *val;
{
	register int i, len;
	if (numbbsenvs == 0)
		bbsenv[0] = NULL;
	len = strlen(env);
	for (i = 0; bbsenv[i]; i++)
		if (!ci_strncmp(env, bbsenv[i], len))
			break;
	if (i >= MAXENVS)
		return -1;
	if (bbsenv[i])
		free(bbsenv[i]);
	else
		bbsenv[++numbbsenvs] = NULL;
	bbsenv[i] = malloc(strlen(env) + strlen(val) + 2);
	strcpy(bbsenv[i], env);
	strcat(bbsenv[i], "=");
	strcat(bbsenv[i], val);
	return 0;
}

int
do_exec(com, wd)
char   *com, *wd;
{
	char    path[MAXPATHLEN];
	char    pcom[MAXCOMSZ];
	char   *arglist[MAXARGS];
	char   *tz;
	register int i, len;
	register int argptr;
	int     status, pid, w;
	int     pmode;
	void    (*isig) (), (*qsig) ();

	strncpy(path, BINDIR, MAXPATHLEN);
	strncpy(pcom, com, MAXCOMSZ);
	len = Min(strlen(com) + 1, MAXCOMSZ);
	pmode = LOOKFIRST;
	for (i = 0, argptr = 0; i < len; i++) {
		if (pcom[i] == '\0')
			break;
		if (pmode == QUOTEMODE) {
			if (pcom[i] == '\001') {
				pmode = LOOKFIRST;
				pcom[i] = '\0';
				continue;
			}
			continue;
		}
		if (pcom[i] == '\001') {
			pmode = QUOTEMODE;
			arglist[argptr++] = &pcom[i + 1];
			if (argptr + 1 == MAXARGS)
				break;
			continue;
		}
		if (pmode == LOOKFIRST)
			if (pcom[i] != ' ') {
				arglist[argptr++] = &pcom[i];
				if (argptr + 1 == MAXARGS)
					break;
				pmode = LOOKLAST;
			} else
				continue;
		if (pcom[i] == ' ') {
			pmode = LOOKFIRST;
			pcom[i] = '\0';
		}
	}
	arglist[argptr] = NULL;
	if (argptr == 0)
		return -1;
	if (*arglist[0] == '/')
		strncpy(path, arglist[0], MAXPATHLEN);
	else
		strncat(path, arglist[0], MAXPATHLEN);
	reset_tty();
	alarm(0);
#ifdef IRIX
	if ((pid = fork()) == 0) {
#else
	if ((pid = vfork()) == 0) {
#endif

#ifdef BBSD
		waitpid(pid, &status, 0);
#endif

		if (wd)
			if (chdir(wd)) {
				sprintf(genbuf, "Unable to chdir to '%s'\n", wd);
				report(genbuf);
				exit(-1);
			}

		bbssetenv("PATH", "/bin:.");
		bbssetenv("TERM", currentuser.termtype);
		bbssetenv("USER", currentuser.userid);
		bbssetenv("USERNAME", currentuser.username);

		if ((tz = getenv("TZ")) != NULL)
			bbssetenv("TZ", tz);
		if (numbbsenvs == 0)
			bbsenv[0] = NULL;
		execve(path, arglist, bbsenv);
		sprintf(genbuf, "EXECV FAILED... path = '%s'\n", path);
		report(genbuf);
		exit(-1);
	}
	isig = signal(SIGINT, SIG_IGN);
	qsig = signal(SIGQUIT, SIG_IGN);
#ifndef BBSD
	while ((w = wait(&status)) != pid && w != 1)
		 /* NULL STATEMENT */ ;
#endif		 
	signal(SIGINT, isig);
	signal(SIGQUIT, qsig);
	restore_tty();
#ifdef DOTIMEOUT
	alarm(IDLE_TIMEOUT);
#endif
	return ((w == -1) ? w : status);
}

char   *
horoscope(month, day)
char    month, day;
{
	char   *name[12] = {
		"¼¯½~", "¤ô²~", "Âù³½", "¨d¦Ï", "ª÷¤û", "Âù¤l",
		"¥¨ÃÉ", "·à¤l", "³B¤k", "¤Ñ¯¯", "¤ÑÃÈ", "®g¤â"
	};
	switch (month) {
	case 1:
		if (day < 21)
			return (name[0]);
		else
			return (name[1]);
	case 2:
		if (day < 19)
			return (name[1]);
		else
			return (name[2]);
	case 3:
		if (day < 21)
			return (name[2]);
		else
			return (name[3]);
	case 4:
		if (day < 21)
			return (name[3]);
		else
			return (name[4]);
	case 5:
		if (day < 21)
			return (name[4]);
		else
			return (name[5]);
	case 6:
		if (day < 22)
			return (name[5]);
		else
			return (name[6]);
	case 7:
		if (day < 23)
			return (name[6]);
		else
			return (name[7]);
	case 8:
		if (day < 23)
			return (name[7]);
		else
			return (name[8]);
	case 9:
		if (day < 23)
			return (name[8]);
		else
			return (name[9]);
	case 10:
		if (day < 24)
			return (name[9]);
		else
			return (name[10]);
	case 11:
		if (day < 23)
			return (name[10]);
		else
			return (name[11]);
	case 12:
		if (day < 22)
			return (name[11]);
		else
			return (name[0]);
	}
	return ("¤£¸Ô");
}
