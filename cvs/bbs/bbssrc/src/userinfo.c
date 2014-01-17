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
$Id: userinfo.c,v 1.12 2002/06/12 12:39:39 edwardc Exp $
*/

#include "bbs.h"

extern time_t login_start_time;
extern char fromhost[60];
char   *
genpasswd();
char   *sysconf_str();

void
disply_userinfo(u, real)
struct userec *u;
int     real;
{
	int     num, diff;
	int     exp;
#ifdef REG_EXPIRED
	time_t  nextreg, now;
#endif

	move(2, 0);
	clrtobot();
	prints("±zªº¥N¸¹     : %-40s", u->userid);
	if (real)
		prints("      ©Ê§O : %s", (u->gender == 'M' ? "¨k" : "¤k"));
	prints("\n±zªº¼ÊºÙ     : %s\n", u->username);
	prints("¯u¹ê©m¦W     : %-40s", u->realname);
	if (real)
		prints("  ¥X¥Í¤é´Á : %d/%d/%d", u->birthmonth, u->birthday, u->birthyear + 1900);
	prints("\n©~¦í¦í§}     : %s\n", u->address);
	prints("¹q¤l¶l¥ó«H½c : %s\n", u->email);
	if (real) {
		prints("¯u¹ê E-mail  : %s\n", u->reginfo);
		if HAS_PERM
			(PERM_ADMINMENU)
				prints("Ident ¸ê®Æ   : %s\n", u->ident);
	}
	prints("²×ºÝ¾÷§ÎºA   : %s\n", u->termtype);
	prints("±b¸¹«Ø¥ß¤é´Á : %s", ctime(&u->firstlogin));
	prints("³Ìªñ¥úÁ{¤é´Á : %s", ctime(&u->lastlogin));
	if (real) {

		/* edwardc.990410 Åã¥Ü¤U¦¸¨­¥÷½T»{®É¶¡¤ñ¸û¹ê¥Î ? :p */
#ifndef REG_EXPIRED
		prints("¨­¥÷½T»{¤é´Á : %s", (u->lastjustify == 0) ? "¥¼´¿µù¥U\n" : Ctime(&u->lastjustify));
#else
		nextreg = u->lastjustify + REG_EXPIRED * 86400;
		prints("¨­¥÷½T»{     : %s", (u->lastjustify == 0) ? "¥¼´¿µù¥U" : ((nextreg - now) < 0 ) ? "[1;31mµù¥U¹L´Á[0m" : "¤w§¹¦¨¡A¦³®Ä´Á­­: ");
		now = time(0);
		sprintf(genbuf, "¡A%s %d ¤Ñ\n", ((nextreg - now) < 0) ? "¶W¹L" : "ÁÙ¦³", (nextreg - now) / 86400);
		prints("%s%s", (u->lastjustify == 0) ? "" : (char *) Ctime(&nextreg)
			,(u->lastjustify == 0) ? "\n" : genbuf);
#endif

		prints("³Ìªñ¥úÁ{¾÷¾¹ : %s\n", u->lasthost);
		prints("¤å³¹¼Æ¥Ø     : %d\n", u->numposts);
		prints("¨p¤H«H½c     : %d «Ê\n", u->nummails);
	}
	prints("¤W¯¸¦¸¼Æ     : %d ¦¸\n", u->numlogins);
	prints("¤W¯¸Á`®É¼Æ   : %d ¤p®É %d ¤ÀÄÁ ( %d ¦~ %d ¤ë %d ¤Ñ )\n", u->stay / 3600, (u->stay / 60) % 60
	, u->stay / 31536000 , ((u->stay / 2628000) % 2628000) % 12, ((u->stay / 86400 ) % 86400) % 30 );
	/* chinsan.20011229: ¶â...³o¸ÌÀ³¸Ó¨S°ÝÃD¤F */
	
	exp = countexp(u);
	prints("¸gÅç­È       : %d  (%s)\n", exp, cexp(exp));
	exp = countperf(u);
	prints("ªí²{­È       : %d  (%s)\n", exp, cperf(exp));
	if (real) {
		strcpy(genbuf, "bTCPRp#@XWBA#VS-DOM-F012345678\0");
		for (num = 0; num < strlen(genbuf) - 1; num++)
			if (!(u->userlevel & (1 << num)))
				genbuf[num] = '-';
		prints("¨Ï¥ÎªÌÅv­­   : %s\n", genbuf);
	} else {
		diff = (time(0) - login_start_time) / 60;
		prints("°±¯d´Á¶¡     : %d ¤p®É %02d ¤À\n", diff / 60, diff % 60);
		prints("¿Ã¹õ¤j¤p     : %dx%d\n", t_lines, t_columns);
	}
	prints("\n");
	if (u->userlevel & PERM_BOARDS) {
		prints("  ±z¬O¥»¯¸ªºªO¥D, ·PÁÂ±zªº¥I¥X.\n");
	} else if (u->userlevel & PERM_LOGINOK) {
		prints("  ±zªºµù¥Uµ{§Ç¤w¸g§¹¦¨, Åwªï¥[¤J¥»¯¸.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  ·s¤â¤W¸ô, ½Ð¾\\Åª Announce °Q½×°Ï.\n");
	} else {
		prints("  µù¥U©|¥¼¦¨¥\\, ½Ð°Ñ¦Ò¥»¯¸¶i¯¸µe­±»¡©ú.\n");
	}
}


int
uinfo_query(u, real, unum)
struct userec *u;
int     real, unum;
{
	struct userec newinfo;
	char    ans[3], buf[STRLEN], genbuf[128];
	char    src[STRLEN], dst[STRLEN];
	int     i, fail = 0, netty_check = 0;
	time_t  now;
	struct tm *tmnow;
	memcpy(&newinfo, u, sizeof(currentuser));
	getdata(t_lines - 1, 0, real ?
		"½Ð¿ï¾Ü (0)µ²§ô (1)­×§ï¸ê®Æ (2)³]©w±K½X (3) §ï ID ==> [0]" :
		"½Ð¿ï¾Ü (0)µ²§ô (1)­×§ï¸ê®Æ (2)³]©w±K½X (3) ¿ïÃ±¦WÀÉ ==> [0]",
		ans, 2, DOECHO, YEA);
	clear();
	refresh();

	now = time(0);
	tmnow = localtime(&now);

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("¨Ï¥ÎªÌ¥N¸¹: %s\n", u->userid);

	switch (ans[0]) {
	case '1':
		move(1, 0);
		prints("½Ð³v¶µ­×§ï,ª½±µ«ö <ENTER> ¥Nªí¨Ï¥Î [] ¤ºªº¸ê®Æ¡C\n");

		sprintf(genbuf, "¼ÊºÙ [%s]: ", u->username);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.username, buf, NAMELEN);
		if (!real && buf[0])
			strncpy(uinfo.username, buf, 40);

		if (real) {
			sprintf(genbuf, "¯u¹ê©m¦W [%s]: ", u->realname);
			getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
			if (buf[0])
				strncpy(newinfo.realname, buf, NAMELEN);
		}

		sprintf(genbuf, "©~¦í¦a§} [%s]: ", u->address);
		getdata(i++, 0, genbuf, buf, STRLEN - 10, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.address, buf, NAMELEN);

		sprintf(genbuf, "¹q¤l«H½c [%s]: ", u->email);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0]) {
			netty_check = 1;
			strncpy(newinfo.email, buf, NAMELEN-12);
		}
		sprintf(genbuf, "²×ºÝ¾÷§ÎºA [%s]: ", u->termtype);
		getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.termtype, buf, 16);

		sprintf(genbuf, "¥X¥Í¦~ [%d]: ", u->birthyear + 1900);
		getdata(i++, 0, genbuf, buf, 5, DOECHO, YEA);
		if (buf[0] && atoi(buf) > tmnow->tm_year + 1805 && atoi(buf) < tmnow->tm_year + 1897)
			newinfo.birthyear = atoi(buf) - 1900;

		sprintf(genbuf, "¥X¥Í¤ë [%d]: ", u->birthmonth);
		getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 12)
			newinfo.birthmonth = atoi(buf);

		sprintf(genbuf, "¥X¥Í¤é [%d]: ", u->birthday);
		getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 31)
			newinfo.birthday = atoi(buf);

		sprintf(genbuf, "©Ê§O [%c](M/F): ", u->gender);
		getdata(i++, 0, genbuf, buf, 2, DOECHO, YEA);
		if (buf[0]) {
			if (strchr("MmFf", buf[0]))
				newinfo.gender = toupper(buf[0]);
		}
		if (real) {
			sprintf(genbuf, "¯u¹êEmail[%s]: ", u->reginfo);
			getdata(i++, 0, genbuf, buf, 62, DOECHO, YEA);
			if (buf[0])
				strncpy(newinfo.reginfo, buf, STRLEN - 16);

			sprintf(genbuf, "¤W½u¦¸¼Æ [%d]: ", u->numlogins);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.numlogins = atoi(buf);

			sprintf(genbuf, "¤å³¹¼Æ¥Ø [%d]: ", u->numposts);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.numposts = atoi(buf);
			
			sprintf(genbuf, "¤W¯¸¬í¼Æ [%d]: ", u->stay);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.stay = atoi(buf);
		}
		break;
	case '2':
		if (!real) {
			getdata(i++, 0, "½Ð¿é¤J­ì±K½X: ", buf, PASSLEN, NOECHO, YEA);
			if (*buf == '\0' || !checkpasswd(u->passwd, buf)) {
				prints("\n\n«Ü©êºp, ±z¿é¤Jªº±K½X¤£¥¿½T¡C\n");
				fail++;
				break;
			}
		}
		getdata(i++, 0, "½Ð³]©w·s±K½X: ", buf, PASSLEN, NOECHO, YEA);
		if (buf[0] == '\0') {
			prints("\n\n±K½X³]©w¨ú®ø, Ä~Äò¨Ï¥ÎÂÂ±K½X\n");
			fail++;
			break;
		}
		strncpy(genbuf, buf, PASSLEN);

		getdata(i++, 0, "½Ð­«·s¿é¤J·s±K½X: ", buf, PASSLEN, NOECHO, YEA);
		if (strncmp(buf, genbuf, PASSLEN)) {
			prints("\n\n·s±K½X½T»{¥¢±Ñ, µLªk³]©w·s±K½X¡C\n");
			fail++;
			break;
		}
		buf[8] = '\0';
		strncpy(newinfo.passwd, genpasswd(buf), ENCPASSLEN);
		break;
	case '3':
		if (!real) {
			sprintf(genbuf, "¥Ø«e¨Ï¥ÎÃ±¦WÀÉ [%d]: ", u->signature);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.signature = atoi(buf);
		} else {
			getdata(i++, 0, "·sªº¨Ï¥ÎªÌ¥N¸¹: ", genbuf, IDLEN + 1, DOECHO, YEA);
			if (*genbuf != '\0') {
				if (getuser(genbuf)) {
					prints("\n¿ù»~! ¤w¸g¦³¦P¼Ë ID ªº¨Ï¥ÎªÌ\n");
					fail++;
				} else {
					strncpy(newinfo.userid, genbuf, IDLEN + 2);
				}
			}
		}
		break;
	default:
		clear();
		return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("½T©w­n§ïÅÜ¶Ü", NA, YEA) == YEA) {
		if (real) {
			char    secu[STRLEN];
			sprintf(secu, "­×§ï %s ªº°ò¥»¸ê®Æ©Î±K½X¡C", u->userid);
			securityreport(secu);
		}
		if (strcmp(u->userid, newinfo.userid)) {

			sprintf(src, "mail/%c/%s", toupper(u->userid[0]), u->userid);
			sprintf(dst, "mail/%c/%s", toupper(newinfo.userid[0]), newinfo.userid);
			rename(src, dst);
			sethomepath(src, u->userid);
			sethomepath(dst, newinfo.userid);
			rename(src, dst);
			sethomefile(src, u->userid, "register");
			unlink(src);
			sethomefile(src, u->userid, "register.old");
			unlink(src);
			setuserid(unum, newinfo.userid);
		}
		if ((netty_check == 1)) {
			sprintf(genbuf, "%s", BBSHOST);
			if ((sysconf_str("EMAILFILE") != NULL) &&
				(!strstr(newinfo.email, genbuf)) &&
				(!invalidaddr(newinfo.email)) &&
				(!invalid_email(newinfo.email))) {
				strcpy(u->email, newinfo.email);
				send_regmail(u);
			} else {
				if (sysconf_str("EMAILFILE") != NULL) {
					move(t_lines - 5, 0);
					prints("\n±z©Ò¶ñªº¹q¤l¶l¥ó¦a§} ¡i[1;33m%s[m¡j\n", newinfo.email);
					prints("®¤¤£¨ü¥»¯¸©Ó»{¡A¨t²Î¤£·|§ë»¼µù¥U«H¡A½Ð§â¥¦­×¥¿¦n...\n");
					pressanykey();
					/*
					 * edwardc.990521 if there's wrong
					 * email, do not update anything.
					 */
					return 0;
				}
			}
		}
		memcpy(u, &newinfo, sizeof(newinfo));
		set_safe_record();
		if (netty_check == 1) {
			newinfo.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			sethomefile(src, newinfo.userid, "register");
			sethomefile(dst, newinfo.userid, "register.old");
			rename(src, dst);
		}
		substitute_record(PASSFILE, &newinfo, sizeof(newinfo), unum);
	}
	clear();
	return 0;
}

void
x_info()
{
	modify_user_mode(GMENU);
	if (!strcmp("guest", currentuser.userid)) {
		disply_userinfo(&currentuser, 0);
		pressreturn();
		return;
	}
	disply_userinfo(&currentuser, 1);
	uinfo_query(&currentuser, 0, usernum);
}

void
getfield(line, info, desc, buf, len)
int     line, len;
char   *info, *desc, *buf;
{
	char    prompt[STRLEN];
	sprintf(genbuf, "  ­ì¥ý³]©w: %-46.46s [1;32m(%s)[m",
		(buf[0] == '\0') ? "(¥¼³]©w)" : buf, info);
	move(line, 0);
	prints(genbuf);
	sprintf(prompt, "  %s: ", desc);
	getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
	if (genbuf[0] != '\0') {
		strncpy(buf, genbuf, len);
	}
	move(line, 0);
	clrtoeol();
	prints("  %s: %s\n", desc, buf);
	clrtoeol();
}

void
x_fillform()
{
	char    rname[NAMELEN], addr[STRLEN];
	char    phone[STRLEN], dept[STRLEN], assoc[STRLEN];
	char    ans[5], *mesg, *ptr;
	FILE   *fn;
	time_t  now;
	modify_user_mode(NEW);
	move(3, 0);
	clrtobot();
	if (!strcmp("guest", currentuser.userid)) {
		prints("©êºp, ½Ð¥Î new ¥Ó½Ð¤@­Ó·s±b¸¹«á¦A¶ñ¥Ó½Ðªí.");
		pressreturn();
		return;
	}
	if (currentuser.userlevel & PERM_LOGINOK) {
		prints("±z¤w¸g§¹¦¨¥»¯¸ªº¨Ï¥ÎªÌµù¥U¤âÄò, Åwªï¥[¤J¥»¯¸ªº¦æ¦C.");
		pressreturn();
		return;
	}
	if ((fn = fopen("new_register", "r")) != NULL) {
		while (fgets(genbuf, STRLEN, fn) != NULL) {
			if ((ptr = strchr(genbuf, '\n')) != NULL)
				*ptr = '\0';
			if (strncmp(genbuf, "userid: ", 8) == 0 &&
				strcmp(genbuf + 8, currentuser.userid) == 0) {
				fclose(fn);
				prints("¯¸ªø©|¥¼³B²z±zªºµù¥U¥Ó½Ð³æ, ½Ð­@¤ßµ¥­Ô.");
				pressreturn();
				return;
			}
		}
		fclose(fn);
	}
	move(3, 0);
	if (askyn("±z½T©w­n¶ñ¼gµù¥U³æ¶Ü", NA, NA) == NA)
		return;
	strncpy(rname, currentuser.realname, NAMELEN);
	strncpy(addr, currentuser.address, STRLEN);
	dept[0] = phone[0] = assoc[0] = '\0';
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s ±z¦n, ½Ð¾Ú¹ê¶ñ¼g¥H¤Uªº¸ê®Æ:\n", currentuser.userid);
		getfield(6, "½Ð¥Î¤¤¤å", "¯u¹ê©m¦W", rname, NAMELEN);
		getfield(8, "¾Ç®Õ¨t¯Å©Î¤½¥qÂ¾ºÙ", "¾Ç®Õ¨t¯Å", dept, STRLEN);
		getfield(10, "¥]¬A¹ì«Ç©ÎªùµP¸¹½X", "¥Ø«e¦í§}", addr, STRLEN);
		getfield(12, "¥]¬A¥iÁpµ¸®É¶¡", "Ápµ¸¹q¸Ü", phone, STRLEN);
#ifdef NEED_ASSOC		/* edwardc.990410 i think it's not general
				 * enough */
		getfield(14, "®Õ¤Í·|©Î²¦·~¾Ç®Õ", "®Õ ¤Í ·|", assoc, STRLEN);
#endif
		mesg = "¥H¤W¸ê®Æ¬O§_¥¿½T, «ö Q ©ñ±óµù¥U (Y/N/Quit)? [N]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return;
		if (ans[0] == 'Y' || ans[0] == 'y')
			break;
	}
	strncpy(currentuser.realname, rname, NAMELEN);
	strncpy(currentuser.address, addr, STRLEN);
	if ((fn = fopen("new_register", "a")) != NULL) {
		now = time(NULL);
		fprintf(fn, "usernum: %d, %s\n", usernum, Ctime(&now));	
			/* edwardc.020612 add a newline in case of register is broken. */
		fprintf(fn, "userid: %s\n", currentuser.userid);
		fprintf(fn, "realname: %s\n", rname);
		fprintf(fn, "dept: %s\n", dept);
		fprintf(fn, "addr: %s\n", addr);
		fprintf(fn, "phone: %s\n", phone);
#ifdef NEED_ASSOC
		fprintf(fn, "assoc: %s\n", assoc);
#endif
		fprintf(fn, "----\n");
		fclose(fn);
	}
	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) == NULL) {
		fclose(fn);
		return;
	}
	fprintf(fn, "usernum: %d\n", usernum);
	fclose(fn);
}
