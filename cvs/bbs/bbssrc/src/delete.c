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
$Id: delete.c,v 1.1 2000/01/15 01:45:28 edwardc Exp $
*/

#include "bbs.h"
#ifdef WITHOUT_ADMIN_TOOLS
#define kick_user
#endif

int
offline()
{
	char    buf[STRLEN];
	modify_user_mode(OFFLINE);
	clear();
	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_BOARDS) || HAS_PERM(PERM_ADMINMENU)
		|| HAS_PERM(PERM_SEEULEVELS)) {
		move(1, 0);
		prints("\n\n±z¦³­«¥ô¦b¨­, ¤£¯àÀH«K¦Û±þ°Õ!!\n");
		pressreturn();
		clear();
		return;
	}
	if (currentuser.stay < 86400) {
		move(1, 0);
		prints("\n\n¹ï¤£°_, ±zÁÙ¥¼°÷¸ê®æ°õ¦æ¦¹©R¥O!!\n");
		prints("½Ð mail µ¹ SYSOP »¡©ú¦Û±þ­ì¦], ÁÂÁÂ¡C\n");
		pressreturn();
		clear();
		return;
	}
	getdata(1, 0, "½Ð¿é¤J§Aªº±K½X: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd(currentuser.passwd, buf)) {
		prints("\n\n«Ü©êºp, ±z¿é¤Jªº±K½X¤£¥¿½T¡C\n");
		pressreturn();
		clear();
		return;
	}
	getdata(3, 0, "½Ð°Ý§A¥s¤°»ò¦W¦r? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentuser.realname)) {
		prints("\n\n«Ü©êºp, §Ú¨Ã¤£»{ÃÑ§A¡C\n");
		pressreturn();
		clear();
		return;
	}
	clear();
	move(1, 0);
	prints("[1;5;31mÄµ§i[0;1;31m¡G ¦Û±þ«á, ±z±NµLªk¦A¥Î¦¹±b¸¹¶i¤J¥»¯¸¡I¡I");
	prints("\n\n\n[1;32m¦ý±b¸¹­n¦b 30 ¤Ñ«á¤~·|§R°£¡C¦nÃø¹L³á :( .....[m\n\n\n");
	if (askyn("§A½T©w­nÂ÷¶}³o­Ó¤j®a®x", NA, NA) == 1) {
		clear();
		currentuser.userlevel = 0;
		substitute_record(PASSFILE, &currentuser, sizeof(struct userec), usernum);
		mail_info();
		modify_user_mode(OFFLINE);
		kick_user(&uinfo);
		exit(0);
	}
}
getuinfo(fn)
FILE   *fn;
{
	int     num;
	char    buf[40];
	fprintf(fn, "\n\n¥Lªº¥N¸¹     : %s\n", currentuser.userid);
	fprintf(fn, "¥Lªº¼ÊºÙ     : %s\n", currentuser.username);
	fprintf(fn, "¯u¹ê©m¦W     : %s\n", currentuser.realname);
	fprintf(fn, "©~¦í¦í§}     : %s\n", currentuser.address);
	fprintf(fn, "¹q¤l¶l¥ó«H½c : %s\n", currentuser.email);
	fprintf(fn, "¯u¹ê E-mail  : %s\n", currentuser.reginfo);
	fprintf(fn, "Ident ¸ê®Æ   : %s\n", currentuser.ident);
	fprintf(fn, "±b¸¹«Ø¥ß¤é´Á : %s", ctime(&currentuser.firstlogin));
	fprintf(fn, "³Ìªñ¥úÁ{¤é´Á : %s", ctime(&currentuser.lastlogin));
	fprintf(fn, "³Ìªñ¥úÁ{¾÷¾¹ : %s\n", currentuser.lasthost);
	fprintf(fn, "¤W¯¸¦¸¼Æ     : %d ¦¸\n", currentuser.numlogins);
	fprintf(fn, "¤å³¹¼Æ¥Ø     : %d\n", currentuser.numposts);
	fprintf(fn, "¤W¯¸Á`®É¼Æ   : %d ¤p®É %d ¤ÀÄÁ\n",
		currentuser.stay / 3600, (currentuser.stay / 60) % 60);
	strcpy(buf, "bTCPRp#@XWBA#VS-DOM-F012345678");
	for (num = 0; num < 30; num++)
		if (!(currentuser.userlevel & (1 << num)))
			buf[num] = '-';
	buf[num] = '\0';
	fprintf(fn, "¨Ï¥ÎªÌÅv­­   : %s\n\n", buf);
}
mail_info()
{
	FILE   *fn;
	time_t  now;
	char    filename[STRLEN];
	now = time(0);
	sprintf(filename, "tmp/suicide.%s", currentuser.userid);
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn, "[1m%s[m ¤w¸g¦b [1m%24.24s[m µn°O¦Û±þ¤F¡A¥H¤U¬O¥Lªº¸ê®Æ¡A½Ð«O¯d...", currentuser.userid
			,ctime(&now));
		getuinfo(fn);
		fclose(fn);
		postfile(filename, "syssecurity", "µn°O¦Û±þ³qª¾(30¤Ñ«á¥Í®Ä)...", 2);
		unlink(filename);
	}
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn, "¤j®a¦n,\n\n");
		fprintf(fn, "§Ú¬O %s (%s)¡C§Ú¤v¸gµn°O¦b30¤Ñ«áÂ÷¶}³o¸Ì¤F¡C\n\n",
			currentuser.userid, currentuser.username);
		fprintf(fn, "§Ú¤£·|§ó¤£¥i¯à§Ñ°O¦Û %s¥H¨Ó¦b¥»¯¸ %d ¦¸ login ¤¤Á`¦@ %d ¤ÀÄÁ³r¯d´Á¶¡ªºÂIÂIºwºw¡C\n",
			ctime(&currentuser.firstlogin), currentuser.numlogins, currentuser.stay / 60);
		fprintf(fn, "½Ð§Úªº¦n¤Í§â %s ±q§A­Ìªº¦n¤Í¦W³æ¤¤®³±¼§a¡C¦]¬°§Ú¤v¸g¨S¦³Åv­­¦A¤W¯¸¤F!\n\n",
			currentuser.userid);
		fprintf(fn, "©Î³\\¦³´Â¤@¤é§Ú·|¦^¨Óªº¡C ¬Ã­«!! ¦A¨£!!\n\n\n");
		fprintf(fn, "%s ©ó %24.24s ¯d.\n\n", currentuser.userid, ctime(&now));
		fclose(fn);
		postfile(filename, "notepad", "µn°O¦Û±þ¯d¨¥...", 2);
		unlink(filename);
	}
}
