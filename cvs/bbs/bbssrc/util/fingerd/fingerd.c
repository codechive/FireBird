/*
 * Copyright (c) 1983, 1993 The Regents of the University of California.  All
 * rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char     copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
        The Regents of the University of California.  All rights reserved.\n";
#endif				/* not lint */

#ifndef lint
/*
 * static char sccsid[] = "@(#)fingerd.c   8.1 (Berkeley) 6/4/93";
 */
static const char rcsid[] =
"$Id: fingerd.c,v 1.1 2000/01/15 01:45:33 edwardc Exp $";
#endif				/* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#ifndef DEBUG
#include <syslog.h>
#endif
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "pathnames.h"

#ifdef SYSV
#include <sys/termios.h>
#endif

#ifdef SYSV
void logerr(const char *, ...);
#else
void logerr     __P((const char *,...));
#endif

/* woju: standalone start */
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#define PIDFILE  "/var/run/fingerd.pid"
#define TCP_QLEN        5
static struct sockaddr_in from;
static int      rfds;		/* read file descriptor set */

/* some define for FirebirdBBS Family (edwardc) */

struct UTMPFILE *utmpshm;
struct UCACHE  *uidshm;
int             usernumber;
char            buf[80];

#define UTMP_SHMKEY		30020
#define UCACHE_SHMKEY	30010
#define CHROOT			/* will chroot and setuid/setgid */

#ifdef DEBUG
#define         LOG_ERR 0
#define         LOG_DAEMON 0
#define         LOG_PID 0
#define         LOG_INFO 0
#define         LOG_NOTICE 0
#endif

/* ----------------------------------------------------- */
/* daemon : initialize and other stuff                   */
/* ----------------------------------------------------- */

start_daemon()
{
	int             n, on;
	char            buf[80];
	struct sockaddr_in fsin;


	/*
	 * More idiot speed-hacking --- the first time conversion makes the C
	 * library open the files containing the locale definition and time
	 * zone. If this hasn't happened in the parent process, it happens in
	 * the children, once per connection --- and it does add up.
	 */

	time_t          dummy = time(NULL);
	struct tm      *dummy_time = localtime(&dummy);
	struct tm      *other_dummy_time = gmtime(&dummy);
	strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

	n = getdtablesize();

	if (fork())
		exit(0);

	while (n)
		(void) close(--n);

	n = open(PIDFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (n >= 0) {
		sprintf(buf, "%5d\n", getpid());
		write(n, buf, 6);
		close(n);
	} else {
		n = open("/tmp/fingerd.pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (n >= 0) {
			sprintf(buf, "%5d\n", getpid());
			write(n, buf, 6);
			close(n);
		}
	}
	(void) open("/dev/null", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);

	if ((n = open("/dev/tty", O_RDWR)) > 0) {
		ioctl(n, TIOCNOTTY, 0);
		close(n);
	}
	/*
	 * openlog("ftpd", LOG_PID | LOG_ODELAY, LOG_AUTH);
	 */
#ifndef DEBUG
	openlog("fingerd", LOG_PID | LOG_CONS, LOG_DAEMON);
#endif
	syslog(LOG_INFO, "start");
}


int
bind_port(port)
	int             port;
{
	int             sock, on;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		syslog(LOG_NOTICE, "socket\n");
		exit(1);
	}
	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
		syslog(LOG_ERR, "(SO_REUSEADDR): %m");
	on = 0;

#if 0				/* 0825 */
	on = 4096;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &on, sizeof(on));
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &on, sizeof(on));
#endif

	from.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) & from, sizeof from) < 0) {
		syslog(LOG_INFO, "bind_port can't bind to %d", port);
		exit(1);
	}
	if (listen(sock, TCP_QLEN) < 0) {
		syslog(LOG_INFO, "bind_port can't listen to %d", port);
		exit(1);
	}
	FD_SET(sock, (fd_set *) & rfds);
	return sock;
}

void
reapchild()
{
	int             state, pid;

	/* signal(SIGCHLD, reapchild); */
	while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0);
}

/* ----------------------------------------------- */
/* ¨ú±o remote user name ¥H§P©w¨­¥÷                */
/* ----------------------------------------------- */

/*
 * rfc931() speaks a common subset of the RFC 931, AUTH, TAP, IDENT and RFC
 * 1413 protocols. It queries an RFC 931 etc. compatible daemon on a remote
 * host to look up the owner of a connection. The information should not be
 * used for authentication purposes. This routine intercepts alarm signals.
 * 
 * Diagnostics are reported through syslog(3).
 * 
 * Author: Wietse Venema, Eindhoven University of Technosyslogy, The
 * Netherlands.
 */

#include <setjmp.h>

#define STRN_CPY(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }
#define STRING_LENGTH    60
#define RFC931_TIMEOUT   10
#define RFC931_PORT     113	/* Semi-well-known port */
#define ANY_PORT        0	/* Any old port will do */


/* ------------------------- */
/* timeout - handle timeouts */
/* ------------------------- */


char           *inet_ntoa();
static jmp_buf  timebuf;

static void
gtimeout(sig)
	int             sig;
{
	longjmp(timebuf, sig);
}


void
getremotename(from, rhost, rname)
	struct sockaddr_in *from;
	char           *rhost;
	char           *rname;
{
	struct sockaddr_in our_sin;
	struct sockaddr_in rmt_sin;
	unsigned        rmt_port, rmt_pt;
	unsigned        our_port, our_pt;
	FILE           *fp;
	char            buffer[512], user[40], *cp;
	int             s;
	struct hostent *hp;

	/* get remote host name */

	hp = NULL;
	if (setjmp(timebuf) == 0) {
		signal(SIGALRM, gtimeout);
		alarm(3);
		hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr),
				   from->sin_family);
		alarm(0);
	}
	strncpy(rhost, hp ? hp->h_name : (char *) inet_ntoa(from->sin_addr), 39);
	rhost[39] = 0;

	/*
	 * Use one unbuffered stdio stream for writing to and for reading
	 * from the RFC931 etc. server. This is done because of a bug in the
	 * SunOS 4.1.x stdio library. The bug may live in other stdio
	 * implementations, too. When we use a single, buffered,
	 * bidirectional stdio stream ("r+" or "w+" mode) we read our own
	 * output. Such behaviour would make sense with resources that
	 * support random-access operations, but not with sockets.
	 */

	s = sizeof our_sin;
	if (getsockname(0, (struct sockaddr *) & our_sin, &s) < 0)
		return;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("bbsd: socket in rfc931");
		return;
	}
	if (!(fp = fdopen(s, "r+"))) {
		close(s);
		perror("bbsd:fdopen");
		return;
	}
	/*
	 * Set up a timer so we won't get stuck while waiting for the server.
	 */

	if (setjmp(timebuf) == 0) {
		signal(SIGALRM, gtimeout);
		alarm(RFC931_TIMEOUT);

		/*
		 * Bind the local and remote ends of the query socket to the
		 * same IP addresses as the connection under investigation.
		 * We go through all this trouble because the local or remote
		 * system might have more than one network address. The
		 * RFC931 etc. client sends only port numbers; the server
		 * takes the IP addresses from the query socket.
		 */

		our_pt = ntohs(our_sin.sin_port);
		our_sin.sin_port = htons(ANY_PORT);

		rmt_sin = *from;
		rmt_pt = ntohs(rmt_sin.sin_port);
		rmt_sin.sin_port = htons(RFC931_PORT);

		setbuf(fp, (char *) 0);
		s = fileno(fp);

		if (bind(s, (struct sockaddr *) & our_sin, sizeof(our_sin)) >= 0 &&
		    connect(s, (struct sockaddr *) & rmt_sin, sizeof(rmt_sin)) >= 0) {

			/*
			 * Send query to server. Neglect the risk that a
			 * 13-byte write would have to be fragmented by the
			 * local system and cause trouble with buggy System V
			 * stdio libraries.
			 */

			fprintf(fp, "%u,%u\r\n", rmt_pt, our_pt);
			fflush(fp);

			/*
			 * Read response from server. Use fgets()/sscanf() so
			 * we can work around System V stdio libraries that
			 * incorrectly assume EOF when a read from a socket
			 * returns less than requested.
			 */

			if (fgets(buffer, sizeof(buffer), fp) && !ferror(fp) && !feof(fp)
			  && sscanf(buffer, "%u , %u : USERID :%*[^:]:%39s",
				    &rmt_port, &our_port, user) == 3
			    && rmt_pt == rmt_port && our_pt == our_port) {

				/*
				 * Strip trailing carriage return. It is part
				 * of the protocol, not part of the data.
				 */

				if (cp = strchr(user, '\r'))
					*cp = 0;
				strcpy(rname, user);
			}
		}
		alarm(0);
	}
	fclose(fp);
}

#define _MODES_C_

#include "bbs.h"

bbsulist()
{
	struct user_info *currentuser;
	int             i, user_num;

	printf("\
¨Ï¥ÎªÌ¥N¸¹     ¼ÊºÙ                    °ÊºA            ¨Ó¦Û        \n\
============ ======================== ========== ======================\n");
	resolve_utmp();
	for (i = user_num = 0; i < USHM_SIZE; i++) {
		currentuser = &(utmpshm->uinfo[i]);

		if (currentuser->userid[0] && currentuser->pid
		    && !currentuser->invisible) {
			printf("%-13s%-25s%-11s%s\n",
			       currentuser->userid, currentuser->username,
			    ModeType(currentuser->mode), currentuser->from);
			user_num++;
		}
	}

	printf("\
============ ======================== ========== ======================\n");
	printf("¡i %s ¡j ¯¸¤W²{¦³¤H¼Æ = %d ¤H\n", BBSNAME, user_num);

}

void
date_string(str, date)
	char           *str;
	time_t         *date;
{
	struct tm      *t = localtime(date);
	static char     week[] = "¤é¤@¤G¤T¥|¤­¤»";

	sprintf(str, "%d¦~%2d¤ë%2d¤é%3d:%02d:%02d ¬P´Á%.2s",
		t->tm_year - 11, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec, &week[t->tm_wday << 1]);
}

/* edwardc */
char           *
mail_string(char *qry_mail_dir)
{
	struct fileheader fh;
	struct stat     st;
	int             fd;
	register int    offset;
	register long   numfiles;
	unsigned char   ch;
	static char    *answer[] = {"  ", "¡ó"};

	offset = (int) ((char *) &(fh.accessed[0]) - (char *) &(fh));
	if ((fd = open(qry_mail_dir, O_RDONLY)) < 0)
		return answer[0];
	fstat(fd, &st);
	numfiles = st.st_size;
	numfiles = numfiles / sizeof(fh);
	if (numfiles <= 0) {
		close(fd);
		return answer[0];
	}
	lseek(fd, (st.st_size - (sizeof(fh) - offset)), SEEK_SET);
	/* Â÷½u¬d¸ß·s«H¥u­n¬d¸ß³Ì«á¤@«Ê¬O§_¬°·s«H¡A¨ä¥L¨Ã¤£­«­n */
	/* Modify by SmallPig */
	read(fd, &ch, 1);
	if (!(ch & FILE_READ)) {
		close(fd);
		return answer[1];
	}
	close(fd);
	return answer[0];
}

void
report()
{
}

void
attach_err(shmkey, name, err)
	int             shmkey;
	char           *name;
	int             err;
{
	char           buf[80];

	sprintf(buf, "Error! %s error #%d! key = %x (%d).\n", name, err, shmkey, shmkey);
#ifndef DEBUG
	syslog(1, buf);
#else
	write(1, buf, strlen(buf));
#endif

	exit(1);
}

void           *
attach_shm(shmstr, defaultkey, shmsize)
	char           *shmstr;
	int             defaultkey, shmsize;
{
	void           *shmptr;
	int             shmkey, shmid;

	shmkey = defaultkey;

	if (shmkey < 1024)
		shmkey = defaultkey;
	shmid = shmget(shmkey, shmsize, 0);
	if (shmid < 0) {
		/* ¨S¦³ shm ¥i¥H§ì´N¤£§ì¤F, ¦]¬°¤]³\ bbs ÁÙ¨S±Ò°Ê¹L */
		attach_err(shmkey, "no shm avaliable!", errno);
	} else {
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat", errno);
	}
	return shmptr;
}

int
fillucache(uentp)
	struct userec  *uentp;
{
	if (usernumber < MAXUSERS) {
		strncpy(uidshm->userid[usernumber], uentp->userid, IDLEN + 1);
		uidshm->userid[usernumber++][IDLEN] = '\0';
	}
	return 0;
}

int
searchuser(userid)
	char           *userid;
{
	register int    i;

	resolve_ucache();
	for (i = 0; i < uidshm->number; i++)
		if (!ci_strncmp(userid, uidshm->userid[i], IDLEN + 1))
			return i + 1;
	return 0;
}

int
ci_strncmp(s1, s2, n)
	register char  *s1, *s2;
	register int    n;
{
	char            c1, c2;

	while (n-- > 0) {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 >= 'a' && c1 <= 'z')
			c1 += 'A' - 'a';
		if (c2 >= 'a' && c2 <= 'z')
			c2 += 'A' - 'a';
		if (c1 != c2)
			return (c1 - c2);
		if (c1 == 0)
			return 0;
	}
	return 0;
}


int
resolve_ucache()
{
	struct stat     st;
	int             ftime;

	if (uidshm == NULL) {
		uidshm = attach_shm("UCACHE_SHMKEY", UCACHE_SHMKEY, sizeof(*uidshm));
	}
	if (stat(FLUSH, &st) < 0) {
		st.st_mtime++;
	}
	ftime = st.st_mtime;
	if (uidshm->uptime < ftime) {
		/* uidshm->uptime = ftime; */
		usernumber = 0;
		apply_record(PASSFILE, fillucache, sizeof(struct userec));
		uidshm->number = usernumber;
		uidshm->uptime = ftime;
	}
}

int
resolve_utmp()
{
	if (utmpshm == NULL) {
		utmpshm = attach_shm("UTMP_SHMKEY", UTMP_SHMKEY, sizeof(*utmpshm));
	}
}

char           *
cexp(exp)
	int             exp;
{
	int             expbase = 0;

	if (exp == -9999)
		return "¨Sµ¥¯Å";
	if (exp <= 100 + expbase)
		return "·s¤â¤W¸ô";
	if (exp > 100 + expbase && exp <= 450 + expbase)
		return "¤@¯ë¯¸¤Í";
	if (exp > 450 + expbase && exp <= 850 + expbase)
		return "¤¤¯Å¯¸¤Í";
	if (exp > 850 + expbase && exp <= 1500 + expbase)
		return "°ª¯Å¯¸¤Í";
	if (exp > 1500 + expbase && exp <= 2500 + expbase)
		return "¦Ñ¯¸¤Í";
	if (exp > 2500 + expbase && exp <= 3000 + expbase)
		return "ªø¦Ñ¯Å";
	if (exp > 3000 + expbase && exp <= 5000 + expbase)
		return "¥»¯¸¤¸¦Ñ";
	if (exp > 5000 + expbase)
		return "¶}°ê¤j¦Ñ";

}

char           *
cperf(perf)
	int             perf;
{

	if (perf == -9999)
		return "¨Sµ¥¯Å";
	if (perf <= 5)
		return "»°§Ö¥[ªo";
	if (perf > 5 && perf <= 12)
		return "§V¤O¤¤";
	if (perf > 12 && perf <= 35)
		return "ÁÙ¤£¿ù";
	if (perf > 35 && perf <= 50)
		return "«Ü¦n";
	if (perf > 50 && perf <= 90)
		return "Àuµ¥¥Í";
	if (perf > 90 && perf <= 140)
		return "¤ÓÀu¨q¤F";
	if (perf > 140 && perf <= 200)
		return "¥»¯¸¤ä¬W";
	if (perf > 200)
		return "¯«¡ã¡ã";

}

int
countexp(udata)
	struct userec  *udata;
{
	int             exp;

	if (!strcmp(udata->userid, "guest"))
		return -9999;
	exp = udata->numposts + udata->numlogins / 5 + (time(0) - udata->firstlogin) / 86400 + udata->stay / 3600;
	return exp > 0 ? exp : 0;
}

int
countperf(udata)
	struct userec  *udata;
{
	int             perf;
	int             reg_days;

	if (!strcmp(udata->userid, "guest"))
		return -9999;
	reg_days = (time(0) - udata->firstlogin) / 86400 + 1;
	perf = ((float) (udata->numposts) / (float) udata->numlogins +
		(float) udata->numlogins / (float) reg_days) * 10;
	return perf > 0 ? perf : 0;
}

int
compute_user_value(urec)
	struct userec  *urec;
{
	int             value;

	/* if (urec) has XEMPT permission, don't kick it */
	if ((urec->userlevel & PERM_XEMPT) || strcmp(urec->userid, "guest") == 0)
		return 999;
	value = (time(0) - urec->lastlogin) / 60;	/* min */
	/* new user should register in 30 mins */
	if (strcmp(urec->userid, "new") == 0) {
		return (30 - value) * 60;
	}
	if (urec->numlogins <= 3)
		return (15 * 1440 - value) / 1440;
	if (!(urec->userlevel & PERM_LOGINOK))
		return (30 * 1440 - value) / 1440;
	if (urec->stay > 1000000)
		return (365 * 1440 - value) / 1440;
	return (120 * 1440 - value) / 1440;
}

bfinger(char *userid)
{
	int             unum;
	int             i, b;
	FILE           *fp;
	char           *str, *mailstr, *modestr, datestr[40], fpath[128];
	struct user_info *currentuser;
	struct userec   acct;

	if (!userid || !*userid)
		return bbsulist();
	if ((unum = searchuser(userid)) &&
	    (fp = fopen(".PASSWDS", "r"))) {

		--unum;
		sprintf(fpath, "mail/%c/%s", toupper(userid[0]), userid);
		/* ¬O§_¦³·s«H¥óÁÙ¨S¬Ý¡H */

		strcat(fpath, "/.DIR");
		str = strrchr(fpath, '.');
		mailstr = mail_string(fpath);

		/* ¬O§_¦b½u¤W¡H */

		modestr = "¤£¦b¯¸¤W";
		for (i = 0; i < USHM_SIZE; i++) {
			currentuser = &(utmpshm->uinfo[i]);
			if (!strcmp(userid, currentuser->userid) && !currentuser->invisible) {
				modestr = (char *) ModeType(currentuser->mode);
				break;
			}
		}

		fseek(fp, unum * sizeof(acct), 0);
		fread(&acct, sizeof(acct), 1, fp);
		fclose(fp);

		date_string(datestr, &acct.lastlogin);

		printf("%s (%s) ¦@¤W¯¸ %d ¦¸¡Aµoªí¹L %d ½g¤å³¹\n", acct.userid, acct.username, acct.numlogins, acct.numposts);
		printf("¤W¦¸¦b [%s] ±q [%s] ¨ì¥»¯¸¤@¹C¡C\n", datestr, acct.lasthost);
		printf("«H½c¡G[%2s]¡A¸gÅç­È¡G[%d](%s) ªí²{­È¡G[%d](%s) ¥Í©R¤O¡G[%d]¡C\n"
		       ,mailstr, countexp(&acct), cexp(countexp(&acct)), countperf(&acct), cperf(countperf(&acct)), compute_user_value(&acct));

		if (strcmp(modestr, "¤£¦b¯¸¤W") != 0)
			printf("¥Ø«e¦b¯¸¤W¡A¥Ø«eª¬ºA¡G %s\n", modestr);
		else
			printf("\n");

		/* Åã¥Ü [¦W¤ù/­pµeÀÉ] */

		strcpy(str, "plans");
		sprintf(fpath, "home/%c/%s/plans", toupper(userid[0]), userid);
		if (fp = fopen(fpath, "r")) {
			char            line[200];

			printf("­Ó¤H»¡©úÀÉ¦p¤U¡G\n");
			i = 6;	/* Jul 19 ­×§ï¹ï¥I¶W¤j­p¹ºÀÉ */
			while (fgets(line, 200, fp)) {
				/* 990327.edwardc ­×¥¿¦C¦L¥X % ·|¦³°ÝÃDªº±¡§Î */
				fputs(line, stdout);
				if (i < 21)
					i++;
				else
					break;
			}

			fclose(fp);

			printf("[0m");	/* ´N¬O¦³¤H¤£¥ÎÁÙ­ì½X Jul 19 */

		} else {
			printf("¨S¦³­Ó¤H»¡©úÀÉ\n");
		}
	} else {
		printf("no such user %s\n", userid);
		exit(1);
	}
}


bdaemon(int argc, char **argv)
{
	int             msock, nfds, csock, ofds, slot;
	struct timeval  tv;
	pid_t           pid;

	start_daemon();
	signal(SIGCHLD, reapchild);
	memset(&from, 0, sizeof(from));
	from.sin_family = AF_INET;
	rfds = 0;
	if (argc > 1) {
		msock = -1;
		for (nfds = 1; nfds < argc; nfds++) {
			csock = atoi(argv[nfds]);
			if (csock > 0)
				msock = bind_port(csock);
			else
				break;
		}
		if (msock < 0) {
			syslog(LOG_INFO, "bdaemon started with invalid arguments (no port)");
			exit(1);
		}
	} else {
		static int      ports[] = {79	/* , 3456 , 4001 , 4002,
		    4003, 4004, 4005 */ };

		for (nfds = 0; nfds < sizeof(ports) / sizeof(int); nfds++) {
			msock = bind_port(ports[nfds]);
		}
	}

	/*
	 * Jul 17: by edwardc ¤£ª¾«ç»ò¼Ë³o¤@¬qªº code ¦ü¥G¦³°ÝÃD .. so
	 * ¥u¦n°¨°_¨Ó, ¨S¦³ chroot §Æ±æ¤£·|¦³°ÝÃD¤~¦n §ï¥Î chdir
	 */

#ifdef  CHROOT
	if (chroot(BBSHOME) != 0) {
		perror("chroot");
		exit(1);
	}
	chdir("/");

	setgid(BBSGID);
	initgroups("bbs", BBSGID);
	setuid(BBSUID);
#else
	chdir(BBSHOME);
#endif

	{
		char            userid[IDLEN + 1] = "sysop";

		searchuser(userid);
		resolve_utmp();
	}


	/* main loop */
	ofds = rfds;
	nfds = msock + 1;

	for (;;) {
forever:
		rfds = ofds;
		tv.tv_sec = 60 * 30;
		tv.tv_usec = 0;
		msock = select(nfds, (fd_set *) & rfds, NULL, NULL, &tv);

		if (msock <= 0)
			continue;

		msock = 0;
		csock = 1;
		for (;;) {
			if (csock & rfds)
				break;
			if (++msock >= nfds)
				goto forever;
			csock <<= 1;
		}

		slot = sizeof(from);
		do {
			csock = accept(msock, (struct sockaddr *) & from, (int *) &slot);
		} while (csock < 0 && errno == EINTR);

		if (csock < 0)
			continue;

		pid = fork();

		if (!pid) {
			int             len;
			struct sockaddr_in our;
			char            rhost[40];
			char            rname[40] = "?";
			char            rfc931[200];
			int             port;

			dup2(csock, 0);
			close(msock);
			getremotename(&from, rhost, rname);	/* RFC931 */
			sprintf(rfc931, "%s@%s", rname, rhost);
			syslog(LOG_INFO, "connect from %s", rfc931);

#ifndef SYSV
			setenv("REMOTEHOST", rhost, 1);
			setenv("REMOTENAME", rname, 1);
			setenv("RFC931", rfc931, 1);
#endif

			/*
			 * woju
			 */
			len = sizeof(our);
			if (getsockname(0, (struct sockaddr *) & our, &len) < 0) {
				syslog(LOG_INFO, "getsockname: %s", strerror(errno));
				exit(1);
			}
			/*
			 * connect_port = ntohs(our.sin_port);
			 */

			while (--nfds >= 0)
				close(nfds);
			dup2(csock, 0);
			dup2(csock, 1);
			return;
		} else
			close(csock);
	}
	syslog(LOG_INFO, "NOTREACHED(bdaemon) reached!?");
	/* NOTREACHED */
}

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	register FILE  *fp;
	register int    ch;
	register char  *lp;
	struct hostent *hp;
	struct sockaddr_in sin;
	int             p[2], syslogging, secure, sval;
	int             i;
#define ENTRIES 50
	char          **ap, *av[ENTRIES + 1], **comp, line[1024], *prog;

	prog = _PATH_FINGER;
	syslogging = secure = 0;
#if 0
woju:	standalone
		opensyslog("fingerd", LOG_PID | LOG_CONS, LOG_DAEMON);
	opterr = 0;
	while ((ch = getopt(argc, argv, "slp:")) != EOF)
		switch (ch) {
		case 'l':
			syslogging = 1;
			break;
		case 'p':
			prog = optarg;
			break;
		case 's':
			secure = 1;
			break;
		case '?':
		default:
			logerr("illegal option -- %c", ch);
		}
	if (syslogging) {
		sval = sizeof(sin);
		if (getpeername(0, (struct sockaddr *) & sin, &sval) < 0)
			logerr("getpeername: %s", strerror(errno));
		if (hp = gethostbyaddr((char *) &sin.sin_addr.s_addr,
				       sizeof(sin.sin_addr.s_addr), AF_INET))
			lp = hp->h_name;
		else
			lp = inet_ntoa(sin.sin_addr);
		syslog(LOG_NOTICE, "query from %s", lp);
	}
woju:	standalone
#endif
		secure = 1;
	bdaemon(argc, argv);

	/*
	 * Enable server-side Transaction TCP.
	 */
	{
		int             one = 1;
		/*
		 * if (setsockopt(STDOUT_FILENO, IPPROTO_TCP, TCP_NOPUSH,
		 * &one, sizeof one) < 0) { logerr("setsockopt(TCP_NOPUSH)
		 * failed: %m"); }
		 */
		if (setsockopt(STDOUT_FILENO, IPPROTO_TCP, SO_REUSEADDR, (char *) &one,
			       sizeof one) < 0) {
			logerr("setsockopt(TCP_NOPUSH) failed: %m", one);
		}
	}

	if (!fgets(line, sizeof(line), stdin))
		exit(1);

	comp = &av[1];
	av[2] = "--";
	for (lp = line, ap = &av[3];;) {
		*ap = strtok(lp, " \t\r\n");
		if (!*ap) {
			if (secure && ap == &av[3]) {
				exit(bfinger(0));
				/*
				 * woju, call bbs-finger puts("must provide
				 * username\r\n"); exit(1);
				 */
			}
			break;
		}
		if (secure && strchr(*ap, '@')) {
			puts("forwarding service denied\r\n");
			exit(1);
		}
		/* RFC742: "/[Ww]" == "-l" */
		if ((*ap)[0] == '/' && ((*ap)[1] == 'W' || (*ap)[1] == 'w')) {
			av[1] = "-l";
			comp = &av[0];
		} else if (++ap == av + ENTRIES)
			break;
		lp = NULL;
	}

	if (lp = strrchr(prog, '/'))
		*comp = ++lp;
	else
		*comp = prog;

	/*
	 * woju: bbs-finger
	 */
	{
		char           *s, userid[IDLEN + 1];
		int             n;

		s = strcmp(comp[2], "--") ? comp[2] : comp[3];
		strncpy(userid, s, IDLEN);
		userid[IDLEN] = 0;
		strtok(userid, ".");
		return bfinger(userid);
	}


}

#ifdef SYSV

void logerr()
{}

#else

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
logerr(const char *fmt,...)
#else
logerr(fmt, va_alist)
	char           *fmt;
va_dcl
#endif
{
	va_list         ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void) vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
	/* NOTREACHED */
}

#endif

#ifdef DEBUG
/* by edwardc for DEBUGGING! */
int
syslog(int dummy, char *mesg)
{
	FILE           *fp;

	fp = fopen("/dev/console", "w");

	if (fp != NULL)
		fprintf(fp, "fingerd[%d]: %s\n", getppid(), mesg);
	else
		fprintf(stderr, "fingerd[%d]: %s\n", getppid(), mesg);

	fclose(fp);
}
#endif
