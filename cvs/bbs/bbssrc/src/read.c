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
$Id: read.c,v 1.1 2000/01/15 01:45:29 edwardc Exp $
*/

#include "bbs.h"

#define PUTCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(">");
#define RMVCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(" ");

struct fileheader SR_fptr;
int     SR_BMDELFLAG = NA;
char   *pnt;

struct keeploc {
	char   *key;
	int     top_line;
	int     crs_line;
	struct keeploc *next;
};


/*struct fileheader *files = NULL;*/
char    currdirect[STRLEN];
char    keyword[256];	/* for ¬ÛÃö¥DÃD */
int     screen_len;
int     last_line;

struct keeploc *
getkeep(s, def_topline, def_cursline)
char   *s;
int     def_topline;
int     def_cursline;
{
	static struct keeploc *keeplist = NULL;
	struct keeploc *p;
	for (p = keeplist; p != NULL; p = p->next) {
		if (!strcmp(s, p->key)) {
			if (p->crs_line < 1)
				p->crs_line = 1;	/* DAMMIT! - rrr */
			return p;
		}
	}
	p = (struct keeploc *) malloc(sizeof(*p));
	p->key = (char *) malloc(strlen(s) + 1);
	strcpy(p->key, s);
	p->top_line = def_topline;
	p->crs_line = def_cursline;
	p->next = keeplist;
	keeplist = p;
	return p;
}

void
fixkeep(s, first, last)
char   *s;
int     first, last;
{
	struct keeploc *k;
	k = getkeep(s, 1, 1);
	if (k->crs_line >= first) {
		k->crs_line = (first == 1 ? 1 : first - 1);
		k->top_line = (first < 11 ? 1 : first - 10);
	}
}

void
modify_locmem(locmem, total)
struct keeploc *locmem;
int     total;
{
	if (locmem->top_line > total) {
		locmem->crs_line = total;
		locmem->top_line = total - t_lines / 2;
		if (locmem->top_line < 1)
			locmem->top_line = 1;
	} else if (locmem->crs_line > total) {
		locmem->crs_line = total;
	}
}

int
move_cursor_line(locmem, mode)
struct keeploc *locmem;
int     mode;
{
	int     top, crs;
	int     reload = 0;
	top = locmem->top_line;
	crs = locmem->crs_line;
	if (mode == READ_PREV) {
		if (crs <= top) {
			top -= screen_len - 1;
			if (top < 1)
				top = 1;
			reload = 1;
		}
		crs--;
		if (crs < 1) {
			crs = 1;
			reload = -1;
		}
	} else if (mode == READ_NEXT) {
		if (crs + 1 >= top + screen_len) {
			top += screen_len - 1;
			reload = 1;
		}
		crs++;
		if (crs > last_line) {
			crs = last_line;
			reload = -1;
		}
	}
	locmem->top_line = top;
	locmem->crs_line = crs;
	return reload;
}

void
draw_title(dotitle)
int     (*dotitle) ();
{
	clear();
	(*dotitle) ();
}

void
draw_entry(doentry, locmem, num, ssize)
char   *(*doentry) ();
struct keeploc *locmem;
int     num, ssize;
{
	char   *str;
	int     base, i;
	base = locmem->top_line;
	move(3, 0);
	clrtobot();
	for (i = 0; i < num; i++) {
		str = (*doentry) (base + i, &pnt[i * ssize]);
		if (!check_stuffmode())
			prints("%s", str);
		else
			showstuff(str, 0);
		prints("\n");
	}
	move(t_lines - 1, 0);
	clrtoeol();
	update_endline();
}


void
i_read(cmdmode, direct, dotitle, doentry, rcmdlist, ssize)
int     cmdmode;
char   *direct;
int     (*dotitle) ();
char   *(*doentry) ();
struct one_key *rcmdlist;
int     ssize;
{
	extern int talkrequest;
	extern int friendflag;
	struct keeploc *locmem;
	char    lbuf[11];
	int     lbc, recbase, mode, ch;
	int     num, entries;
	screen_len = t_lines - 4;
	modify_user_mode(cmdmode);
	pnt = calloc(screen_len, ssize);
	strcpy(currdirect, direct);
	draw_title(dotitle);
	last_line = get_num_records(currdirect, ssize);

	if (last_line == 0) {
		if (cmdmode == RMAIL) {
			prints("¨S¦³¥ô¦ó·s«H¥ó...");
			pressreturn();
			clear();
		} else if (cmdmode == GMENU) {
			char    desc[5];
			char    buf[40];
			if (friendflag)
				strcpy(desc, "¦n¤Í");
			else
				strcpy(desc, "Ãa¤H");
			sprintf(buf, "¨S¦³¥ô¦ó%s (A)·s¼W%s (Q)Â÷¶}¡H[Q] ", desc, desc);
			getdata(t_lines - 1, 0, buf, genbuf, 4, DOECHO, YEA);
			if (genbuf[0] == 'a' || genbuf[0] == 'A')
				(friendflag) ? friend_add() : reject_add();
		} else {
			getdata(t_lines - 1, 0, "¬ÝªO·s¦¨¥ß (P)µoªí¤å³¹ (Q)Â÷¶}¡H[Q] ",
				genbuf, 4, DOECHO, YEA);
			if (genbuf[0] == 'p' || genbuf[0] == 'P')
				do_post();
		}
		free(pnt);
		return;
	}
	num = last_line - screen_len + 2;
	locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
	modify_locmem(locmem, last_line);
	recbase = locmem->top_line;
	entries = get_records(currdirect, pnt, ssize, recbase, screen_len);
	draw_entry(doentry, locmem, entries, ssize);
	PUTCURS;
	lbc = 0;
	mode = DONOTHING;
	while ((ch = egetch()) != EOF) {
		if (talkrequest) {
			talkreply();
			mode = FULLUPDATE;
		} else if (ch >= '0' && ch <= '9') {
			if (lbc < 9)
				lbuf[lbc++] = ch;
		} else if (lbc > 0 && (ch == '\n' || ch == '\r')) {
			lbuf[lbc] = '\0';
			lbc = atoi(lbuf);
			if (cursor_pos(locmem, lbc, 10))
				mode = PARTUPDATE;
			lbc = 0;
		} else {
			lbc = 0;
			mode = i_read_key(rcmdlist, locmem, ch, ssize);

			while (mode == READ_NEXT || mode == READ_PREV) {
				int     reload;
				reload = move_cursor_line(locmem, mode);
				if (reload == -1) {
					mode = FULLUPDATE;
					break;
				} else if (reload) {
					recbase = locmem->top_line;
					entries = get_records(currdirect, pnt, ssize,
						recbase, screen_len);
					if (entries <= 0) {
						last_line = -1;
						break;
					}
				}
				num = locmem->crs_line - locmem->top_line;
				mode = i_read_key(rcmdlist, locmem, ch, ssize);
			}
			modify_user_mode(cmdmode);
		}
		if (mode == DOQUIT)
			break;
		if (mode == GOTO_NEXT) {
			cursor_pos(locmem, locmem->crs_line + 1, 1);
			mode = PARTUPDATE;
		}
		switch (mode) {
		case NEWDIRECT:
		case DIRCHANGED:
			recbase = -1;
			last_line = get_num_records(currdirect, ssize);
			if (last_line == 0 && digestmode > 0) {
				if (digestmode == YEA)
					digest_mode();
				else
					thread_mode();
			}
			if (mode == NEWDIRECT) {
				num = last_line - screen_len + 1;
				locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
			}
		case FULLUPDATE:
			draw_title(dotitle);
		case PARTUPDATE:
			if (last_line < locmem->top_line + screen_len) {
				num = get_num_records(currdirect, ssize);
				if (last_line != num) {
					last_line = num;
					recbase = -1;
				}
			}
			if (last_line == 0) {
				prints("No Messages\n");
				entries = 0;
			} else if (recbase != locmem->top_line) {
				recbase = locmem->top_line;
				if (recbase > last_line) {
					recbase = last_line - screen_len / 2;
					if (recbase < 1)
						recbase = 1;
					locmem->top_line = recbase;
				}
				entries = get_records(currdirect, pnt, ssize,
					recbase, screen_len);
			}
			if (locmem->crs_line > last_line)
				locmem->crs_line = last_line;
			draw_entry(doentry, locmem, entries, ssize);
			PUTCURS;
			break;
		default:
			break;
		}
		mode = DONOTHING;
		if (entries == 0)
			break;
	}
	clear();
	free(pnt);
}

int
i_read_key(rcmdlist, locmem, ch, ssize)
struct one_key *rcmdlist;
struct keeploc *locmem;
int     ch, ssize;
{
	int     i, mode = DONOTHING;
	switch (ch) {
	case 'q':
	case 'e':
	case KEY_LEFT:
		if (digestmode == YEA)
			return digest_mode();
		else if (digestmode == 2)
			return thread_mode();
		else
			return DOQUIT;
	case Ctrl('L'):
		redoscr();
		break;
	case 'k':
	case KEY_UP:
		if (cursor_pos(locmem, locmem->crs_line - 1, screen_len - 2))
			return PARTUPDATE;
		break;
	case 'j':
	case KEY_DOWN:
		if (cursor_pos(locmem, locmem->crs_line + 1, 0))
			return PARTUPDATE;
		break;
	case 'L':		/* ppfoong */
		show_allmsgs();
		return FULLUPDATE;
		break;
	case 'N':
	case Ctrl('F'):
	case KEY_PGDN:
	case ' ':
		if (last_line >= locmem->top_line + screen_len) {
			locmem->top_line += screen_len - 1;
			locmem->crs_line = locmem->top_line;
			return PARTUPDATE;
		}
		RMVCURS;
		locmem->crs_line = last_line;
		PUTCURS;
		break;
	case 'P':
	case Ctrl('B'):
	case KEY_PGUP:
		if (locmem->top_line > 1) {
			locmem->top_line -= screen_len - 1;
			if (locmem->top_line <= 0)
				locmem->top_line = 1;
			locmem->crs_line = locmem->top_line;
			return PARTUPDATE;
		} else {
			RMVCURS;
			locmem->crs_line = locmem->top_line;
			PUTCURS;
		}
		break;
	case KEY_HOME:
		locmem->top_line = 1;
		locmem->crs_line = 1;
		return PARTUPDATE;
	case '$':
	case KEY_END:
		if (last_line >= locmem->top_line + screen_len) {
			locmem->top_line = last_line - screen_len + 1;
			if (locmem->top_line <= 0)
				locmem->top_line = 1;
			locmem->crs_line = last_line;
			return PARTUPDATE;
		}
		RMVCURS;
		locmem->crs_line = last_line;
		PUTCURS;
		break;
	case 'S':		/* youzi */
		if (!HAS_PERM(PERM_PAGE))
			break;
		s_msg();
		return FULLUPDATE;
		break;
	case 'f':		/* youzi */
		if (!HAS_PERM(PERM_BASIC))
			break;
		t_friends();
		return FULLUPDATE;
		break;
	case '!':		/* youzi leave */
		return Q_Goodbye();
		break;
	case '\n':
	case '\r':
	case 'l':
	case KEY_RIGHT:
		ch = 'r';
		/* lookup command table */
	default:
		for (i = 0; rcmdlist[i].fptr != NULL; i++) {
			if (rcmdlist[i].key == ch) {
				mode = (*(rcmdlist[i].fptr)) (locmem->crs_line,
					&pnt[(locmem->crs_line - locmem->top_line) * ssize],
					currdirect);
				break;
			}
		}
	}
	return mode;
}

int
auth_search_down(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_author(locmem, 1, fileinfo->owner))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
auth_search_up(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_author(locmem, -1, fileinfo->owner))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}


int
post_search_down(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_post(locmem, 1))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
post_search_up(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_post(locmem, -1))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
show_author(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	t_query(fileinfo->owner);
	return FULLUPDATE;
}

int
SR_BMfunc(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	int     i, type = 0;
	char    buf[80], ch[4], BMch;
	static char *SR_BMitems[] = {"§R°£", "«O¯d", "¤åºK", "©ñ¤JºëµØ°Ï", "©ñ¤J¼È¦sÀÉ"};
	static char *subBMitems[] = {"¬Û¦P¥DÃD", " ¬Û¦P§@ªÌ", "¬ÛÃö¥DÃD"};
	if (!chk_currBM(currBM)) {
		return DONOTHING;
	}
	saveline(t_lines - 2, 0);
	move(t_lines - 2, 0);
	clrtoeol();

	/* edwardc.990702 °l¥[¬ÛÃö¥DÃD¥\¯à .. */
	/* edwardc.990509 ´Á«Ý¤w¤[ªº¬Û¦P§@ªÌ¥\¯à :) .. */

	ch[0] = '\0';
	getdata(t_lines - 2, 0, "°õ¦æ: 1) ¬Û¦P¥DÃD  2) ¬Û¦P§@ªÌ 3) ¬ÛÃö¥DÃD 0) ¨ú®ø [0]: ", ch, 3, DOECHO, YEA);
	type = atoi(ch);

	if (type < 1 || type > 3) {
		saveline(t_lines - 2, 1);
		return DONOTHING;
	}
	sprintf(buf, "%s (0)¨ú®ø ", subBMitems[type - 1]);

	for (i = 0; i < 5; i++)
		sprintf(buf, "%s(%d)%s ", buf, i + 1, SR_BMitems[i]);

	strcat(buf, "? [0]: ");
	getdata(t_lines - 2, 0, buf, ch, 3, DOECHO, YEA);
	BMch = atoi(ch);
	if (BMch <= 0 || BMch > 5) {
		saveline(t_lines - 2, 1);
		return DONOTHING;
	}
	move(t_lines - 2, 0);	/* ppfoong */
	sprintf(buf, "½T©w­n°õ¦æ%s[%s]¶Ü", subBMitems[type - 1], SR_BMitems[BMch - 1]);
	if (askyn(buf, NA, NA) == 0) {
		saveline(t_lines - 2, 1);
		return DONOTHING;
	}
	if (type == 3) {
		strcpy(keyword, "");
		getdata(t_lines - 2, 0, "½Ð¿é¤J¥DÃDÃöÁä¦r: ", keyword, 50, DOECHO, YEA);
		if (keyword[0] == '\0') {
			saveline(t_lines - 2, 1);
			return DONOTHING;
		}
	} else if (type == 1) {
		strcpy(keyword, fileinfo->title);
	} else {
		strcpy(keyword, fileinfo->owner);
	}

	if (digestmode == 2 && BMch <= 3)
		return;
	if (type < 1 || type > 3) {	/* last check */
		saveline(t_lines - 2, 1);
		return DONOTHING;
	}
	move(t_lines - 2, 0);
	sprintf(buf, "¬O§_±q%s²Ä¤@½g¶}©l%s (Y)²Ä¤@½g (N)¥Ø«e³o¤@½g", (type == 2) ? "¸Ó§@ªÌ" : "¦¹¥DÃD", SR_BMitems[BMch - 1]);
	if (askyn(buf, YEA, NA) == 1) {
		sread(2, 0, ent, type - 1, fileinfo);
		fileinfo = &SR_fptr;
	}
	sread(BMch + SR_BMBASE, 0, ent, type - 1, fileinfo);

	/* ®³±¼¹Æ, Åý syssecurity ªO²M²n¤@ÂI */
#if 0
	/* edwardc.990617 report this action ... */
	sprintf(buf, "¦b %s ªO¤W¹ê¬I %s%s, ÃöÁä¦r: ",
		currboard, subBMitems[type - 1], SR_BMitems[BMch - 1]);
		
	/* edwardc.990813, ¤j B ·|½ð¤H .. */
	if ( strlen(keyword) < 255 )
		strncat(buf, keyword, (255 - strlen(buf)));
	else /* ¨º¦³³o»òªøªº¼ÐÃD @_@ ÄF½Ö°Ú */
		strncat(buf, keyword, 255);
		
	securityreport(buf);
#endif

	return DIRCHANGED;
}

int
SR_first_new(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	sread(2, 0, ent, 0, fileinfo);
	if (sread(3, 0, NULL, 0, &SR_fptr) == -1) {	/* Found The First One */
		sread(0, 1, NULL, 0, &SR_fptr);
		return FULLUPDATE;
	}
	return PARTUPDATE;
}

int
SR_last(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	sread(1, 0, ent, 0, fileinfo);
	return PARTUPDATE;
}

int
SR_first(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	sread(2, 0, ent, 0, fileinfo);
	return PARTUPDATE;
}

int
SR_read(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	sread(0, 1, NULL, 0, fileinfo);
	return FULLUPDATE;
}

int
SR_author(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	sread(0, 1, NULL, 1, fileinfo);
	return FULLUPDATE;
}

int
search_author(locmem, offset, powner)
struct keeploc *locmem;
int     offset;
char   *powner;
{
	static char author[IDLEN + 1];
	char    ans[IDLEN + 1], pmt[STRLEN];
	char    currauth[STRLEN];
	strcpy(currauth, powner);

	sprintf(pmt, "%sªº¤å³¹·j´M§@ªÌ [%s]: ", offset > 0 ? "©¹«á¨Ó" : "©¹¥ý«e", currauth);
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, pmt, ans, IDLEN, DOECHO, YEA);
	if (ans[0] != '\0')
		strcpy(author, ans);
	else
		strcpy(author, currauth);

	return search_articles(locmem, author, offset, 1);
}

int
auth_post_down(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_author(locmem, 1, fileinfo->owner))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
auth_post_up(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_author(locmem, -1, fileinfo->owner))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
t_search_down(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_title(locmem, 1))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}

int
t_search_up(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_title(locmem, -1))
		return PARTUPDATE;
	else
		update_endline();
	return DONOTHING;
}
int
thread_up(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_thread(locmem, -1, fileinfo->title)) {
		update_endline();
		return PARTUPDATE;
	}
	update_endline();
	return DONOTHING;
}

int
thread_down(ent, fileinfo, direct)
int     ent;
struct fileheader *fileinfo;
char   *direct;
{
	struct keeploc *locmem;
	locmem = getkeep(direct, 1, 1);
	if (search_thread(locmem, 1, fileinfo->title)) {
		update_endline();
		return PARTUPDATE;
	}
	update_endline();
	return DONOTHING;
}

int
search_post(locmem, offset)
struct keeploc *locmem;
int     offset;
{
	static char query[STRLEN];
	char    ans[STRLEN], pmt[STRLEN];
	strcpy(ans, query);
	sprintf(pmt, "·j´M%sªº¤å³¹ [%s]: ", offset > 0 ? "©¹«á¨Ó" : "©¹¥ý«e", ans);
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, pmt, ans, 50, DOECHO, YEA);
	if (ans[0] != '\0')
		strcpy(query, ans);

	return search_articles(locmem, query, offset, -1);
}


int
search_title(locmem, offset)
struct keeploc *locmem;
int     offset;
{
	static char title[STRLEN];
	char    ans[STRLEN], pmt[STRLEN];
	strcpy(ans, title);
	sprintf(pmt, "%s·j´M¼ÐÃD [%.16s]: ", offset > 0 ? "©¹«á" : "©¹«e", ans);
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, pmt, ans, 46, DOECHO, YEA);
	if (*ans != '\0')
		strcpy(title, ans);
	return search_articles(locmem, title, offset, 0);
}

int
search_thread(locmem, offset, title)
struct keeploc *locmem;
int     offset;
char   *title;
{

	if (title[0] == 'R' && (title[1] == 'e' || title[1] == 'E') && title[2] == ':')
		title += 4;
	setqtitle(title);
	return search_articles(locmem, title, offset, 2);
}
/*Add by SmallPig*/
int
sread(passonly, readfirst, pnum, auser, ptitle)
int     passonly, readfirst, auser, pnum;
struct fileheader *ptitle;
{
	struct keeploc *locmem;
	int     rem_top, rem_crs;	/* youzi 1997.7.7 */
	int     istest = 0, isstart = 0, isnext = 1;
	int     previous;
	char    genbuf[STRLEN], title[256];
	previous = pnum;
	if (passonly == 0) {
		if (readfirst)
			isstart = 1;
		else {
			isstart = 0;
			move(t_lines - 1, 0);
			clrtoeol();
			prints("[1;44;31m[%8s] [33m¤U¤@«Ê <Space>,<Enter>,¡õ¢x¤W¤@«Ê ¡ô,U                              [m", auser ? "¬Û¦P§@ªÌ" : "¥DÃD¾\\Åª");
			switch (egetch()) {
			case ' ':
			case '\n':
			case KEY_DOWN:
				isnext = 1;
				break;
			case KEY_UP:
			case 'u':
			case 'U':
				isnext = -1;
				break;
			default:
				break;
			}
		}
	} else if (passonly == 1 || passonly >= 3)
		isnext = 1;
	else
		isnext = -1;
	locmem = getkeep(currdirect, 1, 1);
	rem_top = locmem->top_line;
	rem_crs = locmem->crs_line;
	if (auser == 0) {
		strcpy(title, ptitle->title);
		setqtitle(title);
	} else if (auser == 2) {
		strcpy(title, keyword);
		setqtitle(title);
		auser = -2;	/* ¬°¤FÅýµ¥¤U search_article ¥i¥H
				 * lete search */
	} else {
		strcpy(title, ptitle->owner);
		setqtitle(ptitle->title);
	}
	if (!strncmp(title, "Re: ", 4) | !strncmp(title, "RE: ", 4)) {
		strcpy(title, title + 4);
	}
	memcpy(&SR_fptr, ptitle, sizeof(SR_fptr));
	while (!istest) {
		switch (passonly) {
		case 0:
		case 1:
		case 2:
			break;
		case 3:
			if (brc_unread(SR_fptr.filename))
				return -1;
			else
				break;
		case SR_BMDEL:
			if (digestmode)
				return;
			SR_BMDELFLAG = YEA;
			del_post(locmem->crs_line, &SR_fptr, currdirect);
			SR_BMDELFLAG = NA;
			if (sysconf_eval("KEEP_DELETED_HEADER") <= 0) {
				last_line--;
				locmem->crs_line--;
				previous = locmem->crs_line;
			}
			break;
		case SR_BMMARK:
			if (digestmode == 2)
				return;
			mark_post(locmem->crs_line, &SR_fptr, currdirect);
			break;
		case SR_BMDIGEST:
			if (digestmode == YEA)
				return;
			digest_post(locmem->crs_line, &SR_fptr, currdirect);
			break;
		case SR_BMIMPORT:
			a_Import("0Announce", currboard, &SR_fptr, YEA);
			break;
		case SR_BMTMP:
			a_Save("0Announce", currboard, &SR_fptr, YEA);
			break;
		}
		if (!isstart) {
			search_articles(locmem, title, isnext, auser + 2);
		}
		if (previous == locmem->crs_line) {
			break;
		}
		if (uinfo.mode != RMAIL)
			setbfile(genbuf, currboard, SR_fptr.filename);
		else
			sprintf(genbuf, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, SR_fptr.filename);
		previous = locmem->crs_line;
		setquotefile(genbuf);
		if (passonly == 0) {
			ansimore(genbuf, NA);
			brc_addlist(SR_fptr.filename);
			isstart = 0;
			move(t_lines - 1, 0);
			clrtoeol();
			prints("\033[1;44;31m[%8s] \033[33m¦^«H R ¢x µ²§ô Q,¡ö ¢x¤U¤@«Ê ¡õ,Enter¢x¤W¤@«Ê ¡ô,U ¢x ^R ¦^µ¹§@ªÌ   \033[m", auser ? "¬Û¦P§@ªÌ" : "¥DÃD¾\\Åª");
			switch (egetch()) {
			case 'N':
			case 'Q':
			case 'n':
			case 'q':
			case KEY_LEFT:
				istest = 1;
				break;
			case 'Y':
			case 'R':
			case 'y':
			case 'r':
				do_reply(SR_fptr.title);
				break;
			case ' ':
			case '\n':
			case KEY_DOWN:
				isnext = 1;
				break;
			case Ctrl('A'):
				clear();
				show_author(0, &SR_fptr, currdirect);
				isnext = 1;
				break;
			case KEY_UP:
			case 'u':
			case 'U':
				isnext = -1;
				break;
			case Ctrl('R'):
				post_reply(0, &SR_fptr, (char *) NULL);
				break;
			case 'g':
				digest_post(0, &SR_fptr, currdirect);
				break;
			default:
				break;
			}
		}
	}
	if (passonly == 0 && readfirst == 0) {	/* youzi 1997.7.9 */
		RMVCURS;
		locmem->top_line = rem_top;
		locmem->crs_line = rem_crs;
		PUTCURS;
	}
	return 1;
}

void
get_upper_str(ptr2, ptr1)
char   *ptr1, *ptr2;
{
	int     ln, i;
	ln = strlen(ptr1);
	for (i = 0; i < ln; i++) {
		ptr2[i] = toupper(ptr1[i]);
		if (ptr2[i] == '\0')
			ptr2[i] = '\1';
	}
	ptr2[ln] = '\0';
}

int
searchpattern(filename, query)
char   *filename;
char   *query;
{
	FILE   *fp;
	char    buf[256];
	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, 256, fp) != NULL) {
		if (strstr(buf, query)) {
			fclose(fp);
			return YEA;
		}
	}
	fclose(fp);
	return NA;
}

int
search_articles(locmem, query, offset, aflag)
struct keeploc *locmem;
char   *query;
int     offset, aflag;
{
	char   *ptr;
	int     now, match = 0;
	int     complete_search;
	int     ssize = sizeof(struct fileheader);


	if (aflag >= 2) {
		complete_search = 1;
		aflag -= 2;
	} else {
		complete_search = 0;
	}

	if (*query == '\0') {
		return 0;
	}
	move(t_lines - 1, 0);
	clrtoeol();
	prints("[1;44;33m·j´M¤¤¡A½Ðµy­Ô....                                                             [m");
	now = locmem->crs_line;
	refresh();
	while (1) {
		if (offset > 0) {
			if (++now > last_line)
				break;
		} else {
			if (--now < 1)
				break;
		}
		if (now == locmem->crs_line)
			break;
		get_record(currdirect, &SR_fptr, ssize, now);
		if (aflag == -1) {
			char    p_name[256];
			if (uinfo.mode != RMAIL)
				setbfile(p_name, currboard, SR_fptr.filename);
			else
				sprintf(p_name, "mail/%c/%s/%s", toupper(currentuser.userid[0]), currentuser.userid, SR_fptr.filename);
			if (searchpattern(p_name, query)) {
				match = cursor_pos(locmem, now, 10);
				break;
			} else
				continue;
		}
		ptr = aflag ? SR_fptr.owner : SR_fptr.title;
		if (complete_search == 1) {
			if ((*ptr == 'R' || *ptr == 'r') && (*(ptr + 1) == 'E' || *(ptr + 1) == 'e')
				&& (*(ptr + 2) == ':') && (*(ptr + 3) == ' ')) {
				ptr = ptr + 4;
			}
			if (!strcmp(ptr, query)) {
				match = cursor_pos(locmem, now, 10);
				break;
			}
		} else {
			char    upper_ptr[STRLEN], upper_query[STRLEN];
			get_upper_str(upper_ptr, ptr);
			get_upper_str(upper_query, query);
			if (strstr(upper_ptr, upper_query) != NULL) {
				match = cursor_pos(locmem, now, 10);
				break;
			}
		}
	}
	move(t_lines - 1, 0);
	clrtoeol();
	get_record(currdirect, &SR_fptr, sizeof(SR_fptr), locmem->crs_line);
	return match;
}
/* calc cursor pos and show cursor correctly -cuteyu */
int 
cursor_pos(locmem, val, from_top)
struct keeploc *locmem;
int     val;
int     from_top;
{
	if (val > last_line) {
		val = DEFINE(DEF_CIRCLE) ? 1 : last_line;
	}
	if (val <= 0) {
		val = DEFINE(DEF_CIRCLE) ? last_line : 1;
	}
	if (val >= locmem->top_line && val < locmem->top_line + screen_len - 1) {
		RMVCURS;
		locmem->crs_line = val;
		PUTCURS;
		return 0;
	}
	locmem->top_line = val - from_top;
	if (locmem->top_line <= 0)
		locmem->top_line = 1;
	locmem->crs_line = val;
	return 1;
}
