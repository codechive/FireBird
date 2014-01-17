/*
 * paging.c		-- bbs -> web paging using socket.c for socket function
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
 * CVS: $Id: paging.c,v 1.1 2000/09/22 15:14:03 edwardc Exp $
 */

#include "bbs.h"
#include <string.h>
#include "socket.c"

#define AT		"%40"				// `@' sign is translate to %40
#ifndef BBSID
#warning "Error, no BBSID defined. you may using older than 0509-SNAP, pls. update it."
#define BBSID "SEEDNetBBS"
#endif
#define _BBSID	AT""BBSID			// BBSID defined in include/config.h
// using it or change it you want

extern char BoardName[];

char    paging_c[] =
"$Id: paging.c,v 1.1 2000/09/22 15:14:03 edwardc Exp $";

typedef struct {
	char    id[5];
	      //094 x(must be 5 long to keep the \ 0 char..)
	char    ident[20];
	      //¤½¥q ¦ W º Ù
	char    site[35];
	      //domain name
	char    port[5];
	      //the port, httpd is 80
	char    path[50];
	      //save the cgi path
	char	ref[80];
		  //reference page
	int     type[3];
	      //[0] is caller type 0 is numeric, 1 is text, 2 is both
	//      [1] is max length of numeric message
	//      [2] is max length of text message
}       spsrc;

int     _calluser(), _usersetup();
char   *_uri_encode();
/* 990217.edwardc I'm so lazy .. this part can be written into file for
   easy update .. may next version .. :) */

static spsrc services[] =
{
	{"0940",
		"§»»·¹q°T¥ô§Ú¦æ",
		"www.jettone.com.tw",
		"80",
		"/scripts/pager/fpage.asp",
		"http://www.jettone.com.tw/scripts/pager/fpageChk.asp",
		{2, 40, 120},
	},

	{"0941",
		"¤¤µØ°ê»Ú¶Ç©I",
		"www.chips.com.tw",
		"9100",
		"/cgi-bin/paging1.pl",
		"http://www.chips.com.tw:9100/WEB2P/page_1.htm",
		{2, 16, 25},
	},

	{"0943",
		"ÁpµØ¹q«H AlphaCall",
		"www.alphacall.com.tw",
		"80",
		"/web/webtopager/tpnasp/dowebcall.asp",
		"http://www.pager.com.tw/tpn/webcall/webcall.asp",
		{2, 14, 30},
	},

	{"0945",
		"¨³¹F¹q°T Taiwan BBC",
		"www.tw0945.com",
		"80",
		"/Scripts/fiss/PageForm.exe",
		"http://www.tw0945.com/call_AlphaPg.HTM",
		{2, 60, 160},
	},

	{"0946",
		"ÁpµØ¹q«H AlphaCall",
		"www.alphacall.com.tw",
		"80",
		"/web/webtopager/tpnasp/dowebcall.asp",
		"",
		{2, 14, 30},
	},

	{"0947",
		"§»»·¹q°T¥ô§Ú¦æ",
		"web1.hoyard.com.tw",
		"80",
		"/scripts/fp_page1.dll",
		"http://web1.hoyard.com.tw/freeway/freewayi.html",
		{2, 20, 100},
	},

	{"0948",
		"¤j²³¹q«H",
		"www.fitel.net.tw",
		"80",
		"/cgi-bin/Webpage.dll",
		"http://www.fitel.net.tw/html/svc03.htm",
		{2, 17, 60},
	},

	{"0949",
		"«n¤è¹q°T¥ô§Ú¦æ",
		"www.southtel.com.tw",
		"80",
		"/Scripts/Webpg.exe",
		"http://www.southtel.com.tw/numpg.htm",
		{2, 20, 40},
	},

	{"0951",
		"¤¤µØ°ê»Ú¶Ç©I",
		"www.chips.com.tw",
		"9100",
		"/cgi-bin/paging1.pl",
		"http://www.chips.com.tw:9100/WEB2P/page_1.htm",
		{2, 16, 25},
	},

	{0,
		0,
		0,
		0,
		0,
		0,
		0
	}
};

int
b2p()
{
	int     i;
	modify_user_mode(BBSPAGER);
	while (1) {
		i = _b2p();
		if (i == 1998)
			version();
		else if (i == 0)
			continue;
		else
			break;
	}
}

int
_b2p()
{
	int     i = 0;
	char    ans[3];
	clear();
	showtitle("[ºô¸ô¶Ç©I]", BoardName);
	move(1, 0);
	prints("Åwªï¨Ï¥Î [1;44;32m %s [m BBS to Pager ¨t²Î ($Revision: 1.1 $)\n", BoardName);
	prints("¥Ø«e´£¨Ñ¥H¤UªA°È: \n");
	prints("\n[1;44;33m%4s %s %-25s %-26s%-17s[m\n", "½s¸¹", "ªù¸¹", "¤½¥q¦WºÙ", "ºô¯¸", "«¬§O");

	for (i = 0;; i++) {
		if (services[i].id == NULL || services[i].id[0] == '\0')
			break;
		prints("%4d %s %-25s %-26s", i + 1, services[i].id, services[i].ident, services[i].site);

		if (services[i].type[0] == 0)
			prints("¼Æ¦r«¬\n");
		else if (services[i].type[0] == 1)
			prints("¤å¦r«¬\n");
		else if (services[i].type[0] == 2)
			prints("¼Æ¦r»P¤å¦r\n");
		else
			prints("Unknow\n");

	}
	prints("\n%4d ¶Ç©I¯¸¤W¦³³]©wáà¾÷¸¹½Xªº¨Ï¥ÎªÌ\n", i + 1);
	prints("%4d ³]©w¦Û¤vªºáà¾÷ (­­¥H¤W¦³¦C¥X¤§ªù¸¹)\n", i + 2);
	prints("%4d Â÷¶}¥»¨t²Î\n\n", 0);


	while (1) {
		sprintf(genbuf, "½Ð¿ï¾Ü±z©Ò­n¶Ç©Iªºªù¸¹½s¸¹[0-%d] (v ¬d¬Ýª©¥»): ", i + 2);
		strcpy(ans, "");
		getdata(20, 0, genbuf, ans, 3, DOECHO, YEA);

		if (strcmp(ans, "") == 0)
			return 1;

		switch (ans[0]) {
		case '0':
		case 'q':
		case 'Q':
			return 1;
		case 'v':
		case 'V':
			return 1998;
		default:
			/* ¨S¿ìªk, ¥u¦n¥Î default .. */
			if (atoi(ans) <= i && atoi(ans) >= 1) {
				return (_bbcall(atoi(ans) - 1));
			} else if (atoi(ans) == i + 1)
				return (_calluser("*"));
			else if (atoi(ans) == i + 2)
				return (_usersetup());
			else
				continue;
		}
	}

}

int
version()
{
	extern char socket_c[], html_c[];
	clear();
	move(1, 0);
	prints("BBS 2 Pager (c)1999 by[1;36m Edward Chuang, SEEDNet BBS Project $Revision: 1.1 $ [0m\n");
	prints("Here's the CVS ID for this stuff: \n\n");
	prints("paging.c: %s\n", paging_c);
	prints("socket.c: %s\n", socket_c);
	prints("html.c  : %s\n", html_c);
	prints("\n\nOriginal idea delivers from MoonStar BBS (MoonStar.twbbs.org)\n");
	prints("port and socket implemention by edwardc.bbs@bbs.seed.net.tw\n\n");
	prints("first appeared in Firebird BBS 3.0 on  Apr 29 1999 \n");
	pressanykey();

}

char   *
_getid(int len, int startx, char *prompt)
{
	static char idbuf[10];
	while (1) {
		strcpy(idbuf, "");
		getdata(startx, 0, prompt, idbuf, len + 1, DOECHO, YEA);
		if (strcmp(idbuf, "") == 0) {
			return "";
		} else if (strlen(idbuf) != len || atoi(idbuf) == 0) {
			move(startx + 1, 0);
			prints("½Ð­«·s¿é¤J!");
		} else {
			break;
		}
	}
	return ((char *) idbuf);
}

char   *
_getmsg(int len, int startx, char *lineprompt, char *allowchar)
{
	int     i, fail = 0, linelen, x, xx;
	static char msgbuf[256];
	char    msg[80];
	msgbuf[0] = '\0';

	x = 80 - strlen(lineprompt) - 3;
	xx = startx - 1;
	while (len != 0 && fail == 0) {

		if (len < x) {
			linelen = len;
			len = 0;
		} else {
			linelen = x;
			len -= x;
		}
		strcpy(msg, "");
		getdata(startx, 0, lineprompt, msg, linelen + 1, DOECHO, YEA);
		if (strcmp(msg, "") == 0 && strcmp(msgbuf, "") == 0)
			return "";

		move(xx, 0);
		prints("           ");

		fail = 0;
		for (i = 0; i < strlen(msg); i++) {
			if (strcmp(allowchar, "") != 0) {
				if (!strchr(allowchar, msg[i])) {
					fail++;
					break;
				}
			}
		}

		if (fail != 0) {
			move(xx, 0);
			prints("½Ð­«·s¿é¤J!");
			fail = 0;
			len += x;
		} else {
			msg[strlen(msg)] = '\0';
			strcat(msgbuf, msg);
			startx++;
		}

	}
	return (msgbuf);
}

int
_gettype(int startx)
{
	char    msg[5];
	while (1) {
		strcpy(msg, "");
		getdata(startx, 0, "¹ï¤è¬°¼Æ¦r¾÷(N)©Î¤å¦r¾÷(A) (©Î 0 ¸õ¥X)[N/A]: ",
			msg, 3, DOECHO, YEA);

		switch (msg[0]) {
		case 'N':
		case 'n':
			return 1;
		case 'A':
		case 'a':
			return 2;
		case '0':
			return 0;
		default:

		}
	}

}

char   *
_getpwd(int x, int len)
{
	static char passwd[10];
	sprintf(genbuf, "½Ð¿é¤J¶Ç©I±K½X, ¦@ %d ½X(¥i²¤¹L): ", len);
	strcpy(passwd, "");
	getdata(x, 0, genbuf, passwd, len + 1, DOECHO, YEA);

	return ((char *) passwd);

}

char   *
_uri_encode(char *mesg)
{

	static char ret[256];
	char    ch[5];
	int     i;
	strcpy(ret, "");

	for (i = 0; i < strlen(mesg); i++) {
		if (mesg[i] == ' ') {
			strcat(ret, "+");
		} else if (mesg[i] == '?' || mesg[i] == '&' || mesg[i] == '/' ||
			mesg[i] == '=' || mesg[i] == '#' || mesg[i] == '%') {
			sprintf(ch, "%%%X", mesg[i]);
			strcat(ret, ch);
		} else {
			sprintf(ch, "%c", mesg[i]);
			strcat(ret, ch);
		}
	}

	return ((char *) ret);

}

int
_bbcall(int vendorid)
{
	char    buf[256], mesg[256], id[10], ix[5], pass[10], passstr[30];
	int     type;
	sprintf(buf, "[ºô¸ô¶Ç©I -- %s(%s)]", services[vendorid].ident,
		services[vendorid].id);

	clear();
	prints("%s\n", buf);

	sprintf(buf, "½Ð¿é¤Jáà¾÷¸¹½X(¦@¤»½X) %s-", services[vendorid].id);
	strcpy(id, _getid(6, 2, buf));
	if (strcmp(id, "") == 0)
		return 0;

	if (services[vendorid].type[0] != 0) {
		type = _gettype(5);
		if (type == 0)
			return 0;
	} else {
		type = 1;
	}

	strcpy(pass, _getpwd(6, 10));

	if (strcmp(pass, "") != 0) {
		sprintf(passstr, "%s", pass);
	} else {
		strcpy(passstr, "");
	}

	if (type == 1) {
		prints("½Ð¿é¤J­n¶Ç©Iªº°T®§(¥þ¼Æ¦r, ³Ìªø %d ­Ó¼Æ¦r, ¥i¬° 0~9,-,(,),U,¤ÎªÅ¥Õ)\n",
			services[vendorid].type[1]);
		strcpy(mesg, _getmsg(services[vendorid].type[1], 9, ": ", "0123456789-()U "));
	} else if (type == 2) {
		prints("½Ð¿é¤J­n¶Ç©Iªº°T®§(¤å¦r¾÷, ¥i¬°¤¤­^¤å, ³Ìªø %d ­Ó­^¼Æ¦r)\n",
			services[vendorid].type[1]);
		strcpy(mesg, _getmsg(services[vendorid].type[2], 9, ": ", ""));
	}
	if (strcmp(mesg, "") == 0) {
		return 0;
	}
	move(12, 0);
	prints("¶Ç°e¤¤, ½Ðµy­Ô .. :) ....");
	refresh();

	strcpy(ix, services[vendorid].id);

	if (strcmp(ix, "0940") == 0) {
		sprintf(buf, "PrefixCode=0940&subid=%s&PagerType=S&fpoption=0&callmessage=%s",
			id, _uri_encode(mesg));
	} else if (strcmp(ix, "0941") == 0 || strcmp(ix, "0951") == 0 ) {
		sprintf(buf, "PRE_PNO=%s&PAGER_NO=%s&MSG_TYPE=NUMERIC&TRAN_MSG=%s&NOW=on",
			ix, id, _uri_encode(mesg));
	} else if (strcmp(ix, "0943") == 0) {
		sprintf(buf, "CoID=0943&ID=%s&txPasswd=%s&Msg=%s", id
			,(passstr) ? "" : passstr, _uri_encode(mesg));
	} else if (strcmp(ix, "0945") == 0) {
		sprintf(buf,
			"hiUsage=AlphaPage&hiLanguage=Taiwan&hiDataDir=R:\\fiss\\RmtFiles&hiInterval=5&txPagerNo=%s&txPasswd=%s&txSender=%s%s&txContent=%s"
			,id, (passstr) ? "" : passstr,
			currentuser.userid, _BBSID, _uri_encode(mesg));
	} else if (strcmp(ix, "0946") == 0) {
		sprintf(buf, "CoID=0946&ID=%s&txPasswd=%s&Msg=%s", id
			,(passstr) ? "" : passstr, _uri_encode(mesg));
	} else if (strcmp(ix, "0947") == 0) {
		sprintf(buf, "/scripts/fp_page1.dll?AccountNo_0=%s&Password_0=%s&Sender=%s%s&Message=%s",
			id, passstr, currentuser.userid, _BBSID, _uri_encode(mesg));
	} else if (strcmp(ix, "0948") == 0) {
		sprintf(buf,
			"MfcISAPICommand=SinglePage&svc_no=%s&reminder=0&message=%s"
			,id, _uri_encode(mesg));
	} else if (strcmp(ix, "0949") == 0) {
		sprintf(buf, "func=Alphapg&txPagerNo=%s&txSender=%s%s%s&txContent=%s&tran_type=on"
			,id, currentuser.userid, _BBSID, (passstr) ? "" : passstr
			,_uri_encode(mesg));
	} else {
		prints("ERROR: the vendor %s seems not in services list? pls. contact SYSOP.", ix);
		sprintf(buf, "error, vendor %s NOT in services list.", ix);
		report(buf);
		pressreturn();
		return 0;
	}

	dolog(services[vendorid].id, id, buf);

	if (dopage(services[vendorid].path, services[vendorid].site,
			services[vendorid].ref, atoi(services[vendorid].port), buf))
		prints("¶Ç©I°T®§°e¥X! °T®§¦^À³¤w±H¦Ü±zªº«H½c. \n");
	else
		prints("¶Ç©I¥¢±Ñ! ¥¢±Ñ­ì¦]±N±H¦^¦Ü±zªº«H½c. \n");

	pressanykey();
	return 0;

}

int
_calluser(char *userid)
{
	char    buf[256], mesg[256], ix[5], iy[10], who[IDLEN + 1], passwd[10],
	        ch[3];
	int     i, isfriend = 0, type = 0;
	struct override p;
	FILE   *fp;
	/*
	 * edwardc.990219 in some version of FirebirdBBS, older than 2.66M,
	 * override struct called friend.
	 */

	clear();
	modify_user_mode(BBSPAGER);

	prints("[ºô¸ô¶Ç©I -- ¶Ç©I¯¸¤W¨Ï¥ÎªÌ]");

	if (*userid != '*') {
		strcpy(who, userid);
	} else {
		while (1) {
			strcpy(who, "");
			getdata(1, 0, "½Ð¿é¤J§A­n¶Ç©Iªº¨Ï¥ÎªÌ id: ", who, IDLEN + 1, DOECHO, YEA);
			move(2, 0);
			prints("                                        ");

			if (strcmp(who, "") == 0)
				return 0;
			if (!searchuser(who)) {
				move(2, 0);
				prints("¨Ï¥ÎªÌ id ¨Ã¤£¦s¦b!");
			} else {
				break;
			}
		}
	}

	sethomefile(buf, who, "friends");

	fp = fopen(buf, "r");

	if (fp != NULL) {
		while (fread(&p, sizeof(p), 1, fp) != 0) {
			if (strcmp(p.id, currentuser.userid) == 0) {
				isfriend = 1;
				break;
			}
		}
		fclose(fp);
	}
	/* getting it's setting .. */

	sethomefile(buf, who, "bbcall");

	fp = fopen(buf, "r");
	if (fp == NULL) {
		move(2, 0);
		prints("¸Ó¨Ï¥ÎªÌ¨Ã¥¼³]©wáà¾÷.");
		pressanykey();
		return 0;
	}
	fgets(buf, 80, fp);
	fclose(fp);

	i = 0;

	strcpy(ix, (char *) stringtoken(buf, ' ', &i));
	strcpy(iy, (char *) stringtoken(buf, ' ', &i));
	strcpy(passwd, (char *) stringtoken(buf, ' ', &i));

	if (passwd[0] == '*')
		strcpy(passwd, "");

	strcpy(ch, (char *) stringtoken(buf, ' ', &i));
	if (ch[0] == 'Y') {
		sethomefile(mesg, who, "friends");

		fp = fopen(mesg, "r");

		if (fp != NULL) {
			while (fread(&p, sizeof(p), 1, fp) != 0) {
				if (strcmp(p.id, currentuser.userid) == 0) {
					isfriend = 1;
					break;
				}
			}
			fclose(fp);
		}
		if (isfriend == 0) {
			move(2, 0);
			prints("¹ï¤£°_, %s ¥u±µ¨ü¦n¤Íªº¶Ç©I!", who);
			sprintf(mesg, "%s reject bbcall from %s\n", who, currentuser.userid);
			report(mesg);
			pressanykey();
			return 0;
		}
	}
	strcpy(ch, (char *) stringtoken(buf, ' ', &i));

	if (ch[0] == 'A')
		type = 2;
	else
		type = 1;

	move(3, 0);

	for (i = 0;; i++) {
		if (strcmp(services[i].id, "") == 0)
			break;
		if (strcmp(services[i].id, ix) == 0) {
			break;
		}
	}

	if (type == 1) {
		prints("½Ð¿é¤J­n¶Ç©Iªº°T®§(¥þ¼Æ¦r, ³Ìªø %d ­Ó¼Æ¦r, ¥i¬° 0~9,-,(,),U,¤ÎªÅ¥Õ)\n",
			services[i].type[1]);
		strcpy(mesg, _getmsg(services[i].type[1], 5, ": ", "0123456789-()U "));
	} else if (type == 2) {
		prints("½Ð¿é¤J­n¶Ç©Iªº°T®§(¤å¦r¾÷, ¥i¬°¤¤­^¤å, ³Ìªø %d ­Ó­^¼Æ¦r)\n",
			services[i].type[1]);
		strcpy(mesg, _getmsg(services[i].type[2], 5, ": ", ""));
	}
	if (strcmp(mesg, "") == 0)
		return (0);

	move(9, 0);
	if (askyn("½T©w°e¥X?", YEA, NA) == NA) {
		pressanykey();
		return 0;
	}
	/*
	 * 990224.edwardc	rewrite at all by POST method, it's may
	 * effect for all services
	 */
	if (strcmp(ix, "0940") == 0) {
		sprintf(buf, "PrefixCode=0940&subid=%s&PagerType=S&fpoption=0&callmessage=%s",
			iy, _uri_encode(mesg));
	} else if (strcmp(ix, "0941") == 0 || strcmp(ix, "0951") == 0 ) {
		sprintf(buf, "PRE_PNO=%s&PAGER_NO=%s&MSG_TYPE=NUMERIC&TRAN_MSG=%s&NOW=on",
			ix, iy, _uri_encode(mesg));
	} else if (strcmp(ix, "0943") == 0) {
		sprintf(buf, "CoID=0943&ID=%s&txPasswd=%s&Msg=%s", iy,
			(passwd) ? "" : passwd, _uri_encode(mesg));
	} else if (strcmp(ix, "0945") == 0) {
		sprintf(buf,
			"hiUsage=AlphaPage&hiLanguage=Taiwan&hiDataDir=R:\\fiss\\RmtFiles&hiInterval=5&txPagerNo=%s&txPasswd=%s&txSender=%s%s&txContent=%s"
			,iy, (passwd) ? "" : passwd,
			currentuser.userid, _BBSID, _uri_encode(mesg));
	} else if (strcmp(ix, "0946") == 0) {
		sprintf(buf, "CoID=0946&ID=%s&txPasswd=%s&Msg=%s", iy
			,(passwd) ? "" : passwd, _uri_encode(mesg));
	} else if (strcmp(ix, "0947") == 0) {
		sprintf(buf, "/scripts/fp_page1.dll?AccountNo_0=%s&Password_0=%s&Sender=%s%s&Message=%s",
			iy, passwd, currentuser.userid, _BBSID, _uri_encode(mesg));
	} else if (strcmp(ix, "0948") == 0) {
		sprintf(buf,
			"MfcISAPICommand=SinglePage&svc_no=%s&reminder=0&message=%s"
			,iy, _uri_encode(mesg));
	} else if (strcmp(ix, "0949") == 0) {
		sprintf(buf, "func=Alphapg&txPagerNo=%s&txSender=%s%s&txPasswd=%s&txContent=%s&tran_type=on"
			,iy, currentuser.userid, _BBSID
			,(passwd) ? "" : passwd, _uri_encode(mesg));
	} else {
		prints("ERROR: the vendor %s seems not in services list? pls. contact SYSOP.", ix);
		sprintf(buf, "error, vendor %s of %s NOT in services list.", ix, who);
		report(buf);
		pressreturn();
		return 0;
	}

	prints("¶Ç°e¤¤.. ½Ðµy­Ô..");
	refresh();

	sprintf(genbuf, "%s %s", who, ix);
	dolog(genbuf, iy, mesg);

	if (dopage(services[i].path, services[i].site,
			services[i].ref, atoi(services[i].port), buf))
		prints("¶Ç©I°T®§°e¥X! °T®§¦^À³¤w±H¦Ü±zªº«H½c. \n");
	else
		prints("¶Ç©I¥¢±Ñ! ¥¢±Ñ­ì¦]±N±H¦^¦Ü±zªº«H½c. \n");

	pressanykey();
	return 0;

}

int
_nyet()
{
	prints("not implement yet");
	pressreturn();
	return 0;
}

int
dolog(char *id, char *id2, char *msg)
{
	FILE   *fp;
	time_t  now;
	fp = fopen("log/bbcall.log", "a");

	if (fp == NULL)
		return 1;

	now = time(0);

	fprintf(fp, "[%s] %s sendto %s-%s with mesg \n: %s\n", Cdate(&now),
		currentuser.userid, id, id2, msg);

	fclose(fp);

}

int
_usersetup()
{
	char    id[15], ix[5], iy[7], buf[80], passwd[10];
	int     i, found = 0, friend = 0, mailalert = 0, type = 0;
	clear();
	modify_user_mode(BBSPAGER);

	prints("[ºô¸ô¶Ç©I -- ¨Ï¥ÎªÌáà¾÷³]©w]");

	sethomefile(buf, currentuser.userid, "bbcall");

	if (dashf(buf)) {
		while (1) {
			getdata(1, 0, "§A¥Ø«e¤w¦³³]©w, ½Ð¿ï¾Ü 0)Â÷¶} 1)­×§ï³]©w 2)¨ú®ø³]©w :",
				id, 2, DOECHO, YEA);
			if (id[0] == '1')
				break;
			else if (id[0] == '0' || strcmp(id, "") == 0)
				return 0;
			else if (id[0] == '2') {
				unlink(buf);
				return 0;
			}
		}
	}
	while (1) {

		strcpy(id, "");
		getdata(2, 0, "½Ð¿é¤Jáà¾÷ªù¸¹, (ex: 0941123456) ¦@ 10 ½X: ", id, 11,
			DOECHO, YEA);

		if (strcmp(id, "") == 0)
			return 0;

		move(3, 0);
		prints("                                                       ");

		if (strlen(id) == 10 && atol(id) != 0) {

			strcpy(ix, (char *) substr(id, 0, 3));
			strcpy(iy, (char *) substr(id, 4, 9));

			for (i = 0;; i++) {
				if (strcmp(services[i].id, "") == 0)
					break;
				if (strcmp(services[i].id, ix) == 0) {
					found = 1;
					break;
				}
			}

			if (found)
				break;
			else {
				move(3, 0);
				prints("±z©Ò¿é¤Jªºªù¸¹¨Ã¤£¦b¤ä´©½d³ò¤¤, ½Ð­«·s¿é¤J!");
				continue;
			}
		}
		move(3, 0);
		prints("½Ð­«·s¿é¤J!");
	}

	strcpy(passwd, "");
	getdata(4, 0, "½Ð¿é¤J¶Ç©I±K½X(­YµL³]©w©Î¤£¤ä´©½Ð¸õ¹L): ", passwd, 6,
		DOECHO, YEA);

	if (askyn("¬O§_¥u¤¹³\\¦n¤Í©I¥s", NA, NA) == YEA)
		friend = 1;

	type = 0;
	if (services[i].type[0] != 0) {
		if (askyn("¬O§_¬°¼Æ¦r¾÷(§_«h¬°¤¤¤å¾÷)", NA, NA) == NA)
			type = 1;
	}
	if (type) {
		if (askyn("«H½c·s«H³qª¾ (¤¤¤å¾÷¤~¦³)(©|¥¼§¹¦¨)", NA, NA) == YEA)
			mailalert = 1;
	}
	prints("\n§Aªº³]©w: \n");
	prints("ªù    ¸¹: %s-%s\n", ix, iy);
	prints("¶Ç©I±K½X: %s\n", (passwd) ? "µL³]©w" : passwd);
	prints("¤¹³\\¦n¤Í: %s\n", (friend) ? "¬O" : "§_");
	prints("¾÷    «¬: %s\n", (type) ? "¤¤¤å¾÷" : "¼Æ¦r¾÷");
	if (type)
		prints("·s«H³qª¾: %s\n", (mailalert) ? "¬O" : "§_");

	prints("\n");

	if (askyn("¥H¤W³]©w¬O§_¥¿½T", YEA, NA) == YEA) {
		FILE   *fp;
		sethomefile(buf, currentuser.userid, "bbcall");
		fp = fopen(buf, "w");

		if (fp == NULL) {
			prints("open %s error, please report the SYSOP.\n");
			pressreturn();
			return 0;
		}
		fprintf(fp, "%s %s %s %c %c %c\n",
			ix, iy,
			(passwd) ? "*" : passwd,
			(friend) ? 'Y' : 'N',
			(type) ? 'A' : 'N',
			(mailalert) ? 'Y' : 'N');

		fclose(fp);
	}
	return 0;
}
