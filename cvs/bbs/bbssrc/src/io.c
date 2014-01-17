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
$Id: io.c,v 1.2 2002/01/06 10:06:25 chinsan Exp $
*/

#include "bbs.h"
#ifdef AIX
#include <sys/select.h>
#endif
#include <arpa/telnet.h>

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

#define INPUT_ACTIVE 0
#define INPUT_IDLE 1

extern int dumb_term;
extern int ISLOGIN;

static char outbuf[OBUFSIZE];
static int obufsize = 0;
struct user_info uinfo;

char    inbuf[IBUFSIZE];
int     ibufsize = 0;
int     icurrchar = 0;
int     KEY_ESC_arg;

static int i_mode = INPUT_ACTIVE;
extern struct screenline *big_picture;

void
hit_alarm_clock()
{
	if (HAS_PERM(PERM_NOTIMEOUT))
		return;
	if (i_mode == INPUT_IDLE) {
		clear();
		fprintf(stderr, "Idle timeout exceeded! Booting...\n");
		kill(getpid(), SIGHUP);
	}
	i_mode = INPUT_IDLE;
	if (uinfo.mode == LOGIN)
		alarm(LOGIN_TIMEOUT);
	else
		alarm(IDLE_TIMEOUT);
}

void
init_alarm()
{
	signal(SIGALRM, hit_alarm_clock);
	alarm(IDLE_TIMEOUT);
}
#ifdef BBSD
void
oflush()
{
	register int size;
	if (size = obufsize) {
		write(0, outbuf, size);
		obufsize = 0;
	}
}

void
output(s, len)
char   *s;
int     len;
{
	/* Invalid if len >= OBUFSIZE */

	register int size;
	register char *data;
	size = obufsize;
	data = outbuf;
	if (size + len > OBUFSIZE) {
		write(0, data, size);
		size = len;
	} else {
		data += size;
		size += len;
	}
	memcpy(data, s, len);
	obufsize = size;
}

void
ochar(c)
register int c;
{
	register char *data;
	register int size;
	data = outbuf;
	size = obufsize;

	if (size > OBUFSIZE - 2) {	/* doin a oflush */
		write(0, data, size);
		size = 0;
	}
	data[size++] = c;
	if (c == IAC)
		data[size++] = c;

	obufsize = size;
}
#else
void
oflush()
{
	if (obufsize)
		write(1, outbuf, obufsize);
	obufsize = 0;
}

void
output(s, len)
char   *s;
int     len;
{
	/* Invalid if len >= OBUFSIZE */

	if (obufsize + len > OBUFSIZE) {	/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	memcpy(outbuf + obufsize, s, len);
	obufsize += len;
}

void
ochar(c)
int     c;
{
	if (obufsize > OBUFSIZE - 1) {	/* doin a oflush */
		write(1, outbuf, obufsize);
		obufsize = 0;
	}
	outbuf[obufsize++] = c;
}
#endif

int     i_newfd = 0;
struct timeval i_to, *i_top = NULL;
int     (*flushf) () = NULL;

void
add_io(fd, timeout)
int     fd;
int     timeout;
{
	i_newfd = fd;
	if (timeout) {
		i_to.tv_sec = timeout;
		i_to.tv_usec = 0;
		i_top = &i_to;
	} else
		i_top = NULL;
}

void
add_flush(flushfunc)
int     (*flushfunc) ();
{
	flushf = flushfunc;
}

int
num_in_buf()
{
	return icurrchar - ibufsize;
}

static int
iac_count(current)
char   *current;
{
	switch (*(current + 1) & 0xff) {
	case DO:
	case DONT:
	case WILL:
	case WONT:
		return 3;
	case SB:		/* loop forever looking for the SE */
		{
			register char *look = current + 2;
			for (;;) {
				if ((*look++ & 0xff) == IAC) {
					if ((*look++ & 0xff) == SE) {
						return look - current;
					}
				}
			}
		}
	default:
		return 1;
	}
}
#ifdef BBSD
int
igetch()
{
	static int trailing = 0;
	register int ch;
	register char *data;
	data = inbuf;

	for (;;) {
		if (ibufsize == icurrchar) {
			fd_set  readfds;
			struct timeval to;
			register fd_set *rx;
			register int fd, nfds;
			rx = &readfds;
			fd = i_newfd;

	igetnext:

			uinfo.idle_time = time(0);
			update_utmp();	/* 應該是需要 update 一下 :X */

			FD_ZERO(rx);
			FD_SET(0, rx);
			if (fd) {
				FD_SET(fd, rx);
				nfds = fd + 1;
			} else
				nfds = 1;
			to.tv_sec = to.tv_usec = 0;
			if ((ch = select(nfds, rx, NULL, NULL, &to)) <= 0) {
				if (flushf)
					(*flushf) ();

				if (big_picture)
					refresh();
				else
					oflush();

				FD_ZERO(rx);
				FD_SET(0, rx);
				if (fd)
					FD_SET(fd, rx);

				while ((ch = select(nfds, rx, NULL, NULL, i_top)) < 0) {
					if (errno != EINTR)
						return -1;
				}
				if (ch == 0)
					return I_TIMEOUT;
			}
			if (fd && FD_ISSET(fd, rx))
				return I_OTHERDATA;

			for (;;) {
				ch = read(0, data, IBUFSIZE);
				if (ch > 0)
					break;
				if ((ch < 0) && (errno == EINTR))
					continue;
				longjmp(byebye, -1);
			}
			icurrchar = (*data & 0xff) == IAC ? iac_count(data) : 0;
			if (icurrchar >= ch)
				goto igetnext;
			ibufsize = ch;
			i_mode = INPUT_ACTIVE;
		}
		ch = data[icurrchar++];
		if (trailing) {
			trailing = 0;
			if (ch == 0 || ch == 0x0a)
				continue;
		}
		if (ch == Ctrl('L')) {
			redoscr();
			continue;
		}
		if (ch == 0x0d) {
			trailing = 1;
			ch = '\n';
		}
		return (ch);
	}
}
#else
int
igetch()
{
	igetagain:
	if (ibufsize == icurrchar) {
		fd_set  readfds;
		struct timeval to;
		int     sr;
		to.tv_sec = 0;
		to.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		if (i_newfd)
			FD_SET(i_newfd, &readfds);
		if ((sr = select(FD_SETSIZE, &readfds, NULL, NULL, &to)) <= 0) {
			if (flushf)
				(*flushf) ();
			if (dumb_term)
				oflush();
			else
				refresh();
			FD_ZERO(&readfds);
			FD_SET(0, &readfds);
			if (i_newfd)
				FD_SET(i_newfd, &readfds);
			while ((sr = select(FD_SETSIZE, &readfds, NULL, NULL, i_top)) < 0) {
				if (errno == EINTR)
					continue;
				else {
					perror("select");
					fprintf(stderr, "abnormal select conditions\n");
					return -1;
				}
			}
			if (sr == 0)
				return I_TIMEOUT;
		}
		if (i_newfd && FD_ISSET(i_newfd, &readfds))
			return I_OTHERDATA;
		while ((ibufsize = read(0, inbuf, IBUFSIZE)) <= 0) {
			if (ibufsize == 0)
				longjmp(byebye, -1);
			if (ibufsize < 0 && errno != EINTR)
				longjmp(byebye, -1);
		}
		icurrchar = 0;
	}
	i_mode = INPUT_ACTIVE;
	switch (inbuf[icurrchar]) {
	case Ctrl('L'):
		redoscr();
		icurrchar++;
		goto igetagain;
	default:
		break;
	}
	return inbuf[icurrchar++];
}
#endif

int
igetkey()
{
	int     mode;
	int     ch, last;
	extern int RMSG;
	mode = last = 0;
	while (1) {
		if ((uinfo.in_chat == YEA || uinfo.mode == TALK || uinfo.mode == PAGE) && RMSG == YEA) {
			char    a;
			read(0, &a, 1);
			ch = (int) a;
		} else
			ch = igetch();
		if ((ch == Ctrl('Z')) && (RMSG == NA)) {
			r_msg2();
			return 0;
		}
		if (mode == 0) {
			if (ch == KEY_ESC)
				mode = 1;
			else
				return ch;	/* Normal Key */
		} else if (mode == 1) {	/* Escape sequence */
			if (ch == '[' || ch == 'O')
				mode = 2;
			else if (ch == '1' || ch == '4')
				mode = 3;
			else {
				KEY_ESC_arg = ch;
				return KEY_ESC;
			}
		} else if (mode == 2) {	/* Cursor key */
			if (ch >= 'A' && ch <= 'D')
				return KEY_UP + (ch - 'A');
			else if (ch >= '1' && ch <= '6')
				mode = 3;
			else
				return ch;
		} else if (mode == 3) {	/* Ins Del Home End PgUp PgDn */
			if (ch == '~')
				return KEY_HOME + (last - '1');
			else
				return ch;
		}
		last = ch;
	}
}

void
top_show(prompt)
char   *prompt;
{
	if (editansi) {
		prints(ANSI_RESET);
		refresh();
	}
	move(0, 0);
	clrtoeol();
	standout();
	prints("%s", prompt);
	standend();
}

int
ask(prompt)
char   *prompt;
{
	int     ch;
	top_show(prompt);
	ch = igetkey();
	move(0, 0);
	clrtoeol();
	return (ch);
}

int
getdata(line, col, prompt, buf, len, echo, clearlabel)
int     line, col, len, echo, clearlabel;
char   *prompt, *buf;
{
	int     ch, clen = 0, curr = 0, x, y;
	char    tmp[STRLEN];
	extern unsigned char scr_cols;
	extern int RMSG;
	extern int msg_num;
	if (clearlabel == YEA) {
		memset(buf, 0, sizeof(buf));
	}
	move(line, col);
	if (prompt)
		prints("%s", prompt);
	y = line;
	col += (prompt == NULL) ? 0 : strlen(prompt);
	x = col;
	clen = strlen(buf);
	curr = (clen >= len) ? len - 1 : clen;
	buf[curr] = '\0';
	prints("%s", buf);

	if (dumb_term || echo == NA) {
		while ((ch = igetkey()) != '\r') {
			if (RMSG == YEA && msg_num == 0) {
				if (ch == Ctrl('Z') || ch == KEY_UP) {
					buf[0] = Ctrl('Z');
					clen = 1;
					break;
				}
				if (ch == Ctrl('A') || ch == KEY_DOWN) {
					buf[0] = Ctrl('A');
					clen = 1;
					break;
				}
			}
			if (ch == '\n')
				break;
			if (ch == '\177' || ch == Ctrl('H')) {
				if (clen == 0) {
					continue;
				}
				clen--;
				ochar(Ctrl('H'));
				ochar(' ');
				ochar(Ctrl('H'));
				continue;
			}
			if (!isprint2(ch)) {
				continue;
			}
			if (clen >= len - 1) {
				continue;
			}
			buf[clen++] = ch;
			if (echo)
				ochar(ch);
			else
				ochar('*');
		}
		buf[clen] = '\0';
		outc('\n');
		oflush();
		return clen;
	}
	clrtoeol();
	while (1) {
		if ((uinfo.in_chat == YEA || uinfo.mode == TALK) && RMSG == YEA) {
			refresh();
		}
		ch = igetkey();
		if ((RMSG == YEA) && msg_num == 0) {
			if (ch == Ctrl('Z') || ch == KEY_UP) {
				buf[0] = Ctrl('Z');
				clen = 1;
				break;
			}
			if (ch == Ctrl('A') || ch == KEY_DOWN) {
				buf[0] = Ctrl('A');
				clen = 1;
				break;
			}
		}
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == '\177' || ch == Ctrl('H')) {
			if (curr == 0) {
				continue;
			}
			strcpy(tmp, &buf[curr]);
			buf[--curr] = '\0';
			(void) strcat(buf, tmp);
			clen--;
			move(y, x);
			prints("%s", buf);
			clrtoeol();
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_DEL) {
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			strcpy(tmp, &buf[curr + 1]);
			buf[curr] = '\0';
			(void) strcat(buf, tmp);
			clen--;
			move(y, x);
			prints("%s", buf);
			clrtoeol();
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_LEFT) {
			if (curr == 0) {
				continue;
			}
			curr--;
			move(y, x + curr);
			continue;
		}
		if (ch == Ctrl('E') || ch == KEY_END) {
			curr = clen;
			move(y, x + curr);
			continue;
		}
		if (ch == Ctrl('A') || ch == KEY_HOME) {
			curr = 0;
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_RIGHT) {
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			curr++;
			move(y, x + curr);
			continue;
		}
		if (!isprint2(ch)) {
			continue;
		}
		if (x + clen >= scr_cols || clen >= len - 1) {
			continue;
		}
		if (!buf[curr]) {
			buf[curr + 1] = '\0';
			buf[curr] = ch;
		} else {
			strncpy(tmp, &buf[curr], len);
			buf[curr] = ch;
			buf[curr + 1] = '\0';
			strncat(buf, tmp, len - curr);
		}
		curr++;
		clen++;
		move(y, x);
		prints("%s", buf);
		move(y, x + curr);
	}
	buf[clen] = '\0';
	if (echo) {
		move(y, x);
		prints("%s", buf);
	}
	outc('\n');
	refresh();
	return clen;
}
