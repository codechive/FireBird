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
$Id: edit.c,v 1.5 2002/09/05 06:04:10 edwardc Exp $
*/

#include "bbs.h"
#include "edit.h"

struct textline *firstline = NULL;
struct textline *lastline = NULL;
struct shortfile *getbcache();


void    vedit_key();
struct textline *currline = NULL;
int     first_mark_line;
int     currpnt = 0;
extern int local_article;
extern char BoardName[];
char    searchtext[80];
int     editansi = 0;
int     scrollen = 2;
int     marknum;
int     moveln = 0;
int     shifttmp = 0;
int     ismsgline;
int     tmpline;
struct textline *top_of_win = NULL;
int     curr_window_line, currln;
int     redraw_everything;
int     insert_character = 1;
/* for copy/paste */
#define CLEAR_MARK()  mark_on = 0; mark_begin = mark_end = NULL;
struct textline *mark_begin, *mark_end;
int     mark_on;
/* copy/paste */

void
msgline()
{
	char    buf[256], buf2[STRLEN * 2];
	void    display_buffer();
	extern int talkrequest;
	int     tmpshow;
	time_t  now;
	if (ismsgline <= 0)
		return;
	now = time(0);
	tmpshow = showansi;
	showansi = 1;
	if (talkrequest) {
		talkreply();
		clear();
		showansi = 0;
		display_buffer();
		showansi = 1;
	}
	strcpy(buf, "[1;33;44m");
	if (chkmail())
		strcat(buf, "¡i[5;32m¡ó[m[1;33;44m¡j");
	else
		strcat(buf, "¡i  ¡j");

	strcat(buf, "¬O§_¦³·s«H¥ó [31mCtrl-Q[33m ¨D±Ï ");
	sprintf(buf2, " ª¬ºA [[32m%s[33m][[32m%4.4d[33m,[32m%3.3d[33m]   ®É¶¡", insert_character ? "Ins" : "Rep", currln + 1, currpnt + 1);
	strcat(buf, buf2);
	sprintf(buf2, "[1;33;44m¡i[1;32m%.16s[33m¡j[m", ctime(&now));
	strcat(buf, buf2);
	move(t_lines - 1, 0);
	clrtoeol();
	prints("%s", buf);
	showansi = tmpshow;
}

void
msg()
{
	int     x, y;
	int     tmpansi;
	tmpansi = showansi;
	showansi = 1;
	getyx(&x, &y);
	msgline();

	signal(SIGALRM, msg);
	move(x, y);
	refresh();
	alarm(60);
	showansi = tmpansi;
	return;
}

void
indigestion(i)
int     i;
{
	fprintf(stderr, "SERIOUS INTERNAL INDIGESTION CLASS %d\n", i);
}

struct textline *
back_line(pos, num)
struct textline *pos;
int     num;
{
	moveln = 0;
	while (num-- > 0)
		if (pos && pos->prev) {
			pos = pos->prev;
			moveln++;
		}
	return pos;
}

struct textline *
forward_line(pos, num)
struct textline *pos;
int     num;
{
	moveln = 0;
	while (num-- > 0)
		if (pos && pos->next) {
			pos = pos->next;
			moveln++;
		}
	return pos;
}

void
countline()
{
	struct textline *pos;
	pos = firstline;
	moveln = 0;
	while (pos != lastline)
		if (pos->next) {
			pos = pos->next;
			moveln++;
		}
}

int
getlineno()
{
	int     cnt = 0;
	struct textline *p = currline;
	while (p != top_of_win) {
		if (p == NULL)
			break;
		cnt++;
		p = p->prev;
	}
	return cnt;
}
char   *
killsp(s)
char   *s;
{
	while (*s == ' ')
		s++;
	return s;
}

struct textline *
alloc_line()
{
	register struct textline *p;
	p = (struct textline *) malloc(sizeof(*p));
	if (p == NULL) {
		indigestion(13);
		abort_bbs();
	}
	p->next = NULL;
	p->prev = NULL;
	p->data[0] = '\0';
	p->len = 0;
	p->attr = 0;		/* for copy/paste */
	return p;
}
/*
  Appends p after line in list.  keeps up with last line as well.
 */

void
goline(n)
int     n;
{
	register struct textline *p = firstline;
	int     count;
	if (n < 0)
		n = 1;
	if (n == 0)
		return;
	for (count = 1; count < n; count++) {
		if (p) {
			p = p->next;
			continue;
		} else
			break;
	}
	if (p) {
		currln = n - 1;
		curr_window_line = 0;
		top_of_win = p;
		currline = p;
	} else {
		top_of_win = lastline;
		currln = count - 2;
		curr_window_line = 0;
		currline = lastline;
	}
	if (Origin(currline)) {
		currline = currline->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln--;
	}
	if (Origin(currline->prev)) {
		currline = currline->prev->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln -= 2;
	}
}

void
go()
{
	char    tmp[8];
	int     line;
	signal(SIGALRM, SIG_IGN);
	getdata(23, 0, "½Ð°Ý­n¸õ¨ì²Ä´X¦æ: ", tmp, 7, DOECHO, YEA);
	msg();
	if (tmp[0] == '\0')
		return;
	line = atoi(tmp);
	goline(line);
	return;
}


void
searchline(text)
char    text[STRLEN];
{
	int     tmpline;
	int     addr;
	int     tt;

	register struct textline *p = currline;
	int     count = 0;
	tmpline = currln;
	for (;; p = p->next) {
		count++;
		if (p) {
			if (count == 1)
				tt = currpnt;
			else
				tt = 0;
			if (strstr(p->data + tt, text)) {
				addr = (int) (strstr(p->data + tt, text) - p->data) + strlen(text);
				currpnt = addr;
				break;
			}
		} else
			break;
	}
	if (p) {
		currln = currln + count - 1;
		curr_window_line = 0;
		top_of_win = p;
		currline = p;
	} else {
		goline(currln + 1);
	}
	if (Origin(currline)) {
		currline = currline->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln--;
	}
	if (Origin(currline->prev)) {
		currline = currline->prev->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln -= 2;
	}
}

void
search()
{
	char    tmp[STRLEN];
	signal(SIGALRM, SIG_IGN);
	getdata(23, 0, "·j´M¦r¦ê: ", tmp, 65, DOECHO, YEA);
	msg();
	if (tmp[0] == '\0')
		return;
	else
		strcpy(searchtext, tmp);

	searchline(searchtext);
	return;
}


void
append(p, line)
register struct textline *p, *line;
{
	p->next = line->next;
	if (line->next)
		line->next->prev = p;
	else
		lastline = p;
	line->next = p;
	p->prev = line;
}
/*
  delete_line deletes 'line' from the list and maintains the lastline, and
  firstline pointers.
 */

void
delete_line(line)
register struct textline *line;
{
	/* if single line */
	if (!line->next && !line->prev) {
		line->data[0] = '\0';
		line->len = 0;
		CLEAR_MARK();
		return;
	}
#define ADJUST_MARK(p, q) if(p == q) p = (q->next) ? q->next : q->prev

	ADJUST_MARK(mark_begin, line);
	ADJUST_MARK(mark_end, line);

	if (line->next)
		line->next->prev = line->prev;
	else
		lastline = line->prev;	/* if on last line */

	if (line->prev)
		line->prev->next = line->next;
	else
		firstline = line->next;	/* if on first line */

	if (line)
		free(line);
}
/*
  split splits 'line' right before the character pos
 */

void
split(line, pos)
register struct textline *line;
register int pos;
{
	register struct textline *p = alloc_line();
	
	countline();
	if (pos > line->len || moveln > MAX_EDITROW ) {
		free(p);
		return;
	}
	p->len = line->len - pos;
	line->len = pos;
	strcpy(p->data, (line->data + pos));
	p->attr = line->attr;	/* for copy/paste */
	*(line->data + pos) = '\0';
	append(p, line);
	if (line == currline && pos <= currpnt) {
		currline = p;
		currpnt -= pos;
		curr_window_line++;
		currln++;
	}
	redraw_everything = YEA;
}
/*
  join connects 'line' and the next line.  It returns true if:

  1) lines were joined and one was deleted
  2) lines could not be joined
  3) next line is empty

  returns false if:

  1) Some of the joined line wrapped
 */

int
join(line)
register struct textline *line;
{
	register int ovfl;
	if (!line->next)
		return YEA;
	/*
	 * if(*killsp(line->next->data) == '\0') return YEA ;
	 */
	ovfl = line->len + line->next->len - WRAPMARGIN;
	if (ovfl < 0) {
		strcat(line->data, line->next->data);
		line->len += line->next->len;
		delete_line(line->next);
		return YEA;
	} else {
		register char *s;
		register struct textline *p = line->next;
		s = p->data + p->len - ovfl - 1;
		while (s != p->data && *s == ' ')
			s--;
		while (s != p->data && *s != ' ')
			s--;
		if (s == p->data)
			return YEA;
		split(p, (s - p->data) + 1);
		if (line->len + p->len >= WRAPMARGIN) {
			indigestion(0);
			return YEA;
		}
		join(line);
		p = line->next;
		if (p->len >= 1 && p->len + 1 < WRAPMARGIN) {
			if (p->data[p->len - 1] != ' ') {
				strcat(p->data, " ");
				p->len++;
			}
		}
		return NA;
	}
}

void
insert_char(ch)
register int ch;
{
	register int i;
	register char *s;
	register struct textline *p = currline;
	int     wordwrap = YEA;
	if (currpnt > p->len) {
		indigestion(1);
		return;
	}
	if (currpnt < p->len && !insert_character) {
		p->data[currpnt++] = ch;
	} else {
		for (i = p->len; i >= currpnt; i--)
			p->data[i + 1] = p->data[i];
		p->data[currpnt] = ch;
		p->len++;
		currpnt++;
	}
	if (p->len < WRAPMARGIN)
		return;
	s = p->data + (p->len - 1);
	while (s != p->data && *s == ' ')
		s--;
	while (s != p->data && *s != ' ')
		s--;
	if (s == p->data) {
		wordwrap = NA;
		s = p->data + (p->len - 2);
	}
	split(p, (s - p->data) + 1);
	p = p->next;
	if (wordwrap && p->len >= 1) {
		i = p->len;
		if (p->data[i - 1] != ' ') {
			p->data[i] = ' ';
			p->data[i + 1] = '\0';
			p->len++;
		} {
		}
	}
	while (!join(p)) {
		p = p->next;
		if (p == NULL) {
			indigestion(2);
			break;
		}
	}
	if (Origin(currline)) {
		currline = p->prev;
		curr_window_line--;
		currln--;
	}
}

void
ve_insert_str(str)
char   *str;
{
	while (*str)
		insert_char(*(str++));
}

void
delete_char()
{
	register int i;
	if (currline->len == 0)
		return;
	if (currpnt >= currline->len) {
		indigestion(1);
		return;
	}
	for (i = currpnt; i != currline->len; i++)
		currline->data[i] = currline->data[i + 1];
	currline->len--;
}

void
vedit_init()
{
	register struct textline *p = alloc_line();
	first_mark_line = 0;
	firstline = p;
	lastline = p;
	currline = p;
	currpnt = 0;
	marknum = 0;
	process_ESC_action('M', '0');
	top_of_win = p;
	curr_window_line = 0;
	currln = 0;
	redraw_everything = NA;
	CLEAR_MARK();
}

void
insert_to_fp(fp)
FILE   *fp;
{
	int     ansi = 0;
	struct textline *p;
	for (p = firstline; p; p = p->next)
		if (p->data[0]) {
			fprintf(fp, "%s\n", p->data);
			if (strchr(p->data, '\033'))
				ansi++;
		}
	if (ansi)
		fprintf(fp, "%s\n", ANSI_RESET);
}

void
insert_from_fp(fp)
FILE   *fp;
{
	int     ch;
	while ((ch = getc(fp)) != EOF)
		if (isprint2(ch) || ch == 27) {
			if (currpnt < 254)
				insert_char(ch);
			else if (currpnt < 255)
				insert_char('.');
		} else if (ch == Ctrl('I')) {
			do {
				insert_char(' ');
			} while (currpnt & 0x7);
		} else if (ch == '\n')
			split(currline, currpnt);
}

void
read_file(filename)
char   *filename;
{
	FILE   *fp;
	if (currline == NULL)
		vedit_init();
	if ((fp = fopen(filename, "r+")) == NULL) {
		if ((fp = fopen(filename, "w+")) != NULL) {
			fclose(fp);
			return;
		}
		indigestion(4);
		abort_bbs();
	}
	insert_from_fp(fp);
	fclose(fp);
}

char    save_title[STRLEN];
char    save_filename[4096];
int     in_mail;

int
write_posts()
{
	char   *ptr;
	time_t  now;
	struct {
		char    author[IDLEN + 1];
		char    board[18];
		char    title[66];
		time_t  date;
		int     number;
	}       postlog;
	if (junkboard() || normal_board(currboard) != 1)
		return;
	now = time(0);
	strcpy(postlog.author, currentuser.userid);
	strcpy(postlog.board, currboard);
	ptr = save_title;
	if (!strncmp(ptr, "Re: ", 4))
		ptr += 4;
	strncpy(postlog.title, ptr, 64);
	postlog.title[65] = '\0';
	postlog.date = now;
	postlog.number = 1;
	append_record(".post", &postlog, sizeof(postlog));
}


void
write_header(fp, mode)
FILE   *fp;
int     mode;
{
	int     noname;
	extern char BoardName[];
	extern char fromhost[];
	extern struct postheader header;
	struct shortfile *bp;
	char    uid[20];
	char    uname[40];
	time_t  now;
	
	now = time(0);
	strncpy(uid, currentuser.userid, 20);
	uid[19] = '\0';
	if (in_mail)
#if defined(MAIL_REALNAMES)
		strncpy(uname, currentuser.realname, NAMELEN);
#else
		strncpy(uname, currentuser.username, NAMELEN);
#endif
	else
#if defined(POSTS_REALNAMES)
		strncpy(uname, currentuser.realname, NAMELEN);
#else
		strncpy(uname, currentuser.username, NAMELEN);
#endif

	uid[19] = '\0';
	save_title[STRLEN - 10] = '\0';
	bp = getbcache(currboard);
	noname = bp->flag & ANONY_FLAG;
	
	if (in_mail)
		fprintf(fp, "±H«H¤H: %s (%s)\n", uid, uname);
	else {
		if (mode == 0 && !(noname && header.chk_anony)) {
			write_posts();
		}
		fprintf(fp, "µo«H¤H: %s (%s), «H°Ï: %s\n", (noname && header.chk_anony) ? currboard : uid,
			(noname && header.chk_anony) ? "§Ú¬O°Î¦W¤Ñ¨Ï" : uname, currboard);
	}
	fprintf(fp, "¼Ð  ÃD: %s\n", save_title);
	fprintf(fp, "µo«H¯¸: %s (%24.24s)", BoardName, ctime(&now));
	if (in_mail)
		fprintf(fp, "\n¨Ó  ·½: %s \n", fromhost);
	else
		fprintf(fp, ", %s\n", (local_article) ? "¯¸¤º«H¥ó" : "Âà«H");
	fprintf(fp, "\n");
}

void
addsignature(fp, blank)
FILE   *fp;
int     blank;
{
	FILE   *sigfile;
	int     i, valid_ln = 0;
	char    tmpsig[MAXSIGLINES][256];
	char    inbuf[256];
	char    fname[STRLEN];
	setuserfile(fname, "signatures");
	if ((sigfile = fopen(fname, "r")) == NULL) {
		return;
	}
	if (blank)
		fputs("\n", fp);
	fputs("--\n", fp);
	for (i = 1; i <= (currentuser.signature - 1) * MAXSIGLINES && currentuser.signature != 1; i++) {
		if (!fgets(inbuf, sizeof(inbuf), sigfile)) {
			fclose(sigfile);
			return;
		}
	}
	for (i = 1; i <= MAXSIGLINES; i++) {
		if (fgets(inbuf, sizeof(inbuf), sigfile)) {
			if (inbuf[0] != '\n')
				valid_ln = i;
			strcpy(tmpsig[i - 1], inbuf);
		} else
			break;
	}
	fclose(sigfile);
	for (i = 1; i <= valid_ln; i++)
		fputs(tmpsig[i - 1], fp);
}
#define KEEP_EDITING -2

void
valid_article(pmt, abort)
char   *pmt, *abort;
{
	struct textline *p = firstline;
	char    ch;
	int     total, lines, len, sig, y;
	int     w;
	w = NA;

	if (uinfo.mode == POSTING) {
		total = lines = len = sig = 0;
		while (p != NULL) {
			if (!sig) {
				ch = p->data[0];
				if (strcmp(p->data, "--") == 0)
					sig = 1;
				else if (ch != ':' && ch != '>' && ch != '=' &&
					!strstr(p->data, "ªº¤j§@¤¤´£¨ì: ¡j")) {
					lines++;
					len += strlen(p->data);
				}
			}
			total++;
			p = p->next;
		}
		y = 2;
		if (lines < total * 0.35 - MAXSIGLINES) {
			move(y, 0);
			prints("ª`·N¡G¥»½g¤å³¹ªº¤Þ¨¥¹Lªø, «ØÄ³±z§R±¼¤@¨Ç¤£¥²­nªº¤Þ¨¥.\n");
			y += 3;
			w = YEA;
		}
		if (len < 40 || lines < 1) {
			move(y, 0);
			prints("ª`·N¡G¥»½g¤å³¹¹L©óÂ²µu, ¨t²Î»{¬°¬OÄé¤ô¤å³¹.\n");
			y += 3;
			w = YEA;
		}
		if (w) {
			strcpy(pmt, "(E)¦A½s¿è, (S)Âà«H, (L)¤£Âà«H, (A)¨ú®ø or (T)§ó§ï¼ÐÃD? [E]: ");
		}
	}
	getdata(0, 0, pmt, abort, 3, DOECHO, YEA);
	if (w && abort[0] == '\0')
		abort[0] = 'E';
	switch (abort[0]) {
	case 'A':
	case 'a':		/* abort */
	case 'E':
	case 'e':		/* keep editing */
		return;
	}

}

int
write_file(filename, saveheader)
char   *filename;
int     saveheader;
{
	struct textline *p = firstline;
	FILE   *fp;
	char    abort[6];
	int     aborted = 0;

	char    p_buf[100];
	signal(SIGALRM, SIG_IGN);
	clear();

	if (uinfo.mode != CCUGOPHER) {
		if (uinfo.mode == POSTING) {
			if (local_article == 1)
				strcpy(p_buf, "(L)¤£Âà«H, (S)Âà«H, (A)¨ú®ø, (T)§ó§ï¼ÐÃD or (E)¦A½s¿è? [L]: ");
			else
				strcpy(p_buf, "(S)Âà«H, (L)¤£Âà«H, (A)¨ú®ø, (T)§ó§ï¼ÐÃD or (E)¦A½s¿è? [S]: ");
		} else if (uinfo.mode == SMAIL)
			strcpy(p_buf, "(S)±H¥X, (A)¨ú®ø, or (E)¦A½s¿è? [S]: ");
		else
			strcpy(p_buf, "(S)Àx¦sÀÉ®×, (A)©ñ±ó½s¿è, (E)Ä~Äò½s¿è? [S]: ");
		valid_article(p_buf, abort);
	} else
		abort[0] = 'a';
	if (abort[0] == 'a' || abort[0] == 'A') {
		struct stat stbuf;
		clear();
		if (uinfo.mode != CCUGOPHER) {
			prints("¨ú®ø...\n");
			refresh();
			sleep(1);
		}
		if (stat(filename, &stbuf) || stbuf.st_size == 0)
			unlink(filename);
		aborted = -1;
	} else if (abort[0] == 'e' || abort[0] == 'E') {
		msg();
		return KEEP_EDITING;
	} else if (abort[0] == 't' || abort[0] == 'T') {
		char    buf[STRLEN];
		move(1, 0);
		prints("ÂÂ¼ÐÃD: %s", save_title);
		sprintf(buf, "%s", save_title);
		getdata(2, 0, "·s¼ÐÃD: ", buf, 50, DOECHO, NA);
		if (strcmp(save_title, buf) && strlen(buf) != 0) {
			local_article = 0;
			strncpy(save_title, buf, STRLEN);
		}
	} else if (abort[0] == 's' || abort[0] == 'S') {
		local_article = 0;
	} else if (abort[0] == 'l' || abort[0] == 'L')
		local_article = 1;
	firstline = NULL;
	if (!aborted) {
		if ((fp = fopen(filename, "w")) == NULL) {
			indigestion(5);
			abort_bbs();
		}
		if (saveheader)
			write_header(fp, 0);
	}
	while (p != NULL) {
		struct textline *v = p->next;
		if (!aborted)
			if (p->next != NULL || p->data[0] != '\0')
				fprintf(fp, "%s\n", p->data);
		free(p);
		p = v;
	}
	if (!aborted)
		fclose(fp);
	currline = NULL;
	lastline = NULL;
	firstline = NULL;
	if (abort[0] == 'l' || abort[0] == 'L' || local_article == 1) {
		sprintf(genbuf, "local_article = %u", local_article);
		report(genbuf);
		local_article = 0;
		if (aborted != -1)
			aborted = 1;
	}
	return aborted;
}
keep_fail_post()
{
	char    filename[STRLEN];
	struct textline *p = firstline;
	FILE   *fp;
	sprintf(filename, "home/%c/%s/%s.deadve", toupper(currentuser.userid[0]), currentuser.userid, currentuser.userid);
	if ((fp = fopen(filename, "w")) == NULL) {
		indigestion(5);
		return;
	}
	while (p != NULL) {
		struct textline *v = p->next;
		if (p->next != NULL || p->data[0] != '\0')
			fprintf(fp, "%s\n", p->data);
		free(p);
		p = v;
	}
	return;
}


void 
strnput(str)
char   *str;
{
	int     count = 0;
	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			str++;
			continue;
		}
		outc(*str++);
	}
}

void
cstrnput(str)
char   *str;
{
	int     count = 0;
	prints("%s", ANSI_REVERSE);
	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			str++;
			continue;
		}
		outc(*str++);
	}
	while (++count < STRLEN)
		outc(' ');
	clrtoeol();
	prints("%s", ANSI_RESET);
}


/*Function Add by SmallPig*/
int
Origin(text)
struct textline *text;
{
	char    tmp[STRLEN];
	if (uinfo.mode != EDIT)
		return 0;
	if (!text)
		return 0;
	sprintf(tmp, ":¡E%s %s¡E[FROM:", BoardName, BBSHOST);
	if (strstr(text->data, tmp) && *text->data != ':')
		return 1;
	else
		return 0;
}

int
Origin2(text)
char    text[256];
{
	char    tmp[STRLEN];
	sprintf(tmp, ":¡E%s %s¡E[FROM:", BoardName, BBSHOST);
	if (strstr(text, tmp))
		return 1;
	else
		return 0;
}

void
display_buffer()
{
	register struct textline *p;
	register int i;
	int     shift;
	int     temp_showansi;
	temp_showansi = showansi;

	for (p = top_of_win, i = 0; i < t_lines - 1; i++) {
		move(i, 0);
		if (p) {
			shift = (currpnt + 2 > STRLEN) ?
				(currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
			if (editansi) {
				showansi = 1;
				prints("%s", p->data);
			} else if ((p->attr & M_MARK)) {
				showansi = 1;
				clear_whole_line(i);
				cstrnput(p->data + shift);
			} else {
				if (p->len >= shift) {
					showansi = 0;
					strnput(p->data + shift);
				} else
					clrtoeol();
			}
			p = p->next;
		} else
			prints("%s~", editansi ? ANSI_RESET : "");
		clrtoeol();
	}

	showansi = temp_showansi;
	msgline();
	return;
}

int
vedit_process_ESC(arg)
int     arg;			/* ESC + x */
{
	int     ch2, action;
#define WHICH_ACTION_COLOR    \
"(M)°Ï¶ô³B²z (I/E)Åª¨ú/¼g¤J°Å¶KÃ¯ (C)¨Ï¥Î±m¦â (F/B/R)«e´º/­I´º/ÁÙ­ì¦â±m"
#define WHICH_ACTION_MONO    \
"(M)°Ï¶ô³B²z (I/E)Åª¨ú/¼g¤J°Å¶KÃ¯ (C)¨Ï¥Î³æ¦â (F/B/R)«e´º/­I´º/ÁÙ­ì¦â±m"

#define CHOOSE_MARK     "(0)¨ú®ø¼Ð°O (1)³]©w¶}ÀY (2)³]©wµ²§À (3)½Æ»s¼Ð°O¤º®e "
#define FROM_WHICH_PAGE "Åª¨ú°Å¶KÃ¯²Ä´X­¶? (0-7) [¹w³]¬° 0]"
#define SAVE_ALL_TO     "§â¾ã½g¤å³¹¼g¤J°Å¶KÃ¯²Ä´X­¶? (0-7) [¹w³]¬° 0]"
#define SAVE_PART_TO    "§â¾ã½g¤å³¹¼g¤J°Å¶KÃ¯²Ä´X­¶? (0-7) [¹w³]¬° 0]"
#define FROM_WHICH_SIG  "¨ú¥XÃ±¦WÃ¯²Ä´X­¶? (0-7) [¹w³]¬° 0]"
#define CHOOSE_FG       "«e´ºÃC¦â? 0)¶Â 1)¬õ 2)ºñ 3)¶À 4)²`ÂÅ 5)¯»¬õ 6)²LÂÅ 7)¥Õ "
#define CHOOSE_BG       "­I´ºÃC¦â? 0)¶Â 1)¬õ 2)ºñ 3)¶À 4)²`ÂÅ 5)¯»¬õ 6)²LÂÅ 7)¥Õ "
#define CHOOSE_BIG5     "¤º½Xªí? 0)¼ÐÂI 1)²Å¸¹ 2)ªí®æ 3)ª`­µ 4)¥­°² 5)¤ù°² 6)§ÆÃ¾ 7)¼Æ¦r"
#define CHOOSE_ERROR    "¿ï¶µ¿ù»~"

	switch (arg) {
	case 'M':
	case 'm':
		ch2 = ask(CHOOSE_MARK);
		action = 'M';
		break;
	case 'I':
	case 'i':		/* import */
		ch2 = ask(FROM_WHICH_PAGE);
		action = 'I';
		break;
	case 'E':
	case 'e':		/* export */
		ch2 = ask(mark_on ? SAVE_PART_TO : SAVE_ALL_TO);
		action = 'E';
		break;
	case 'S':
	case 's':		/* signature */
		ch2 = '0';
		action = 'S';
		break;
	case 'F':
	case 'f':
		ch2 = ask(CHOOSE_FG);
		action = 'F';
		break;
	case 'B':
	case 'b':
		ch2 = ask(CHOOSE_BG);
		action = 'B';
		break;
	case 'R':
	case 'r':
		ch2 = '0';	/* not used */
		action = 'R';
		break;
	case 'D':
	case 'd':
		ch2 = '4';
		action = 'M';
		break;
	case 'N':
	case 'n':
		ch2 = '0';
		action = 'N';
		break;
	case 'G':
	case 'g':
		ch2 = '1';
		action = 'G';
		break;
	case 'L':
	case 'l':
		ch2 = '0';	/* not used */
		action = 'L';
		break;
	case 'C':
	case 'c':
		ch2 = '0';	/* not used */
		action = 'C';
		break;
	case 'O':
	case 'o':
		ch2 = ask(CHOOSE_BIG5);
		action = 'O';
		break;
	case '=':
		ch2 = '0';	/* not used */
		action = '=';
		break;
	case 'Q':
	case 'q':
		ch2 = '0';	/* not used */
		action = 'M';
		break;
		marknum = 0;
		break;
	default:
		return;
	}

	if (strchr("IES", action) &&
		(ch2 == '\n' || ch2 == '\r'))
		ch2 = '0';

	if (ch2 >= '0' && ch2 <= '7')
		return process_ESC_action(action, ch2);
	else {
		return ask(CHOOSE_ERROR);
	}
}

int
mark_block()
{
	struct textline *p;
	int     pass_mark = 0;
	first_mark_line = 0;
	if (mark_begin == NULL && mark_end == NULL)
		return 0;
	if (mark_begin == mark_end) {
		mark_begin->attr |= M_MARK;
		return 1;
	}
	if (mark_begin == NULL || mark_end == NULL) {
		if (mark_begin != NULL)
			mark_begin->attr |= M_MARK;
		else
			mark_end->attr |= M_MARK;
		return 1;
	} else {
		for (p = firstline; p != NULL; p = p->next) {
			if (p == mark_begin || p == mark_end) {
				pass_mark++;
				p->attr |= M_MARK;
				continue;
			}
			if (pass_mark == 1)
				p->attr |= M_MARK;
			else {
				first_mark_line++;
				p->attr &= ~(M_MARK);
			}
			if (pass_mark == 2)
				first_mark_line--;
		}
		return 1;
	}
}

void
process_MARK_action(arg, msg)
int     arg;			/* operation of MARK */
char   *msg;			/* message to return */
{
	struct textline *p;
	int     dele_1line;
	switch (arg) {
	case '0':		/* cancel */
		for (p = firstline; p != NULL; p = p->next)
			p->attr &= ~(M_MARK);
		CLEAR_MARK();
		break;
	case '1':		/* mark begin */
		mark_begin = currline;
		mark_on = mark_block();
		if (mark_on)
			strcpy(msg, "¼Ð°O¤w³]©w§¹¦¨");
		else
			strcpy(msg, "¤w³]©w¶}ÀY¼Ð°O, ©|µLµ²§À¼Ð°O");
		break;
	case '2':		/* mark end */
		mark_end = currline;
		mark_on = mark_block();
		if (mark_on)
			strcpy(msg, "¼Ð°O¤w³]©w§¹¦¨");
		else
			strcpy(msg, "¤w³]©wµ²§À¼Ð°O, ©|µL¶}ÀY¼Ð°O");
		break;
	case '3':		/* copy mark */
		if (mark_on && !(currline->attr & M_MARK)) {
			for (p = firstline; p != NULL; p = p->next) {
				if (p->attr & M_MARK) {
					ve_insert_str(p->data);
					split(currline, currpnt);
				}
			}
		} else
			bell();
		strcpy(msg, "¼Ð°O½Æ»s§¹¦¨");
		break;
	case '4':		/* delete mark */
		dele_1line = 0;
		if (mark_on && (currline->attr & M_MARK))
			if (currline == firstline)
				dele_1line = 1;
			else
				dele_1line = 2;
		for (p = firstline; p != NULL; p = p->next) {
			if (p->attr & M_MARK) {
				currline = p;
				vedit_key(Ctrl('Y'));
				if (p == firstline)
					p = currline;
				else
					p->prev = currline;
			}
		}
		process_ESC_action('M', '0');
		marknum = 0;
		if (dele_1line == 0 || dele_1line == 2) {
			if (first_mark_line == 0)
				first_mark_line = 1;
			goline(first_mark_line);
		} else
			goline(1);
		break;
	default:
		strcpy(msg, CHOOSE_ERROR);
	}
	strcpy(msg, "\0");
}

int
process_ESC_action(action, arg)
int     action, arg;
/* valid action are I/E/S/B/F/R/C/O/= */
/* valid arg are    '0' - '7' */
{
	int     newch = 0;
	int     savemode;
	char    msg[80], buf[80];
	char    filename[80];
	char    ch;
	FILE   *fp;
	msg[0] = '\0';
	switch (action) {
	case 'L':
		if (ismsgline >= 1) {
			ismsgline = 0;
			move(t_lines - 1, 0);
			clrtoeol();
			refresh();
		} else
			ismsgline = 1;
		break;
	case 'M':
		process_MARK_action(arg, msg);
		break;
	case 'I':
		sprintf(filename, "home/%c/%s/clip_%c",
			toupper(currentuser.userid[0]), currentuser.userid, arg);
		if ((fp = fopen(filename, "r")) != NULL) {
			insert_from_fp(fp);
			fclose(fp);
			sprintf(msg, "¤w¨ú¥X°Å¶KÃ¯²Ä %c ­¶", arg);
		} else
			sprintf(msg, "µLªk¨ú¥X°Å¶KÃ¯²Ä %c ­¶", arg);
		break;
	case 'G':
		go();
		redraw_everything = YEA;
		break;
	case 'E':
		sprintf(filename, "home/%c/%s/clip_%c",
			toupper(currentuser.userid[0]), currentuser.userid, arg);
		if ((fp = fopen(filename, "w")) != NULL) {
			if (mark_on) {
				struct textline *p;
				for (p = firstline; p != NULL; p = p->next)
					if (p->attr & M_MARK)
						fprintf(fp, "%s\n", p->data);
			} else
				insert_to_fp(fp);
			fclose(fp);
			sprintf(msg, "¤w¶K¦Ü°Å¶KÃ¯²Ä %c ­¶", arg);
		} else
			sprintf(msg, "µLªk¶K¦Ü°Å¶KÃ¯²Ä %c ­¶", arg);
		break;
	case 'N':
		searchline(searchtext);
		redraw_everything = YEA;
		break;
	case 'S':
		search();
		redraw_everything = YEA;
		break;
	case 'F':
		sprintf(buf, "%c[3%cm", 27, arg);
		ve_insert_str(buf);
		break;
	case 'B':
		sprintf(buf, "%c[4%cm", 27, arg);
		ve_insert_str(buf);
		break;
	case 'R':
		ve_insert_str(ANSI_RESET);
		break;
	case 'C':
		editansi = showansi = 1;
		redraw_everything = YEA;
		clear();
		display_buffer();
		redoscr();
		strcpy(msg, "¤wÅã¥Ü±m¦â½s¿è¦¨ªG¡A§Y±N¤Á¦^³æ¦â¼Ò¦¡");
		break;
	case 'O':
		clear();
		sprintf(filename, "table/big5.%c", arg);
		if ((fp = fopen(filename, "r")) != NULL) {
			while ((ch = getc(fp)) != EOF)
				prints("%c", ch);
			fclose(fp);
			getdata(22, 0, "½Ð¥Î¤º½X¿é¤Jªk¿é¤J¡G", buf, 20, DOECHO, YEA);
			ve_insert_str(buf);
		}
		display_buffer();
		redoscr();
		break;
	case '=':
		if (!HAS_PERM(PERM_ADMINMENU))
			break;
		savemode = uinfo.mode;
		showansi = 1;
		x_dict();
		modify_user_mode(savemode);
		display_buffer();
		redoscr();
		break;
	}

	if (strchr("FBRCM", action))
		redraw_everything = YEA;

	if (msg[0] != '\0') {
		if (action == 'C') {	/* need redraw */
			move(t_lines - 2, 0);
			clrtoeol();
			prints("[1m%s%s%s[m", msg, ", ½Ð«ö¥ô·NÁäªð¦^½s¿èµe­±...", ANSI_RESET);
			igetkey();
			newch = '\0';
			editansi = showansi = 0;
			clear();
			display_buffer();
		} else
			newch = ask(strcat(msg, "¡A½ÐÄ~Äò½s¿è¡C"));
	} else
		newch = '\0';
	return newch;
}

void
vedit_key(ch)
int     ch;
{
	int     i;
#define NO_ANSI_MODIFY  if(no_touch) { warn++; break; }

	static int lastindent = -1;
	int     no_touch, warn, shift;
	if (ch == Ctrl('P') || ch == KEY_UP ||
		ch == Ctrl('N') || ch == KEY_DOWN) {
		if (lastindent == -1)
			lastindent = currpnt;
	} else
		lastindent = -1;

	no_touch = (editansi && strchr(currline->data, '\033')) ? 1 : 0;
	warn = 0;


	if (ch < 0x100 && isprint2(ch)) {
		if (no_touch)
			warn++;
		else
			insert_char(ch);
	} else
		switch (ch) {
		case Ctrl('I'):
			NO_ANSI_MODIFY;
			do {
				insert_char(' ');
			} while (currpnt & 0x7);
			break;
		case '\r':
		case '\n':
			NO_ANSI_MODIFY;
			split(currline, currpnt);
			break;
		case Ctrl('G'):/* redraw screen */
			clear();
			redraw_everything = YEA;
			break;
		case Ctrl('Q'):/* call help screen */
			show_help(F_HELP_EDIT);
			redraw_everything = YEA;
			break;
		case KEY_LEFT:	/* backward character */
			if (currpnt > 0) {
				currpnt--;
			} else if (currline->prev) {
				curr_window_line--;
				currln--;
				currline = currline->prev;
				currpnt = currline->len;
			}
			break;
		case Ctrl('C'):
			process_ESC_action('M', '3');
			break;
		case Ctrl('U'):
			if (marknum == 0) {
				marknum = 1;
				process_ESC_action('M', '1');
			} else
				process_ESC_action('M', '2');
			clear();
			break;
		case Ctrl('V'):
		case KEY_RIGHT:/* forward character */
			if (currline->len != currpnt) {
				currpnt++;
			} else if (currline->next) {
				currpnt = 0;
				curr_window_line++;
				currln++;
				currline = currline->next;
				if (Origin(currline)) {
					curr_window_line--;
					currln--;
					currline = currline->prev;
				}
			}
			break;
		case Ctrl('P'):
		case KEY_UP:	/* Previous line */
			if (currline->prev) {
				currln--;
				curr_window_line--;
				currline = currline->prev;
				currpnt = (currline->len > lastindent) ? lastindent : currline->len;
			}
			break;
		case Ctrl('N'):
		case KEY_DOWN:	/* Next line */
			if (currline->next) {
				currline = currline->next;
				curr_window_line++;
				currln++;
				if (Origin(currline)) {
					currln--;
					curr_window_line--;
					currline = currline->prev;
				}
				currpnt = (currline->len > lastindent) ? lastindent : currline->len;
			}
			break;
		case Ctrl('B'):
		case KEY_PGUP:	/* previous page */
			top_of_win = back_line(top_of_win, 22);
			currline = back_line(currline, 22);
			currln -= moveln;
			curr_window_line = getlineno();
			if (currpnt > currline->len)
				currpnt = currline->len;
			redraw_everything = YEA;
			break;
		case Ctrl('F'):
		case KEY_PGDN:	/* next page */
			top_of_win = forward_line(top_of_win, 22);
			currline = forward_line(currline, 22);
			currln += moveln;
			curr_window_line = getlineno();
			if (currpnt > currline->len)
				currpnt = currline->len;
			if (Origin(currline->prev)) {
				currln -= 2;
				curr_window_line = 0;
				currline = currline->prev->prev;
				top_of_win = lastline->prev->prev;
			}
			if (Origin(currline)) {
				currln--;
				curr_window_line--;
				currline = currline->prev;
			}
			redraw_everything = YEA;
			break;
		case Ctrl('A'):
		case KEY_HOME:	/* begin of line */
			currpnt = 0;
			break;
		case Ctrl('E'):
		case KEY_END:	/* end of line */
			currpnt = currline->len;
			break;
		case Ctrl('S'):/* start of file */
			top_of_win = firstline;
			currline = top_of_win;
			currpnt = 0;
			curr_window_line = 0;
			currln = 0;
			redraw_everything = YEA;
			break;
		case Ctrl('T'):/* tail of file */
			top_of_win = back_line(lastline, 22);
			countline();
			currln = moveln;
			currline = lastline;
			curr_window_line = getlineno();
			currpnt = 0;
			if (Origin(currline->prev)) {
				currline = currline->prev->prev;
				currln -= 2;
				curr_window_line -= 2;
			}
			redraw_everything = YEA;
			break;
		case Ctrl('O'):
		case KEY_INS:	/* Toggle insert/overwrite */
			insert_character = !insert_character;
			/*
			 * move(0,73); prints( " [%s] ", insert_character ?
			 * "Ins" : "Rep" );
			 */
			break;
		case Ctrl('H'):
		case '\177':	/* backspace */
			NO_ANSI_MODIFY;
			if (currpnt == 0) {
				struct textline *p;
				if (!currline->prev) {
					break;
				}
				currln--;
				curr_window_line--;
				currline = currline->prev;
				currpnt = currline->len;
				if (*killsp(currline->next->data) == '\0') {
					delete_line(currline->next);
					redraw_everything = YEA;
					break;
				}
				p = currline;
				while (!join(p)) {
					p = p->next;
					if (p == NULL) {
						indigestion(2);
						abort_bbs();
					}
				}
				redraw_everything = YEA;
				break;
			}
			currpnt--;
			delete_char();
			break;
		case Ctrl('D'):
		case KEY_DEL:	/* delete current character */
			NO_ANSI_MODIFY;
			if (currline->len == currpnt) {
				struct textline *p = currline;
				if (!Origin(currline->next)) {
					while (!join(p)) {
						p = p->next;
						if (p == NULL) {
							indigestion(2);
							abort_bbs();
						}
					}
				} else if (currpnt == 0)
					vedit_key(Ctrl('K'));
				redraw_everything = YEA;
				break;
			}
			delete_char();
			break;
		case Ctrl('Y'):/* delete current line */
			/* STONGLY coupled with Ctrl-K */
			no_touch = 0;	/* ANSI_MODIFY hack */
			currpnt = 0;
			if (currline->next) {
				if (Origin(currline->next) && !currline->prev) {
					currline->data[0] = '\0';
					currline->len = 0;
					break;
				}
			} else if (currline->prev != NULL) {
				currline->len = 0;
			} else {
				currline->len = 0;
				currline->data[0] = '\0';
				break;
			}
			currline->len = 0;
			vedit_key(Ctrl('K'));
			break;
		case Ctrl('K'):/* delete to end of line */
			NO_ANSI_MODIFY;
			if (currline->prev == NULL && currline->next == NULL) {
				currline->data[0] = '\0';
				currpnt = 0;
				break;
			}
			if (currline->next) {
				if (Origin(currline->next) && currpnt == currline->len && currpnt != 0)
					break;
				if (Origin(currline->next) && currline->prev == NULL) {
					vedit_key(Ctrl('Y'));
					break;
				}
			}
			if (currline->len == 0) {
				struct textline *p = currline->next;
				if (!p) {
					p = currline->prev;
					if (!p) {
						break;
					}
					if (curr_window_line > 0)
						curr_window_line--;
					currln--;
				}
				if (currline == top_of_win)
					top_of_win = p;
				delete_line(currline);
				currline = p;
				if (Origin(currline)) {
					currline = currline->prev;
					curr_window_line--;
					currln--;
				}
				redraw_everything = YEA;
				break;
			}
			if (currline->len == currpnt) {
				struct textline *p = currline;
				while (!join(p)) {
					p = p->next;
					if (p == NULL) {
						indigestion(2);
						abort_bbs();
					}
				}
				redraw_everything = YEA;
				break;
			}
			currline->len = currpnt;
			currline->data[currpnt] = '\0';
			break;
		default:
			break;
		}

	if (curr_window_line < 0) {
		curr_window_line = 0;
		if (!top_of_win->prev) {
			indigestion(6);
		} else {
			top_of_win = top_of_win->prev;
/*            redraw_everything = YEA ;
            move(t_lines-2,0);
            clrtoeol();
            refresh();*/
			rscroll();
		}
	}
	if (curr_window_line >= t_lines - 1) {
		for (i = curr_window_line - t_lines + 1; i >= 0; i--) {
			curr_window_line--;
			if (!top_of_win->next) {
				indigestion(7);
			} else {
				top_of_win = top_of_win->next;
				/*
				 * redraw_everything = YEA ;
				 * move(t_lines-1,0); clrtoeol(); refresh();
				 */
				scroll();
			}
		}
	}
	if (editansi /* || mark_on */ )
		redraw_everything = YEA;
	shift = (currpnt + 2 > STRLEN) ?
		(currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
	msgline();
	if (shifttmp != shift || redraw_everything == YEA) {
		redraw_everything = YEA;
		shifttmp = shift;
	} else
		redraw_everything = NA;

	move(curr_window_line, 0);
	if (currline->attr & M_MARK) {
		showansi = 1;
		cstrnput(currline->data + shift);
		showansi = 0;
	} else
		strnput(currline->data + shift);
	clrtoeol();
}

int
raw_vedit(filename, saveheader)
char   *filename;
int     saveheader;
{
	int     newch, ch = 0, foo, shift;
	read_file(filename);
	top_of_win = firstline;
	currline = firstline;
	curr_window_line = 0;
	currln = 0;
	currpnt = 0;
	clear();
	display_buffer();
	msgline();
	while (ch != EOF) {
		newch = '\0';
		switch (ch) {
		case Ctrl('W'):
		case Ctrl('X'):/* Save and exit */
			foo = write_file(filename, saveheader);
			if (foo != KEEP_EDITING)
				return foo;
			redraw_everything = YEA;
			break;
		case KEY_ESC:
			if (KEY_ESC_arg == KEY_ESC)
				insert_char(KEY_ESC);
			else {
				newch = vedit_process_ESC(KEY_ESC_arg);
				clear();
			}
			redraw_everything = YEA;
			break;
		default:
			vedit_key(ch);
		}
		if (redraw_everything) {
			display_buffer();
		}
		redraw_everything = NA;
		shift = (currpnt + 2 > STRLEN) ?
			(currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
		move(curr_window_line, currpnt - shift);

		ch = (newch != '\0') ? newch : igetkey();
	}
	return 1;
}

int
vedit(filename, saveheader)
char   *filename;
int     saveheader;
{
	int     ans, t;
	t = showansi;
	showansi = 0;
	init_alarm();
	ismsgline = (DEFINE(DEF_EDITMSG)) ? 1 : 0;
	msg();
	ans = raw_vedit(filename, saveheader);
	showansi = t;
	signal(SIGALRM, SIG_IGN);
	return ans;
}
