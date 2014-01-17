/*
 * gopherd.c	-- a daemon export bbs 0Announce via gopher protocol
 *	
 * A part of Firebird BBS 3.0
 *
 * Copyright (c) 1999, Firebird BBS 3.0 Team <edwardc@firebird.dhs.org>
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
 * CVS: $Id: gopherd.c,v 1.1 2000/01/15 01:45:33 edwardc Exp $
 */

#include "bbs.h"
#include "config.h"

#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define GOPHER_PORT     70
#define GOPHER_HOME     (BBSHOME "/0Announce")
#define MYGOPHERHOST	BBSHOST

#define TCP_BUFSIZ      4096
#define TCP_QLEN        5

#define GOPHER_LOGFILE  (BBSHOME"/reclog/bgopherd.log")
#define GOPHER_PIDFILE  (BBSHOME"/reclog/bgopherd.pid")

/* ----------------------------------------------------- */
/* operation log and debug information                   */
/* ----------------------------------------------------- */

int flog;                       /* log file descriptor */

void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;
  char buf[256];

  time(&now);
  p = localtime(&now);
  sprintf(buf, "%02d/%02d/%02d %02d:%02d:%02d [%d] %-7s%s\n",
    p->tm_year, p->tm_mon + 1, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec, getpid(), key, msg);
  write(flog, buf, strlen(buf));
}

void
log_init()
{
  flog = open(GOPHER_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
  logit("START", "gopher daemon");
}

void
log_close()
{
  close(flog);
}

#if 0

/* ----------------------------------------------------- */
/* 取得 remote user name 以判定身份                      */
/* ----------------------------------------------------- */

/*
 * rfc931() speaks a common subset of the RFC 931, AUTH, TAP, IDENT and RFC
 * 1413 protocols. It queries an RFC 931 etc. compatible daemon on a remote
 * host to look up the owner of a connection. The information should not be
 * used for authentication purposes. This routine intercepts alarm signals.
 *
 * Diagnostics are reported through syslog(3).
 *
 * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
 */

#include <setjmp.h>

#define STRN_CPY(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }
#define STRING_LENGTH    60
#define RFC931_TIMEOUT   10
#define RFC931_PORT     113     /* Semi-well-known port */
#define ANY_PORT        0       /* Any old port will do */

/* ------------------------- */
/* timeout - handle timeouts */
/* ------------------------- */

static jmp_buf timebuf;

static void
timeout(sig)
  int sig;
{
  longjmp(timebuf, sig);
}

void
getremotename(from, rhost, rname)
  struct sockaddr_in *from;
  char *rhost;
  char *rname;
{
/* ------------------------- */
/* timeout - handle timeouts */
/* ------------------------- */

static jmp_buf timebuf;

static void
timeout(sig)
  int sig;
{
  longjmp(timebuf, sig);
}

void
getremotename(from, rhost, rname)
  struct sockaddr_in *from;
  char *rhost;
  char *rname;
{
/* ------------------------- */
/* timeout - handle timeouts */
/* ------------------------- */

static jmp_buf timebuf;

static void
timeout(sig)
  int sig;
{
  longjmp(timebuf, sig);
}

void
getremotename(from, rhost, rname)
  struct sockaddr_in *from;
  char *rhost;
  char *rname;
{
  struct sockaddr_in our_sin;
  struct sockaddr_in rmt_sin;
  unsigned rmt_port, rmt_pt;
  unsigned our_port, our_pt;
  FILE *fp;
  char buffer[512], user[80], *cp;
  int s;
  struct hostent *hp;

  /* get remote host name */

  hp = NULL;
  if (setjmp(timebuf) == 0)
  {
    signal(SIGALRM, timeout);
    alarm(3);
    hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr),
      from->sin_family);
    alarm(0);
  }
  strcpy(rhost, hp ? hp->h_name : (char *) inet_ntoa(from->sin_addr));

  /*
   * Use one unbuffered stdio stream for writing to and for reading from the
   * RFC931 etc. server. This is done because of a bug in the SunOS 4.1.x
   * stdio library. The bug may live in other stdio implementations, too.
   * When we use a single, buffered, bidirectional stdio stream ("r+" or "w+"
   * mode) we read our own output. Such behaviour would make sense with
   * resources that support random-access operations, but not with sockets.
   */

  *rname = '\0';

  s = sizeof our_sin;
  if (getsockname(0, &our_sin, &s) < 0)
    return;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    logit("ERROR", "socket in rfc931");
    return;
  }

  if (!(fp = fdopen(s, "r+")))
  {
    close(s);
    return;
  }

  /*
   * Set up a timer so we won't get stuck while waiting for the server.
   */

  if (setjmp(timebuf) == 0)
  {
    signal(SIGALRM, timeout);
    alarm(RFC931_TIMEOUT);

    /*
     * Bind the local and remote ends of the query socket to the same IP
     * addresses as the connection under investigation. We go through all
     * this trouble because the local or remote system might have more than
     * one network address. The RFC931 etc. client sends only port numbers;
     * the server takes the IP addresses from the query socket.
     */

    our_pt = ntohs(our_sin.sin_port);
    our_sin.sin_port = htons(ANY_PORT);

    rmt_sin = *from;
    rmt_pt = ntohs(rmt_sin.sin_port);
    rmt_sin.sin_port = htons(RFC931_PORT);

    setbuf(fp, (char *) 0);
    s = fileno(fp);

    if (bind(s, (struct sockaddr *) & our_sin, sizeof(our_sin)) >= 0 &&
      connect(s, (struct sockaddr *) & rmt_sin, sizeof(rmt_sin)) >= 0)
    {

      /*
       * Send query to server. Neglect the risk that a 13-byte write would
       * have to be fragmented by the local system and cause trouble with
       * buggy System V stdio libraries.
       */

      fprintf(fp, "%u,%u\r\n", rmt_pt, our_pt);
      fflush(fp);
      /*
       * Read response from server. Use fgets()/sscanf() so we can work
       * around System V stdio libraries that incorrectly assume EOF when a
       * read from a socket returns less than requested.
       */

      if (fgets(buffer, sizeof(buffer), fp) && !ferror(fp) && !feof(fp)
        && sscanf(buffer, "%u , %u : USERID :%*[^:]:%79s",
          &rmt_port, &our_port, user) == 3
        && rmt_pt == rmt_port && our_pt == our_port)
      {

        /*
         * Strip trailing carriage return. It is part of the protocol, not
         * part of the data.
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

#endif

/* ----------------------------------------------------- */
/* buffered TCP I/O routines                             */
/* ----------------------------------------------------- */

int tcp_pos;
char tcp_pool[TCP_BUFSIZ + 512];

void
tcp_init()
{
  tcp_pos = 0;
}

int
tcp_puts(sock, msg)
  int sock;
  char *msg;
{
  int pos, len;
  char *head, *tail;

  len = strlen(msg);
  pos = tcp_pos;

  head = tcp_pool;
  tail = head + pos;
  memcpy(tail, msg, len);
  memcpy(tail + len, "\r\n", 2);
  pos += (len + 2);

  while (pos >= TCP_BUFSIZ)
  {
    len = write(sock, head, TCP_BUFSIZ);
    if (len <= 0)
      return 0;
    pos -= len;
    if (pos == 0)
      break;
    memcpy(head, head + len, pos);
  }

  tcp_pos = pos;
  return len;
}

int
tcp_flush(sock, msg)
  int sock;
  char *msg;
{
  int pos, len;

  if (msg != NULL)
  {
    len = tcp_puts(sock, msg);
    if (len == 0)
      return 0;
  }

  pos = tcp_pos;
  if (pos == 0)
    return 0;

  msg = tcp_pool;
  for (;;)
  {
    len = write(sock, msg, pos);
    if (len <= 0)
      return 0;
    pos -= len;
    if (pos == 0)
    {
      tcp_pos = 0;
      return len;
    }
    msg += len;
  }
}

/* ----------------------------------------------------- */
/* transform to real path & security check               */
/* ----------------------------------------------------- */

int
real_path(path)
  char *path;
{
  int ch, level;
  char *source, *target;

  if (*path == '/')
    return 0;

  level = 1;
  source = target = path;
  for (;;)
  {
    ch = *source;

    if (ch == '/')
    {
      int next;

      next = source[1];

      if (next == '/')
      {
        return 0;               /* [//] */
      }
      else if (next == '.')
      {
        next = source[2];

        if (next == '/')
          return 0;             /* [/./] */

        if (next == '.' && source[3] == '/')
        {
          /* -------------------------- */
          /* abc/xyz/../def ==> abc/def */
          /* -------------------------- */

          for (;;)
          {
            if (target <= path)
              return 0;

            target--;
            if (*target == '/')
              break;
          }

          source += 3;
          continue;
        }
      }

      level++;
    }

    *target = ch;

    if (ch == 0)
      return level;

    target++;
    source++;
  }
}

/* ----------------------------------------------------- */
/* send file out in text-mail format                     */
/* ----------------------------------------------------- */
int
dashf( fname )
char *fname;
{
    struct stat st;

    return ( stat( fname, &st ) == 0 && S_ISREG( st.st_mode ) );
}

void
send_file(sock, fpath)
  int sock;
  char *fpath;
{
  FILE *fp;
  struct stat st;

  if (!stat(fpath, &st) && S_ISREG(st.st_mode) && (fp = fopen(fpath, "r")))
  {
    int ch;
    char *ip, *p, buf[512];

    ip = buf;
    while (fgets(ip, 511, fp))
    {
      for (p = ip; ch = *p; p++)
      {
        if (ch == '\n' || ch == '\r')
        {
          *p = '\0';
          break;
        }
      }
      if (*ip == '.' && ip[1] == '\0')
        tcp_puts(sock, "..");
      else
        tcp_puts(sock, ip);
    }
    fclose(fp);
  }
}

/* ----------------------------------------------------- */
/* transform .Names to gopher index format & send it out */
/* ----------------------------------------------------- */

void
send_index(sock, fpath, plus)
  int sock;
  char *fpath;
  char *plus;
{
  int mode;
  FILE *fp;
  char gtitle[80];
  char gserver[]=MYGOPHERHOST, gport[]="70";
  char *ptr, buf[1024];
  char outbuf[2048];
  
#ifdef  DEBUG
  logit("index", fpath);
#endif

  sprintf(outbuf, "%s.Names", fpath);

  if (fp = fopen(outbuf, "r"))
  {
    mode = -1;
    while (fgets(buf, 511, fp))
    {
      if( buf[0] == '#' || !memcmp(buf, "Numb=",5))
        continue;

      if (ptr = strchr(buf, '\n'))
        *ptr = '\0';

      if (!memcmp(buf, "Name=", 5))
      {
        ptr = buf + 5;
        strcpy( gtitle, ptr);
        continue;
      }

      if(!memcmp(buf,"Path=~/",7))
      {
        ptr = buf + 7;
        sprintf( outbuf, "%s%s", fpath, ptr );
        mode = ( dashf( outbuf ) ? 0 : 1 );
        sprintf(outbuf, "%c/%s%s", mode + '0', fpath, ptr);
        if (!real_path(outbuf))
       {
          mode = -1;
          continue;
        }
        ptr = buf;
        strcpy(ptr, outbuf);

        sprintf(outbuf, "%s%c%-39s\t%s\t%s\t%s",
           plus, mode + '0',gtitle, ptr, gserver, gport);
        if( !strstr( gtitle, "(BM: SYSOPS)" ) && 
            !strstr( gtitle, "(BM: BMS)" ) &&
            !strstr( gtitle, "(BM: SECRET)" ) )
           tcp_puts(sock, outbuf);
        mode = -1;
      }
    }
    fclose(fp);
  }
}

/* ----------------------------------------------------- */
/* parse client's command and serve it                   */
/* ----------------------------------------------------- */
void
serve(sock)
  int sock;
{
  register char *ptr;
  int n, ch;
  char buf[1024];

  ptr = buf;
  n = read(sock, ptr, 1024);
  if (n < 0)
    return;

  ptr[n] = '\0';

  while (ch = *ptr)
  {
    if (ch == '\r' || ch == '\n')
    {
      *ptr = '\0';
      n = ptr - buf;
      break;
    }
    ptr++;
  }

  ptr = buf;
  ch = *ptr;
  if (ch == 0)
    ch = '1';

  logit("cmd", ptr);
  tcp_init();

  switch (ch)
  {
  case '0':
    ptr += 2;
    if (real_path(ptr))
      send_file(sock, ptr);
    break;

  case '1':
    if (n <= 2)
      *ptr = '\0';
    else
    {
      strcpy(ptr + n, "/");
      ptr += 2;
    }
    send_index(sock, ptr, "");
    break;

  case '\t':
    tcp_puts(sock, "+-1");
    send_index(sock, "", "+INFO: ");
    break;

  default:
    return;
  }
  tcp_flush(sock, ".");
}
/* ----------------------------------------------------- */
/* daemon : initialize and other stuff                   */
/* ----------------------------------------------------- */

int
start_daemon()
{
  int n, on;
  char buf[80];
  struct sockaddr_in fsin;

  /*
   * More idiot speed-hacking --- the first time conversion makes the C
   * library open the files containing the locale definition and time zone.
   * If this hasn't happened in the parent process, it happens in the
   * children, once per connection --- and it does add up.
   */

  time_t dummy = time(NULL);
  struct tm *dummy_time = localtime(&dummy);
  struct tm *other_dummy_time = gmtime(&dummy);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

  n = getdtablesize();

  if (fork())
    exit(0);

  while (n)
    (void) close(--n);

  n = open(GOPHER_PIDFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (n >= 0)
  {
    sprintf(buf, "%5d\n", getpid());
    write(n, buf, 6);
    close(n);
  }

  (void) open("/dev/null", O_RDONLY);
  (void) dup2(0, 1);
  (void) dup2(0, 2);
  if ((n = open("/dev/tty", O_RDWR)) > 0)
  {
    ioctl(n, TIOCNOTTY, 0);
    close(n);
  }

  n = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(n, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
  memset((char *) &fsin, 0, sizeof(fsin));
  fsin.sin_family = AF_INET;
  fsin.sin_addr.s_addr = htonl(INADDR_ANY);
  fsin.sin_port = htons(GOPHER_PORT);

  if (bind(n, (struct sockaddr *) & fsin, sizeof(fsin)) < 0)
    exit(1);

  return n;
}

#if 0
void
reaper()
{
  int state, pid;
  while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0);
}

#endif

void
abort_server()
{
  log_close();
  exit(1);
}

int
main()
{
  int msock, csock, nfds, sr;
  extern int errno;
  fd_set rset;

  msock = start_daemon();

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(GOPHER_HOME);
  log_init();

  (void) signal(SIGHUP, abort_server);

#if 0
  (void) signal(SIGCHLD, reaper);
#endif

  listen(msock, TCP_QLEN);

  /* initialize resource */

  nfds = msock + 1;
  for (;;)
  {
    /* Set up the fdsets. */

    FD_ZERO(&rset);
    FD_SET(msock, &rset);

    sr = select(nfds, &rset, NULL, NULL, NULL);

    if (sr < 0)
    {
      if (errno == EINTR)
        continue;
      logit("select", sys_errlist[errno]);
      exit(-1);
    }

    if (sr == 0)                /* * No network traffic */
    {
      continue;
    }

    if (FD_ISSET(msock, &rset))
    {
      csock = accept(msock, NULL, NULL);
      if (csock < 0)
      {

#if 0
        if (errno != EINTR)
          continue;
        reaper();
#endif

        continue;
      }
      serve(csock);
      shutdown(csock, 2);
      close(csock);
    }
  }
}
