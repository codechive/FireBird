/*
 * socket.c		-- socket implementation for http GET/POST method usage
 *
 * A part of SEEDNetBBS generation 1
 *
 * Copyright (c) 1998, 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
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
 * CVS: $Id: socket.c,v 1.1 2000/09/22 15:14:03 edwardc Exp $
 */

#include "bbs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

char    socket_c[] =
"$Id: socket.c,v 1.1 2000/09/22 15:14:03 edwardc Exp $";
#define MAXLINE	512
#define BBS			// standalone use please comment out this.
#define POST		// using POST method (pretty good)
#undef  GET			// using GET method	(not 100% compatiable !!)
#define parameter	"User-Agent: BBS Caller [$Revision: 1.1 $]\nContent-type: application/x-www-form-urlencoded\n"

#define _BBSPAGER	0x1
#define _TVSCHEDULE 0x2

int
readn(register int fd, register char *ptr, register int nbytes)
{
	int     nleft, nread;
	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread < 0)
			return (nread);
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes - nleft);
}

int
writen(register int fd, register char *ptr, register int n)
{
	int     nleft, nwrite;
	nleft = n;
	while (nleft > 0) {
		nwrite = write(fd, ptr, nleft);
		if (nwrite <= 0)
			return (nwrite);

		nleft -= nwrite;
		ptr += nwrite;
	}
	return (n - nleft);
}

int
readl(register int fd, register char *ptr, register int maxlen)
{
	int     n, rc;
	char    c;
	for (n = 1; n < maxlen; n++) {
		if ((rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return 0;
			else
				break;
		} else
			return -1;
	}

	*ptr = 0;
	return n;
}

int
control(register int fd, char *command, char *path, char *ref, int job)
{
	int     n, x;
	char    send[MAXLINE], recv[MAXLINE + 1], fbuf[STRLEN];
	FILE   *fp;
	/*
	 * edwardc.990217 note: save result function can be re-fine for
	 * ignore the HTML tags, headers... for a pretty output ..
	 */

	sprintf(fbuf, "tmp/%s.pager.%d", currentuser.userid, time(0));
	fp = fopen(fbuf, "w");

	/* 990224.edwardc add POST method for compatiable for all services */

#ifdef POST
	sprintf(send,
		"POST %s HTTP/1.0\nReferer:%s\n%sContent-length:%d\n\n%s",
		path, ref, parameter, strlen(command), command);
#endif

#ifdef GET
#ifndef POST
	sprintf(send, "GET %s HTTP/1.0\r\n\n\n", command);
#else
#error "you cannot define the GET and POST at the same time!!"
#endif
#endif

	n = strlen(send);
	x = 0;
	if (writen(fd, send, n) != n)
		return (err(0, "cannot write to socket"));

	while ((n = readl(fd, recv, MAXLINE)) > 0) {
		if (strstr(recv, "200 OK") && x == 0)
			x = 1;

		if (job == _BBSPAGER)
			fputs((char *) no_tag(recv), fp);
		else if (job == _TVSCHEDULE) {
			/* v 0.1 */
			if (recv[0] == '<' && recv[1] == 'T' && recv[2] == 'R' && recv[3] == '>'
				&& recv[4] == '<' && recv[5] == 'T' && recv[6] == 'D' && recv[7] == '>'
				&& !isalpha(recv[8]) && recv[strlen(recv) - 2] == 'R' && recv[strlen(recv) - 1] == '>')
				fputs((char *) no_tag(recv), fp);
		} else {
			fputs(recv, fp);
		}
	}

	fclose(fp);

	if (job == _BBSPAGER)
		mail_file(fbuf, currentuser.userid, "網路傳呼結果");
	else if (job == _TVSCHEDULE)
		mail_file(fbuf, currentuser.userid, "電視節目表查詢");
	else
		mail_file(fbuf, currentuser.userid, "undefinied job.");

	unlink(fbuf);
	return x;

}

int
dopage(char *path, char *site, char *ref, int port, char *command)
{
	int     fd;
	struct sockaddr_in sock;
	struct hostent *hp;
	bzero((char *) &sock, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_port = htons(port);

	/* non-BSD socket implement */
	/* it's seems useful for SYSV and BSD families .. */

	hp = (struct hostent *) gethostbyname(site);
	if (hp != 0) {
		memcpy(&sock.sin_addr, hp->h_addr_list[0], sizeof sock.sin_addr);
	} else {
		hp = (struct hostent *) gethostbyaddr(site, strlen(site), AF_INET);
		if (hp == 0) {
			return (err(0, "cannot resolv hostname "));
			return -1;
		}
		memcpy(&sock.sin_addr, hp->h_addr_list[0], sizeof sock.sin_addr);
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (err(0, "doconnect: cannot open client socket"));

	if (connect(fd, (struct sockaddr *) & sock, sizeof(sock)) < 0)
		return (err(0, "cannot connect to the server"));

	if (control(fd, command, path, ref, _BBSPAGER)) {
		close(fd);
		return 1;
	} else {
		close(fd);
		return 0;
	}

}
#ifdef BBS

int
err(int x, char *str)
{
	prints("%s, please report to SYSOP\n", str);
	pressreturn();
	return -1;
}
#endif
