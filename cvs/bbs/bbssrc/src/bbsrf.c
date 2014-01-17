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
$Id: bbsrf.c,v 1.1 2000/01/15 01:45:27 edwardc Exp $
*/

#include "bbs.h"

#if defined(SOLARIS) || defined(IRIX)
#define USE_UTMPX
#include <utmpx.h>
#else
#define USE_UTMP
#include <utmp.h>
#endif

#ifdef SYSV
#include <sys/utsname.h>
#endif

#include <pwd.h>

/*-----    For rstat(), getting load average    ---*/
#if defined(BSD44)
#include <stdlib.h>

#elif defined(LINUX)
/* include nothing :-) */
#else

#include <rpcsvc/rstat.h>
#endif
/*-----------------------------------------------*/

#ifdef BBSRF_DEBUG
#define RF_DEBUG(str)  puts( str )
#else
#define RF_DEBUG(str)  {}
#endif

#ifdef BSD44
#define UTMP_PATH "/var/run/utmp"
#elif defined (LINUX)
#define UTMP_PATH "/var/run/utmp"
#else
#define UTMP_PATH "/etc/utmp"
#endif

int     max_load = 30;
char    bbs_prog_path[256];
char   *ttyname();
#ifdef USE_UTMPX
struct utmpx *
invis()
{
	static struct utmpx data;
	FILE   *fp;
	char   *name, *tp;
	struct passwd *pp;
	tp = ttyname(0);
	if (!tp)
		return NULL;
	tp = strchr(tp, '/') + 1;
	tp = strchr(tp, '/') + 1;
	pp = getpwuid(getuid());
	if (!pp) {
		fprintf(stderr, "You Don't exist!\n");
		exit(0);
	}
	name = pp->pw_name;

#ifdef INVISIBLE
	if ((fp = fopen(UTMPX_FILE, "r+")) == NULL) {
#else
	if ((fp = fopen(UTMPX_FILE, "r")) == NULL) {
#endif
		printf("Cannot open %s \n", UTMPX_FILE);
		exit(0);
	}
	while (read(fileno(fp), &data, sizeof(struct utmpx)) > 0) {
		if (data.ut_type != DEAD_PROCESS && !strcmp(tp, data.ut_line)) {
			struct utmpx nildata;
			memcpy(&nildata, &data, sizeof(nildata));
#ifdef INVISIBLE
			memset(nildata.ut_name, 0, 8);
			fseek(fp, (long) (ftell(fp) - sizeof(struct utmpx)), 0);
			if (write(fileno(fp), &nildata, sizeof(struct utmpx)) != sizeof(struct utmpx))
				 /* NIL IF STATEMENT */ ;
#endif
			fclose(fp);
			return &data;
		}
	}
	fclose(fp);
	return NULL;
}
#endif

#ifdef USE_UTMP
struct utmp *
invis()
{
	static struct utmp data;
	FILE   *fp;
	char   *tp;
	struct passwd *pp;
	tp = ttyname(0);
	if (!tp)
		return NULL;
	tp = strrchr(tp, '/') + 1;
	pp = getpwuid(getuid());
	if (!pp) {
		fprintf(stderr, "You Don't exist!\n");
		exit(0);
	}
#ifdef INVISIBLE
	if ((fp = fopen(UTMP_PATH, "r+")) == NULL) {
#else
	if ((fp = fopen(UTMP_PATH, "r")) == NULL) {
#endif
		printf("bbsrf: cannot open %s\n", UTMP_PATH);
		exit(0);
	}
	while (read(fileno(fp), &data, sizeof(struct utmp)) > 0) {
		if (!strcmp(tp, data.ut_line)) {
			struct utmp nildata;
			memcpy(&nildata, &data, sizeof(nildata));
#ifdef INVISIBLE
			memset(nildata.ut_name, 0, 8);
			fseek(fp, (long) (ftell(fp) - sizeof(struct utmp)), 0);
			write(fileno(fp), &nildata, sizeof(struct utmp));
#endif
			fclose(fp);
			return &data;
		}
	}
	fclose(fp);
	return NULL;
}
#endif

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

char   *env[] = {
	"TERM=xxxxxxxxxxxxxxxxxxxxxxxxxxx",
NULL};

void
set_max_load(prog_name)
char   *prog_name;
{
	char   *p;
	p = strrchr(prog_name, '.');

	if (p && *(p + 1) != '\0')
		max_load = atoi(p + 1);
}

int
check_ban_site(addr)
char   *addr;
{
	FILE   *fp;
	char    temp[STRLEN];
	if ((fp = fopen(".bansite", "r")) != NULL) {
		while (fgets(temp, STRLEN, fp) != NULL) {
			strtok(temp, " \n");
			if ((!strncmp(addr, temp, 16))
				|| (!strncmp(temp, addr, strlen(temp)) && temp[strlen(temp) - 1] == '.')
				|| (temp[0] == '.' && strstr(addr, temp) != NULL)) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}

int
main(argc, argv)
int     argc;
char   *argv[];
{
	int     uid;
	set_max_load(argv[0]);

	uid = getuid();

	if (uid == BBSUID) {	/* bbs uid */

#ifdef USE_UTMP
		struct utmp *whee;
#endif
#ifdef USE_UTMPX
		struct utmpx *whee;
#endif

		char    hid[17];
/* load control for BBS */
/*#ifdef LOAD_LIMIT */
		{
			double  cpu_load[3];
			int     load;
			get_load(cpu_load);
			load = cpu_load[0];
			printf("[1;36mBBS ³Ìªñ [33m(1,10,15)[36m ¤ÀÄÁªº¥­§¡­t²ü¤À§O¬°[33m %.2f, %.2f, %.2f [36m(¥Ø«e¤W­­ = %d).[0m\n\n",
				cpu_load[0], cpu_load[1], cpu_load[2], max_load);

			if (load < 0 || load > max_load) {
				printf("«Ü©êºp,¥Ø«e¨t²Î­t²ü¹L­«, ½Ðµy«á¦A¨Ó\n");
				sleep(load);
				exit(-1);
			}
		}
/*#endif*/
/* ppfoong */
		{
			char    buf[256];
			FILE   *fp;
			if ((fp = fopen("NOLOGIN", "r")) != NULL) {
				while (fgets(buf, 256, fp) != NULL)
					printf(buf);
				fclose(fp);
				sleep(3);
				exit(-1);
			}
		}
		RF_DEBUG("Before invis....");
		whee = invis();

		RF_DEBUG("Before Chroot....");

#ifdef BBSRF_CHROOT
		sprintf(bbs_prog_path, "/bin/bbs", BBSHOME);
		if (chroot(BBSHOME) != 0) {
			printf("Cannot chroot, exit!\n");
			exit(-1);
		}
#else
		sprintf(bbs_prog_path, "%s/bin/bbs", BBSHOME);
#endif

		RF_DEBUG("Before setuid....");
		setuid(uid);

		if (whee) {
			char    ttybuf[16];
			char   *tp;
			RF_DEBUG("Before ttyname (whee)....");
			tp = ttyname(0);
			strcpy(ttybuf, (tp == NULL) ? "/dev/ttyp0" : tp);

			if (whee->ut_host[0])
				strncpy(hid, whee->ut_host, 16);
			else
#ifdef SYSV
			{
				struct utsname name;
				if (uname(&name) >= 0)
					strcpy(hid, name.nodename);
				else
					strcpy(hid, "localhost");
			}
#else
				gethostname(hid, 16);
#endif

			hid[16] = '\0';

			if (check_ban_site(hid))
				exit(-1);
			RF_DEBUG("Before execl (whee)....");

			execl(bbs_prog_path, "bbs", "h", hid, ttybuf, NULL);
		} else {
			RF_DEBUG("Before execl (not whee)....");
			execl(bbs_prog_path, "bbs", "h", "unknown", "notty", NULL);
		}

		printf("execl failed\n");
		exit(-1);
	}
	setuid(uid);
	printf("UID DOES NOT MATCH\n");
	exit(-1);
	return -1;
}
