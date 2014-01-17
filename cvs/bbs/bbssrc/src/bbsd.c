/*
 * bbsd.c		-- bbs daemon for Firebird BBS 3.0
 *
 * A part of Firebird BBS 3.0 Project
 *
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@firebird.dhs.org>
 * All rights reserved.
 *
 * ** use metrials from mbbsd of Maple BBS 3.00 **
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
 * CVS: $Id: bbsd.c,v 1.11 2002/07/31 10:39:04 biboman Exp $
 */
/*-------------------------------------------------------*/
/* mbbsd.c      ( NTHU CS MapleBBS Ver 3.00 )            */
/*-------------------------------------------------------*/
/* target : BBS daemon/main/login/top-menu routines      */
/* create : 95/03/29                                     */
/* update : 96/10/10                                     */
/*-------------------------------------------------------*/

#include "bbs.h"

#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/telnet.h>

#ifdef SYSV
#include <sys/termios.h>
#else
#include <termios.h>
#endif

#ifdef LINUX
#include <sys/ioctl.h>
#endif

#define HAVE_CHKLOAD
#define SOCKET_QLEN     4

#define TH_LOW          10
#define TH_HIGH         15

#define PID_FILE		BBSHOME"/log/bbsd.pid"
#define LOG_FILE		BBSHOME"/log/bbsd.log"
#define BAD_HOST		BBSHOME"/.bansite"
#define NOLOGIN			BBSHOME"/NOLOGIN"

#ifdef  HAVE_CHKLOAD
#define BANNER  "\r\nFirebird BBS 3.0 [bbsd ready]\r\n[1;36mBBS ³Ìªñ [33m(1,10,15)[36m ¤ÀÄÁªº¥­§¡­t²ü¬°[33m %s [36m(¤W­­ = %d) [%s] .[0m\r\n"
#else
#define BANNER  "\r\nFirebird BBS 3.0 [bbsd ready]\r\n"BBSNAME" ["BBSHOST"] ("BBSIP")\r\n\n"
#endif

jmp_buf byebye;

char    remoteusername[40] = "?";
extern  char    fromhost[60];
char    genbuf[1024];
char	loadstr[1024];
char    status[64];

const char bbsd_c[] =
	"$Id: bbsd.c,v 1.11 2002/07/31 10:39:04 biboman Exp $";
	
/* ----------------------------------------------------- */
/* FSA (finite state automata) for telnet protocol       */
/* ----------------------------------------------------- */


static void
telnet_init()
{
	static char svr[] = {
		IAC, DO, TELOPT_TTYPE,
		IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE,
		IAC, WILL, TELOPT_ECHO,
		IAC, WILL, TELOPT_SGA
	};

	register int n, len;
	register char *cmd, *data;
	int     rset;
	struct timeval to;
	char    buf[256];
	
	data = buf;
#ifdef HAVE_CHKLOAD
	sprintf(data, BANNER, loadstr, TH_LOW, status);
#else
	sprintf(data, BANNER);
#endif
	write(0, data, strlen(data));

#if 0
	to.tv_sec = 1;
	rset = to.tv_usec = 0;
	FD_SET(0, (fd_set *) & rset);

	for (n = 0, cmd = svr; n < 4; n++) {
		len = (n == 1 ? 6 : 3);
		write(0, cmd, len);
		cmd += len;

		if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0) {
			read(0, data, sizeof(buf));
		}
		rset = oset;
	}
#else
  
    for (n = 0, cmd = svr; n < 4; n++)
	{
		len = (n == 1 ? 6 : 3);
		send(0, cmd, len, 0);
		cmd += len;
                  
		rset = 1;
		/* Thor.981221: for future reservation bug */
		to.tv_sec = 1;
		to.tv_usec = 1;
		if (select(1, (fd_set *) & rset, NULL, NULL, &to) > 0)
			recv(0, buf, sizeof(buf), 0);
	}
	
#endif                                              
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
 * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
 */

#include <setjmp.h>

#define STRN_CPY(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }
#define RFC931_TIMEOUT   5
#define RFC931_PORT     113	/* Semi-well-known port */
#define ANY_PORT        0	/* Any old port will do */


/* ------------------------- */
/* timeout - handle timeouts */
/* ------------------------- */

static void
timeout(sig)
int     sig;
{
	(void) longjmp(byebye, sig);
}

#ifndef DISABLE_RFC931
static void
getremotename(from, rhost, rname)
struct sockaddr_in *from;
char   *rhost;
char   *rname;
{
	struct sockaddr_in our_sin;
	struct sockaddr_in rmt_sin;
	unsigned rmt_port, rmt_pt;
	unsigned our_port, our_pt;
	FILE   *fp;
	char    buffer[512], user[80], *cp;
	int     s;
	struct hostent *hp;
	/* get remote host name */

	hp = NULL;
	if (setjmp(byebye) == 0) {
		(void) signal(SIGALRM, timeout);
		(void) alarm(3);
		hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr),
			from->sin_family);
		(void) alarm(0);
	}
	(void) strcpy(rhost, hp ? hp->h_name : (char *) inet_ntoa(from->sin_addr));
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
		return;
	}
	if (!(fp = fdopen(s, "r+"))) {
		(void) close(s);
		return;
	}
	/*
	 * Set up a timer so we won't get stuck while waiting for the server.
	 */

	if (setjmp(byebye) == 0) {
		(void) signal(SIGALRM, timeout);
		(void) alarm(RFC931_TIMEOUT);
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

		(void) setbuf(fp, (char *) 0);
		s = fileno(fp);

		if (bind(s, (struct sockaddr *) & our_sin, sizeof(our_sin)) >= 0 &&
			connect(s, (struct sockaddr *) & rmt_sin, sizeof(rmt_sin)) >= 0) {
			/*
			 * Send query to server. Neglect the risk that a
			 * 13-byte write would have to be fragmented by the
			 * local system and cause trouble with buggy System V
			 * stdio libraries.
			 */

			(void) fprintf(fp, "%u,%u\r\n", rmt_pt, our_pt);
			(void) fflush(fp);
			/*
			 * Read response from server. Use fgets()/sscanf() so
			 * we can work around System V stdio libraries that
			 * incorrectly assume EOF when a read from a socket
			 * returns less than requested.
			 */

			if (fgets(buffer, sizeof(buffer), fp) && !ferror(fp) && !feof(fp)
				&& sscanf(buffer, "%u , %u : USERID :%*[^:]:%79s",
					&rmt_port, &our_port, user) == 3
				&& rmt_pt == rmt_port && our_pt == our_port) {
				/*
				 * Strip trailing carriage return. It is part
				 * of the protocol, not part of the data.
				 */

				if (cp = (char *) strchr(user, '\r'))
					*cp = 0;
				strcpy(rname, user);
			}
		}
		(void) alarm(0);
	}
	(void) fclose(fp);
}
#else
static void
getremotename(from, rhost, rname)
struct sockaddr_in *from;
char   *rhost;
char   *rname;
{
	struct hostent *hp;

	/* get remote host name */
	hp = NULL;
	if (setjmp(byebye) == 0) {
		(void) signal(SIGALRM, timeout);
		(void) alarm(3);
		hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr),
			from->sin_family);
		(void) alarm(0);
	}
	(void) strcpy(rhost, hp ? hp->h_name : (char *) inet_ntoa(from->sin_addr));
	(void) strcpy(rname, "");
}
#endif 

/* ----------------------------------- */
/* check system / memory / CPU loading */
/* ----------------------------------- */

#ifdef  HAVE_CHKLOAD
int     fkmem;

int
chkload(limit)
int     limit;
{
	double  cpu_load[3];
	register int i;
#ifdef BSD44
	getloadavg(cpu_load, 3);
#elif defined(LINUX)
	FILE   *fp;
	fp = fopen("/proc/loadavg", "r");

	if (!fp)
		cpu_load[0] = cpu_load[1] = cpu_load[2] = 0;
	else {
		float   av[3];
		fscanf(fp, "%g %g %g", av, av + 1, av + 2);
		fclose(fp);
		cpu_load[0] = av[0];
		cpu_load[1] = av[1];
		cpu_load[2] = av[2];
	}
#else

#include <nlist.h>

#ifdef SOLARIS

#define VMUNIX  "/dev/ksyms"
#define KMEM    "/dev/kmem"

	static struct nlist nlst[] = {
		{"avenrun"},
		{0}
	};
#else

#define VMUNIX  "/vmunix"
#define KMEM    "/dev/kmem"

	static struct nlist nlst[] = {
		{"_avenrun"},
		{0}
	};
#endif

	long    avenrun[3];
	static long offset = -1;
	int     kmem;
	if ((kmem = open(KMEM, O_RDONLY)) == -1)
		return (1);

	if (offset < 0) {
		(void) nlist(VMUNIX, nlst);
		if (nlst[0].n_type == 0)
			return (1);
		offset = (long) nlst[0].n_value;
	}
	if (lseek(kmem, offset, L_SET) == -1) {
		close(kmem);
		return (1);
	}
	if (read(kmem, (char *) avenrun, sizeof(avenrun)) == -1) {
		close(kmem);
		return (1);
	}
	close(kmem);
#define loaddouble(la) ((double)(la) / (1 << 8))

	for (i = 0; i < 3; i++)
		cpu_load[i] = loaddouble(avenrun[i]);
#endif

	i = cpu_load[0];
	if (i < limit)
		i = 0;
	sprintf(loadstr, "%.2f %.2f %.2f%s",
		cpu_load[0], cpu_load[1], cpu_load[2],
		(i ? "¡A½Ðµy«á¦A¨Ó¡C" : ""));
	if (cpu_load[0] >= (float) 0 && cpu_load[0] < (float) 1) {
		strcpy(status, "­t²ü¥¿±`");
	} else if (cpu_load[0] >= 1 && cpu_load[0] < (float) 10) {
		strcpy(status, "­t²ü°¾°ª");
	} else {
		strcpy(status, "­t²ü¹L­«");
	}

	return i;
}
#endif


/* ----------------------------------------------------- */
/* stand-alone daemon                                    */
/* ----------------------------------------------------- */


static int mainset;		/* read file descriptor set */
static struct sockaddr_in xsin;

static void
reapchild()
{
	int     state, pid;
	while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0);
}

static void
start_daemon()
{
	int     n;
	char    buf[80];
	/*
	 * More idiot speed-hacking --- the first time conversion makes the C
	 * library open the files containing the locale definition and time
	 * zone. If this hasn't happened in the parent process, it happens in
	 * the children, once per connection --- and it does add up.
	 */

	time_t  dummy = time(NULL);
	struct tm *dummy_time = localtime(&dummy);
	(void) strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

	n = getdtablesize();
	if (fork())
		exit(0);
	if (fork())	/* one more time */
		exit(0);
		
	snprintf(genbuf, 1024, "%d\t%s", getpid(), buf);

	while (n)
		(void) close(--n);

/*
	n = open("/dev/tty", O_RDWR);
	if (n > 0) {
		(void) ioctl(n, TIOCNOTTY, (char *) 0);
		(void) close(n);
	}
*/
	for (n = 1; n < NSIG; n++)
		(void) signal(n, SIG_IGN);
}


static void
close_daemon()
{
	exit(0);
}

static void
bbsd_log(char *str)
{
	char buf[256];
	time_t mytime;
	struct tm *tm;
	
	mytime = time(0);
	tm = localtime(&mytime);
	sprintf(buf, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d bbsd[%d]: %s", tm->tm_year % 100 , tm->tm_mon + 1, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec, getpid(), str);
	fprintf(stderr,"%s", buf);
	file_append(LOG_FILE, buf);
}

static int
bind_port(port)
int     port;
{
	int     sock, on;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	on = 1;
	(void) setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

	xsin.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) & xsin, sizeof xsin) < 0) {
		snprintf(genbuf, 1024, "bbsd bind_port can't bind to %d\n", port);
		bbsd_log(genbuf);
		exit(1);
	}
	if (listen(sock, SOCKET_QLEN) < 0) {
		snprintf(genbuf, 1024,"bbsd bind_port can't listen to %d\n", port);
		bbsd_log(genbuf);
		exit(1);
	}
	FD_SET(sock, (fd_set *) & mainset);

	snprintf(genbuf, 1024,"started on port %d\n", port);
	bbsd_log(genbuf);
	
	return sock;
}

static int
bad_host(name, user)
char *name;
char *user;
{
	FILE   *list;
	char    buf[80], *buf2;

	if (list = fopen(BAD_HOST, "r")) {
		while (fgets(buf, 80, list)) {
			buf[strlen(buf) - 1] = '\0';
			if (strchr(buf,'@')) {
				buf2 = strtok(buf,"@");
				if (!strcasecmp(buf2, user)) {
					buf2 = strtok(NULL,"@");
					if (!strcasecmp(buf2, name))
						return 1;
				}
			} else {
				if (!strcasecmp(buf, name))
					return 1;
				if (buf[strlen(buf) - 1] == '.' && !strncasecmp(buf, name, strlen(buf)))
					return 1;
				if (*buf == '.' && strlen(buf) < strlen(name) && !strcasecmp(buf, name + strlen(name) - strlen(buf)))
					return 1;
			}
		}
		fclose(list);
	}

	return 0;
	
}

int
main(argc, argv)
int     argc;
char   *argv[];
{
	extern int errno;

	register int msock, csock;	/* socket for Master and Child */
	register int nfds;	/* number of sockets */
	register int overload;
	register pid_t pid;
	register time_t uptime;
	int     readset;
	int     value;
	struct timeval tv;

	if ( argc == 2 ) {
		if ( strchr(argv[1], '-') && strchr(argv[1], 'v') ) {
#ifdef VERSION_ID
			printf("This is bbsd. %s\n%s\n", VERSION_ID, bbsd_c);
#else
			printf("This is bbsd. FB-3.1 series\n%s\n", bbsd_c);
#endif
			printf("build for %s under %s\n", _OSTYPE, _OSTRGT);
			printf("on %s\n", COMPILEDATE);
			exit(0);
		}
	}
	
	/* --------------------------------------------------- */
	/* setup standalone                                    */
	/* --------------------------------------------------- */

	start_daemon();

	(void) signal(SIGCHLD, reapchild);
	(void) signal(SIGTERM, close_daemon);


	/* --------------------------------------------------- */
	/* port binding                                        */
	/* --------------------------------------------------- */

	xsin.sin_family = AF_INET;

	if (argc > 1) {
		msock = -1;
		for (nfds = 1; nfds < argc; nfds++) {
			csock = atoi(argv[nfds]);
			if (csock > 0)
				msock = bind_port(csock);
			else
				break;
		}
		if (msock < 0)
			exit(1);
	} else {
		static int ports[] = {3006};
		for (nfds = 0; nfds < sizeof(ports) / sizeof(int); nfds++) {
			csock = ports[nfds];
			msock = bind_port(csock);
		}
	}
	nfds = msock + 1;

	/* --------------------------------------------------- */
	/* Give up root privileges: no way back from here      */
	/* --------------------------------------------------- */

	(void) setgid((gid_t)BBSGID);
	(void) setuid((uid_t)BBSUID);
	(void) chdir(BBSHOME);
	umask((mode_t)022);

	unlink(PID_FILE);
	snprintf(genbuf, 1024, "%d", getpid());
	file_append(PID_FILE, genbuf);
	
	/* --------------------------------------------------- */
	/* main loop                                           */
	/* --------------------------------------------------- */

#if 0
	resolve_utmp();
#endif

	tv.tv_sec = 60 * 30;
	tv.tv_usec = 0;

	overload = uptime = 0;

	for (;;) {

#ifdef  HAVE_CHKLOAD
		pid = time(0);
		if (pid > uptime) {
			overload = chkload(overload ? TH_LOW : TH_HIGH);
			uptime = pid + overload + 45;	/* µu®É¶¡¤º¤£¦AÀË¬d system load */
		}
#endif

again:

		readset = mainset;

		value = sizeof xsin;
		do {
			csock = accept(msock, (struct sockaddr *) & xsin, &value);
		} while (csock < 0 && errno == EINTR);

		if (csock < 0) {
			goto again;
		}
#ifdef NOLOGIN
		{
			FILE   *fp;
			char    buf[256];
#define MYBANNER "\r\nFirebird BBS 3.0 [bbsd NOLOGIN]\r\n"

			if ((fp = fopen(NOLOGIN, "r")) != NULL) {
				(void) write(csock, MYBANNER, strlen(MYBANNER));
				while (fgets(buf, 255, fp) != 0) {
					strcat(buf, "\r");
					(void) write(csock, buf, strlen(buf));
				}
				fclose(fp);
				sleep(3);
				close(csock);
				continue;
			}
		}
#endif

#ifdef  HAVE_CHKLOAD
		if (overload) {
			(void) write(csock, loadstr, strlen(loadstr));
			(void) close(csock);
			continue;
		}
#endif

		pid = fork();

		if (!pid) {

#ifdef  HAVE_CHKLOAD
			(void) close(fkmem);
#endif

			while (--nfds >= 0)
				(void) close(nfds);
			(void) dup2(csock, 0);
			(void) close(csock);

			getremotename(&xsin, fromhost, remoteusername);	/* FC931 */

			/* ban ±¼ bad host / bad user */
			if ( bad_host(fromhost,remoteusername) )
				exit(1);

			bbssetenv("REMOTEHOST", fromhost, 1);
			bbssetenv("REMOTEUSER", remoteusername, 1);

			telnet_init();
			nice(3);	/* lower priority .. */
			start_client();
		}
		(void) close(csock);
	}
}
