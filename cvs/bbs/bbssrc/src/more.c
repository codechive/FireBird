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
$Id: more.c,v 1.10 2002/09/05 06:04:10 edwardc Exp $
*/

#include "bbs.h"
extern time_t	login_start_time;
time_t  calltime = 0;
void    R_monitor();
struct shortfile *getbcache();

struct ACSHM {
	char    data[ACBOARD_MAXLINE][ACBOARD_BUFSIZE];
	int     movielines;
	int     movieitems;
	time_t  update;
};

struct ACSHM *movieshm;
int     nnline = 0, xxxline = 0;
char    more_buf[MORE_BUFSIZE];
int     more_size, more_num;

int
NNread_init()
{
	struct fileheader fh;
	FILE   *fp;
	char   *ptr;
	char    buf[ACBOARD_BUFSIZE], buf2[ACBOARD_BUFSIZE + 10];
	struct stat st;
	int     max = 0, i = 0, j = 0, x, y = 0;
	int     flag;		/* flag = 1 §Y¬°¹L¼{±¼ "--\n" ¥H«á¤§¥ô¦ó¤º®e */
	if (movieshm == NULL) {
		movieshm = (void *)attach_shm("ACBOARD_SHMKEY", 4123, sizeof(*movieshm));
		max = 1;	/* ªì¦¸¨Ï¥Î .. */
	}
	if (stat("boards/notepad/.DIGEST", &st) < 0) {
		empty_movie(1);
		return 1;
	}
	if (movieshm->update <= st.st_mtime && max == 0) {
		empty_movie(2);
		return 1;
	} else if (movieshm->update > st.st_mtime && max == 1)
		return 1;

	for (i = 0; i < ACBOARD_MAXLINE; i++)
		movieshm->data[i][0] = 0;

	max = get_num_records("boards/notepad/.DIGEST", sizeof(fh));

	i = 1;
	while (i <= max && j < ACBOARD_MAXLINE) {
		get_record("boards/notepad/.DIGEST", &fh, sizeof(fh), i++);
		sprintf(buf, "boards/notepad/%s", fh.filename);
		fp = fopen(buf, "r");

		if (fp == NULL)
			continue;

		y++;		/* record how many files have been append */

		/* if title[0] is '$' mean this is no header file */

		if (fh.title[0] != '$') {
			for (x = 0; x < 4; x++)
				fgets(buf, ACBOARD_BUFSIZE, fp);
		} else {
			for (x = 0; x < (int)(fh.title[1] - '0'); x++)
				fgets(buf, ACBOARD_BUFSIZE, fp);
		}

		flag = 0;
		for (x = 0; x < MAXMOVIE - 1; x++) {
			if (fgets(buf, ACBOARD_BUFSIZE, fp) != 0) {
				if (flag == 1 || strcmp(buf, "--\n") == 0) {
					strcpy(buf2, "[K");
					flag = (flag == 0) ? 1 : flag;
				}
				ptr = movieshm->data[j];

				if (flag == 0) {
					strcpy(buf2, "[K");
					strcat(buf2, buf);
				}
				memcpy(ptr, buf2, sizeof(buf2));
			} else {
				/* no data handling */
				strcpy(movieshm->data[j], "[K");
			}
			j++;
		}

		fclose(fp);
	}

	if (j == 0) {
		empty_movie(3);
		return 1;
	}
	movieshm->movielines = j;
	movieshm->movieitems = y;
	movieshm->update = time(0);

	sprintf(buf, "¬¡°Ê¬ÝªO§ó·s, ¦@ %d ¦æ, %d ³¡¥÷.", j, y);
	report(buf);

	return 1;
}

int
empty_movie(int x)
{
	sprintf(genbuf, "no source for stuff movie!! (%d)", x);
	report(genbuf);

#ifdef BIGGER_MOVIE
	strcpy(movieshm->data[1], "[K      ** ©|¥¼³]©w¬¡°Ê¬ÝªO ** ");
	strcpy(movieshm->data[3], "[K         ½Ð¸Ô¨£¦w¸Ë»¡©ú®Ñ Firebird-30 ");
	strcpy(movieshm->data[5], "[K         ³]©w notepad ªO");
#else
	strcpy(movieshm->data[2], "[K      ** ©|¥¼³]©w¬¡°Ê¬ÝªO ** ");
	strcpy(movieshm->data[3], "[K         ½Ð¸Ô¨£¦w¸Ë»¡©ú®Ñ Firebird-30 ");
	strcpy(movieshm->data[4], "[K         ³]©w notepad ªO");
#endif

	movieshm->movielines = MAXMOVIE;
	movieshm->movieitems = 1;
	movieshm->update = time(0);

}

void
setcalltime()
{
	char    ans[6];
	int     ttt;
	move(1, 0);
	clrtoeol();
	getdata(1, 0, "´X¤ÀÄÁ«á­n¨t²Î´£¿ô§A: ", ans, 3, DOECHO, YEA);
	if (!isdigit(ans[0]))
		return;
	ttt = atoi(ans);
	if (ttt <= 0)
		return;
	calltime = time(0) + ttt * 60;
}

int
readln(fd, buf)
int     fd;
char   *buf;
{
	int     len, bytes, in_esc, ch;
	len = bytes = in_esc = 0;
	while (1) {
		if (more_num >= more_size) {
			more_size = read(fd, more_buf, MORE_BUFSIZE);
			if (more_size == 0) {
				break;
			}
			more_num = 0;
		}
		ch = more_buf[more_num++];
		bytes++;
		if (ch == '\n' || bytes > 255) {
			break;
		} else if (ch == '\t') {
			do {
				len++, *buf++ = ' ';
			} while ((len % 8) != 0);
		} else if (ch == '') {
			if (showansi)
				*buf++ = ch;
			in_esc = 1;
		} else if (in_esc) {
			if (showansi)
				*buf++ = ch;
			if (strchr("[0123456789;,", ch) == NULL) {
				in_esc = 0;
			}
		} else if (isprint2(ch)) {
			if (len > 79)
				break;
			len++, *buf++ = ch;
		}
	}
	*buf++ = ch;
	*buf = '\0';
	return bytes;
}

int
morekey()
{
	int     ch;
	while (1) {
		switch (ch = egetch()) {
		case 'q':
		case KEY_LEFT:
		case EOF:
			return KEY_LEFT;
		case ' ':
		case KEY_RIGHT:
		case KEY_PGDN:
		case Ctrl('F'):
			return KEY_RIGHT;
		case KEY_PGUP:
		case Ctrl('B'):
			return KEY_PGUP;
		case '\r':
		case KEY_DOWN:
		case 'j':
			return KEY_DOWN;
		case 'k':
		case KEY_UP:
			return KEY_UP;
		case 'h':
		case 'H':
		case '?':
			return 'H';
		case 'y':
		case 'Y':
		case 'n':
		case 'N':
		case 'r':
		case 'R':
		case 'c':
		case 'C':
		case 'm':
		case 'M':
			return toupper(ch);
		case '0':
		case 'g': 
		case KEY_HOME:
			return KEY_HOME;
		case '$':
		case 'G': 
		case KEY_END:
			return KEY_END;
		default:;
		}
	}
}

int
seek_nth_line(fd, no)
int     fd, no;
{
	int     n_read, line_count, viewed;
	char   *p, *end;
	lseek(fd, 0, SEEK_SET);
	line_count = viewed = 0;
	if (no > 0)
		while (1) {
			n_read = read(fd, more_buf, MORE_BUFSIZE);
			p = more_buf;
			end = p + n_read;
			for (; p < end && line_count < no; p++)
				if (*p == '\n')
					line_count++;
			if (line_count >= no) {
				viewed += (p - more_buf);
				lseek(fd, (off_t) viewed, SEEK_SET);
				break;
			} else
				viewed += n_read;
		}

	more_num = MORE_BUFSIZE + 1;	/* invalidate the readln()'s buffer */
	return viewed;
}
/*Add by SmallPig*/
int
countln(fname)
char   *fname;
{
	FILE   *fp;
	char    tmp[256];
	int     count = 0;
	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	while (fgets(tmp, sizeof(tmp), fp) != NULL)
		count++;
	fclose(fp);
	return count;
}

int
more(filename, promptend)
char   *filename;
int     promptend;
{
	showansi = 0;
	return rawmore(filename, promptend);
	showansi = 1;
}
 /* below added by netty  *//* Rewrite by SmallPig */
void
netty_more()
{
	char    buf[256];
	int     ne_row = 1;
	int     x, y;
	time_t  thetime = time(0);
	if (!DEFINE(DEF_ACBOARD)) {
		update_endline();
		return;
	}
	/* 990613.edwardc ­×¥¿¤£·| refresh ªº°ÝÃD */
	nnline = (thetime / 10 % movieshm->movieitems) * (MAXMOVIE - 1);

	getyx(&y, &x);
	update_endline();
#ifdef BIGGER_MOVIE
	move(1, 0);
#else
	move(3, 0);
#endif
	while ((nnline < movieshm->movielines)) {
#ifndef BIGGER_MOVIE
		move(2 + ne_row, 0);
#else
		move(1 + ne_row, 0);
#endif
		clrtoeol();
		strcpy(buf, movieshm->data[nnline]);
		showstuff(buf, 0);
		nnline = nnline + 1;
		ne_row = ne_row + 1;
		if (nnline == movieshm->movielines) {
			nnline = 0;
			break;
		}
		if (ne_row > MAXMOVIE - 1) {
			break;
		}
	}
	move(y, x);
}
printacbar()
{
#ifndef BIGGER_MOVIE
	struct shortfile *bp;
	int     x, y;
	getyx(&y, &x);

	bp = getbcache(DEFAULTBOARD);
	move(2, 0);
	prints("[1;36m¢«¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢t[37m¬¡  °Ê  ¬Ý  ªO[36m¢u¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢ª [m\n");
	move(2 + MAXMOVIE, 0);
	if (bp->flag & VOTE_FLAG)
		prints("[1;36m¢©¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢t[37m¨t²Î§ë²¼¤¤ [ Config->Vote ] [36m¢u¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢¨ [m\n");
	else
		prints("[1;36m¢©¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢¨ [m\n");
	move(y, x);
#endif
	refresh();
}

check_calltime()
{
	int     line;
	time_t	now;
	
	now = time(0);
/*	
	if((now - login_start_time) > 28800) {
		bell(); bell(); bell();
		clear();
		prints("[1;44;32mBBS ¨t²Î³q§i: [37m%-65s[m", "±z¤w¤W¯¸ 8 ¤p®É¤F, ¥ð®§¤@¤U§a!");
		igetkey();
		abort_bbs();
	}
*/
	if (now >= calltime && calltime != 0) {
		if (uinfo.mode == TALK)
			line = t_lines / 2 - 1;
		else
			line = 0;
		saveline(line, 0);	/* restore line */
		bell();
		bell();
		bell();
		move(line, 0);
		clrtoeol();
		prints("[1;44;32mBBS ¨t²Î³q§i: [37m%-65s[m", "¨t²Î¾xÄÁ ¹a¡ã¡ã¡ã¡ã¡ã¡ã");
		igetkey();
		move(line, 0);
		clrtoeol();
		saveline(line, 1);
		calltime = 0;
	}
}

void
R_monitor()
{

	if (!DEFINE(DEF_ACBOARD) && !DEFINE(DEF_ENDLINE))
		return;

	if (uinfo.mode != MMENU)
		return;
	alarm(0);
	signal(SIGALRM, R_monitor);
	netty_more();
	printacbar();
	if (!DEFINE(DEF_ACBOARD))
		alarm(60);
	else
		alarm(10);
}


/*rawmore2() ansimore2() Add by SmaLLPig*/
int
rawmore(filename, promptend, row, numlines, stuffmode)
char   *filename;
int     promptend;
int     row;
int     numlines;
int     stuffmode;
{
	extern int t_lines;
	struct stat st;
	int     fd, tsize;
	char    buf[256];
	int     i, ch, viewed, pos, isin = NA, titleshow = NA;
	int     numbytes;
	int     curr_row = row;
	int     linesread = 0;
	if ((fd = open(filename, O_RDONLY)) == -1) {
		return -1;
	}
	if (fstat(fd, &st)) {
		return -1;
	}
	tsize = st.st_size;
	more_size = more_num = 0;

	clrtobot();
	i = pos = viewed = 0;
	numbytes = readln(fd, buf);
	curr_row++;
	linesread++;
	while (numbytes) {
		if (linesread <= numlines || numlines == 0) {
			viewed += numbytes;
			if (!titleshow &&
				(!strncmp(buf, "¡¼ ¤Þ¥Î", 7))
				|| (!strncmp(buf, "==>", 3))
				|| (!strncmp(buf, "¡i ¦b", 5))
				|| (!strncmp(buf, "¡° ¤Þ­z", 7))) {
				prints("[1;33m%s[m", buf);
				titleshow = YEA;
			} else if (buf[0] != ':' && buf[0] != '>') {
				if (isin == YEA) {
					isin = NA;
					prints("[37m");
				}
				if (check_stuffmode() || stuffmode == YEA)
					showstuff(buf, 0);
				else
					prints("%s", buf);
			} else {
				prints("[36m");
				if (check_stuffmode() || stuffmode == YEA)
					showstuff(buf, 0);
				else
					prints("%s", buf);
				isin = YEA;
			}
			i++;
			pos++;
			if (pos == t_lines) {
				scroll();
				pos--;
			}
			numbytes = readln(fd, buf);
			curr_row++;
			linesread++;
			if (numbytes == 0)
				break;
			if (i == t_lines - 1) {
				if (showansi) {
					move(t_lines - 1, 0);
					prints("[0m[m");
					refresh();
				}
				move(t_lines - 1, 0);
				clrtoeol();
				prints("[1;44;32m¤U­±ÁÙ¦³³á (%d%%)[33m   ¢x µ²§ô ¡ö <q> ¢x ¡ô/¡õ/PgUp/PgDn ²¾°Ê ¢x ? »²§U»¡©ú ¢x     [m", (viewed * 100) / tsize);
				ch = morekey();
				move(t_lines - 1, 0);
				clrtoeol();
				refresh();
				if (ch == KEY_LEFT) {
					close(fd);
					return ch;
				} else if (ch == KEY_RIGHT) {
					i = 1;
				} else if (ch == KEY_DOWN) {
					i = t_lines - 2;
				} else if (ch == KEY_PGUP || ch == KEY_UP) {
					clear();
					i = pos = 0;
					curr_row -= (ch == KEY_PGUP) ? (2 * t_lines - 2) : (t_lines + 1);
					if (curr_row < 0) {
						close(fd);
						return ch;
					}
					viewed = seek_nth_line(fd, curr_row);
					numbytes = readln(fd, buf);
					curr_row++;
				} else if (ch == 'H') {
					show_help(F_HELP_MORE);
					i = pos = 0;
					curr_row -= (t_lines);
					if (curr_row < 0)
						curr_row = 0;
					viewed = seek_nth_line(fd, curr_row);
					numbytes = readln(fd, buf);
					curr_row++;
				} else if (ch == KEY_HOME ) {
					clear();
					curr_row = 0;
				} else if( ch == KEY_END ) {
					curr_row = t_lines;
				}
			}
		} else
			break;	/* More Than Want */
	}
	close(fd);
	if (promptend) {
		pressanykey();
	}
	return 0;
}

int
mesgmore(filename, promptend, row, numlines)
char   *filename;
int     promptend;
int     row;
int     numlines;
{
	extern int t_lines;
	struct stat st;
	int     fd, tsize;
	char    buf[256];
	int     i, ch, viewed, pos, isin = NA;
	int     numbytes;
	int     curr_row = row;
	int     linesread = 0;
	int     qflag = 0;
	time_t  now;
	char    title[256];
	if ((fd = open(filename, O_RDONLY)) == -1) {
		return -1;
	}
	if (fstat(fd, &st)) {
		return -1;
	}
	tsize = st.st_size;
	more_size = more_num = 0;

	clrtobot();
	i = pos = viewed = 0;
	numbytes = readln(fd, buf);
	curr_row++;
	linesread++;
	while (numbytes) {
		if (linesread <= numlines || numlines == 0) {
			viewed += numbytes;

			prints("%s", buf);

			isin = YEA;
			i++;
			pos++;
			if (pos == t_lines) {
				scroll();
				pos--;
			}
			numbytes = readln(fd, buf);
			curr_row++;
			linesread++;
			if (numbytes == 0)
				break;
			if (i == t_lines - 1) {
				if (showansi) {
					move(t_lines - 1, 0);
					prints("[0m[[m");
					refresh();
				}
				move(t_lines - 1, 0);
				clrtoeol();
				prints("[0m[1;44;32m(%d%%) ¬O§_Ä~Äò [[1;37mY/n[1;32m]   «O¯d <[1;37mr[32m>    ²M°£ <[1;37mc[1;32m>   ±H¦^«H½c <[1;37mm[1;32m>                      [m", (viewed * 100) / tsize);
				ch = morekey();
				move(t_lines - 1, 0);
				clrtoeol();
				refresh();
				if (ch == KEY_LEFT) {
					qflag = 1;
					break;
				} else if (ch == KEY_RIGHT) {
					i = 1;
				} else if (ch == KEY_DOWN) {
					i = t_lines - 2;
				} else if (ch == KEY_PGUP || ch == KEY_UP) {
					clear();
					i = pos = 0;
					curr_row -= (ch == KEY_PGUP) ? (2 * t_lines - 2) : (t_lines + 1);
					if (curr_row < 0) {
						qflag = 1;
						break;
					}
					viewed = seek_nth_line(fd, curr_row);
					numbytes = readln(fd, buf);
					curr_row++;
				} else if (ch == 'H') {
					show_help(F_HELP_MSG);
					i = pos = 0;
					curr_row -= (t_lines);
					if (curr_row < 0)
						curr_row = 0;
					viewed = seek_nth_line(fd, curr_row);
					numbytes = readln(fd, buf);
					curr_row++;
				} else if (ch == 'C') {
					close(fd);
					unlink(filename);
					return ch;
				} else if (ch == 'M') {
					close(fd);
					now = time(0);
					sprintf(title, "[%12.12s] ©Ò¦³°T®§³Æ¥÷", ctime(&now) + 4);
					mail_file(filename, currentuser.userid, title);
					unlink(filename);
					return ch;
				} else if (ch == 'N' || ch == 'R') {
					qflag = 1;
					break;
				} else if (ch == 'Y') {
					i = 1;
				}
			}
		} else
			break;	/* More Than Want */
	}

	close(fd);

	if ((qflag != 1) && (promptend == YEA || pos < t_lines))
		msgmorebar(filename);

	if (dashf(filename) && (tsize > 65536)) {
		clear();
		prints("±zªº°T®§°O¿ýÀÉ¤w¸gº¡¸ü, §Y±N³Q²M°£¡C\n");
		if (askyn("­n±N³o¨Ç°T®§³Æ¥÷¦Ü«H½c¶Ü", YEA, NA) == YEA) {
			now = time(0);
			sprintf(title, "[%12.12s] ©Ò¦³°T®§³Æ¥÷", ctime(&now) + 4);
			mail_file(filename, currentuser.userid, title);
		}
		unlink(filename);
	}
	return ch;
}

int
ansimore(filename, promptend)
char   *filename;
int     promptend;
{
	int     ch, stuff = NA;
	clear();

	if (strstr(filename, "vote") && strstr(filename, "note"))
		stuff = YEA;

	ch = rawmore(filename, promptend, 0, 0, stuff);
	move(t_lines - 1, 0);
	prints("[0m[m");
	refresh();
	return ch;
}

int
ansimore2(filename, promptend, row, numlines)
char   *filename;
int     promptend;
int     row;
int     numlines;
{
	int     ch, stuff = NA;
	if (strstr(filename, "vote") && strstr(filename, "note"))
		stuff = YEA;

	ch = rawmore(filename, promptend, row, numlines, stuff);
	refresh();
	return ch;
}
/* edwardc.990624 ¥ý¼È®É¥Î ansimore3() ¥N´À ... */

int
ansimore3(filename, promptend)
char   *filename;
int     promptend;
{
	int     ch;
	clear();
	ch = rawmore(filename, promptend, 0, 0, YEA);
	move(t_lines - 1, 0);
	prints("[0m[m");
	refresh();
	return ch;
}
