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
$Id: boards.c,v 1.6 2003/01/05 00:22:02 edwardc Exp $
*/

#include "bbs.h"

#define BBS_PAGESIZE    (t_lines - 4)

#define BRC_MAXSIZE     90000
#define BRC_MAXNUM      60
#define BRC_STRLEN      15
#define BRC_ITEMSIZE    (BRC_STRLEN + 1 + BRC_MAXNUM * sizeof( int ))
#define UNREAD_TIME     (login_start_time - 30 * 86400)

char    brc_buf[BRC_MAXSIZE];
int     brc_size, brc_changed = 0;
char    brc_name[BRC_STRLEN];
int     brc_list[BRC_MAXNUM], brc_num;

char   *sysconf_str();
extern time_t login_start_time;
extern int numboards;
extern struct shortfile *bcache;

struct newpostdata {
	char   *name, *title, *BM;
	char    flag;
	int     pos, total;
	char    unread, zap;
	char    status;
}      *nbrd;

int    *zapbuf;
int     brdnum, yank_flag = 0;
char   *boardprefix;

void
EGroup(cmd)
char   *cmd;
{
	char    buf[STRLEN];
	sprintf(buf, "EGROUP%c", *cmd);
	boardprefix = sysconf_str(buf);
	choose_board(DEFINE(DEF_NEWPOST) ? 1 : 0);
}

void
Boards()
{
	boardprefix = NULL;
	choose_board(0);
}

void
New()
{
	if (heavyload()) {
		clear();
		prints("©êºp¡A¥Ø«e¨t²Î­t²ü¹L­«¡A½Ð§ï¥Î Boards «ü¥O¾\\Äý°Q½×°Ï...");
		pressanykey();
		return;
	}
	boardprefix = NULL;
	choose_board(1);
}

void
load_zapbuf()
{
	char    fname[STRLEN];
	int     fd, size, n;
	size = MAXBOARD * sizeof(int);
	zapbuf = (int *) malloc(size);
	for (n = 0; n < MAXBOARD; n++)
		zapbuf[n] = 1;
	setuserfile(fname, ".lastread");
	if ((fd = open(fname, O_RDONLY, 0600)) != -1) {
		size = numboards * sizeof(int);
		read(fd, zapbuf, size);
		close(fd);
	}
}

void
save_zapbuf()
{
	char    fname[STRLEN];
	int     fd, size;
	setuserfile(fname, ".lastread");
	if ((fd = open(fname, O_WRONLY | O_CREAT, 0600)) != -1) {
		size = numboards * sizeof(int);
		write(fd, zapbuf, size);
		close(fd);
	}
}

int
load_boards()
{
	struct shortfile *bptr;
	struct newpostdata *ptr;
	int     n;
	resolve_boards();
	if (zapbuf == NULL) {
		load_zapbuf();
	}
	brdnum = 0;
	for (n = 0; n < numboards; n++) {
		bptr = &bcache[n];

	/* Harimau.010115 dont show deleted board */
#ifdef NO_DELETED_BOARD
	if (bptr->filename[0]=='\0') {
		continue;
	}
#endif
		if (!(bptr->level & PERM_POSTMASK) && !HAS_PERM(bptr->level) && !(bptr->level & PERM_NOZAP)) {
			continue;
		}
		if (boardprefix != NULL &&
			strchr(boardprefix, bptr->title[0]) == NULL && boardprefix[0] != '*')
			continue;
		if (boardprefix != NULL && boardprefix[0] == '*') {
			if (!strstr(bptr->title, "¡´") && !strstr(bptr->title, "¡ó")
				&& bptr->title[0] != '*')
				continue;
		}
		if (boardprefix == NULL && bptr->title[0] == '*')
			continue;
		if (yank_flag || zapbuf[n] != 0 || (bptr->level & PERM_NOZAP)) {
			ptr = &nbrd[brdnum++];
			ptr->name = bptr->filename;
			ptr->title = bptr->title;
			ptr->BM = bptr->BM;
			ptr->flag = bptr->flag | ((bptr->level & PERM_NOZAP) ? NOZAP_FLAG : 0);
			ptr->pos = n;
			ptr->total = -1;
			ptr->zap = (zapbuf[n] == 0);
			if (bptr->level & PERM_POSTMASK)
				ptr->status = 'p';
			else if (bptr->level & PERM_NOZAP)
				ptr->status = 'z';
			else if ((bptr->level & ~PERM_POSTMASK) != 0)
				ptr->status = 'r';
			else
				ptr->status = ' ';
		}
	}
	if (brdnum == 0 && !yank_flag && boardprefix == NULL) {
		brdnum = -1;
		yank_flag = 1;
		return -1;
	}
	return 0;
}

int
search_board(num)
int    *num;
{
	static int i = 0, find = YEA;
	static char bname[STRLEN];
	int     n, ch, tmpn = NA;
	if (find == YEA) {
		bzero(bname, sizeof(bname));
		find = NA;
		i = 0;
	}
	while (1) {
		move(t_lines - 1, 0);
		clrtoeol();
		prints("½Ð¿é¤J­n§ä´Mªº board ¦WºÙ¡G%s", bname);
		ch = egetch();

		if (isprint2(ch)) {
			bname[i++] = ch;
			for (n = 0; n < brdnum; n++) {
				if (!ci_strncmp(nbrd[n].name, bname, i)) {
					tmpn = YEA;
					*num = n;
					if (!strcmp(nbrd[n].name, bname))
						return 1	/* §ä¨ìÃþ¦üªºªO¡Aµe­±­«µe
							    */ ;
				}
			}
			if (tmpn)
				return 1;
			if (find == NA) {
				bname[--i] = '\0';
			}
			continue;
		} else if (ch == Ctrl('H') || ch == KEY_LEFT || ch == KEY_DEL ||
			ch == '\177') {
			i--;
			if (i < 0) {
				find = YEA;
				break;
			} else {
				bname[i] = '\0';
				continue;
			}
		} else if (ch == '\t') {
			find = YEA;
			break;
		} else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
			find = YEA;
			break;
		}
		bell(1);
	}
	if (find) {
		move(t_lines - 1, 0);
		clrtoeol();
		return 2 /* µ²§ô¤F */ ;
	}
	return 1;
}

int
check_newpost(ptr)
struct newpostdata *ptr;
{
	struct fileheader fh;
	struct stat st;
	char    filename[STRLEN];
	int     fd, offset, total;
	ptr->total = ptr->unread = 0;
	setbdir(genbuf, ptr->name);
	if ((fd = open(genbuf, O_RDWR)) < 0)
		return 0;
	fstat(fd, &st);
	total = st.st_size / sizeof(fh);
	if (total <= 0) {
		close(fd);
		return 0;
	}
	ptr->total = total;
	if (!brc_initial(ptr->name)) {
		ptr->unread = 1;
	} else {
		offset = (int) ((char *) &(fh.filename[0]) - (char *) &(fh));
		lseek(fd, (off_t) (offset + (total - 1) * sizeof(fh)), SEEK_SET);
		if (read(fd, filename, STRLEN) > 0 && brc_unread(filename)) {
			ptr->unread = 1;
		}
	}
	close(fd);
	return 1;
}

int
unread_position(dirfile, ptr)
char   *dirfile;
struct newpostdata *ptr;
{
	struct fileheader fh;
	char    filename[STRLEN];
	int     fd, offset, step, num;
	num = ptr->total + 1;
	if (ptr->unread && (fd = open(dirfile, O_RDWR)) > 0) {
		if (!brc_initial(ptr->name)) {
			num = 1;
		} else {
			offset = (int) ((char *) &(fh.filename[0]) - (char *) &(fh));
			num = ptr->total - 1;
			step = 4;
			while (num > 0) {
				lseek(fd, (off_t) (offset + num * sizeof(fh)), SEEK_SET);
				if (read(fd, filename, STRLEN) <= 0 ||
					!brc_unread(filename))
					break;
				num -= step;
				if (step < 32)
					step += step / 2;
			}
			if (num < 0)
				num = 0;
			while (num < ptr->total) {
				lseek(fd, (off_t) (offset + num * sizeof(fh)), SEEK_SET);
				if (read(fd, filename, STRLEN) <= 0 ||
					brc_unread(filename))
					break;
				num++;
			}
		}
		close(fd);
	}
	if (num < 0)
		num = 0;
	return num;
}

void
show_brdlist(page, clsflag, newflag)
int     page, clsflag, newflag;
{
	struct newpostdata *ptr;
	int     n;
	char    tmpBM[BM_LEN - 1];
	if (clsflag) {
		clear();
		docmdtitle("[°Q½×°Ï¦Cªí]", "  [m¥D¿ï³æ[[1;32m¡ö[m,[1;32me[m] ¾\\Åª[[1;32m¡÷[m,[1;32mRtn[m] ¿ï¾Ü[[1;32m¡ô[m,[1;32m¡õ[m] ¦C¥X[[1;32my[m] ±Æ§Ç[[1;32ms[m] ·j´M[[1;32m/[m] ¤Á´«[[1;32mc[m] ¨D§U[[1;32mh[m]\n");
		prints("[1;44;37m  %s °Q½×°Ï¦WºÙ       V  Ãþ§O  Âà %-25s S ªO  ¥D   %s   [m\n",
			newflag ? "¥þ³¡  ¥¼" : "½s¸¹  ", "¤¤  ¤å  ±Ô  ­z", newflag ? "" : "   ");
	}
	move(3, 0);
	for (n = page; n < page + BBS_PAGESIZE; n++) {
		if (n >= brdnum) {
			prints("\n");
			continue;
		}
		ptr = &nbrd[n];
		if (!newflag)
			prints(" %5d %c", n + 1, ptr->zap && !(ptr->flag & NOZAP_FLAG) ? '-' : ' ');
		else if (ptr->zap && !(ptr->flag & NOZAP_FLAG)) {
			ptr->total = ptr->unread = 0;
			prints("     -   -");
		} else {
			if (ptr->total == -1) {
				refresh();
				check_newpost(ptr);
			}
			prints(" %5d  %s", ptr->total, ptr->unread ? "¡»" : "¡º");
		}
		strcpy(tmpBM, ptr->BM);
		prints(" %-16s %s %-35s %c %-12s\n", ptr->name, (ptr->flag & VOTE_FLAG) ? "[1;31mV[m" : " "
			,ptr->title + 1, HAS_PERM(PERM_POST) ? ptr->status : ' ',
			ptr->BM[0] <= ' ' ? "¸Û¼xªO¥D¤¤" : strtok(tmpBM, " "));
	}
}

int
cmpboard(brd, tmp)
struct newpostdata *brd, *tmp;
{
	register int type = 0;
	if (!(currentuser.flags[0] & BRDSORT_FLAG)) {
		type = brd->title[0] - tmp->title[0];
		if (type == 0)
			type = ci_strncmp(brd->title + 1, tmp->title + 1, 6);

	}
	if (type == 0)
		type = ci_strcmp(brd->name, tmp->name);
	return type;
}

int
choose_board(newflag)
int     newflag;
{
	static int num;
	struct newpostdata newpost_buffer[MAXBOARD];
	struct newpostdata *ptr;
	int     page, ch, tmp, number, tmpnum;
	int     loop_mode = 0;
	if (!strcmp(currentuser.userid, "guest"))
		yank_flag = 1;
	nbrd = newpost_buffer;
	modify_user_mode(newflag ? READNEW : READBRD);
	brdnum = number = 0;
	show_brdlist(0, 1, newflag);
	while (1) {
		if (brdnum <= 0) {
			if (load_boards() == -1)
				continue;
			qsort(nbrd, brdnum, sizeof(nbrd[0]), cmpboard);
			page = -1;
			if (brdnum <= 0)
				break;
		}
		if (num < 0)
			num = 0;
		if (num >= brdnum)
			num = brdnum - 1;
		if (page < 0) {
			if (newflag) {
				tmp = num;
				while (num < brdnum) {
					ptr = &nbrd[num];
					if (ptr->total == -1)
						check_newpost(ptr);
					if (ptr->unread)
						break;
					num++;
				}
				if (num >= brdnum)
					num = tmp;
			}
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			show_brdlist(page, 1, newflag);
			update_endline();
		}
		if (num < page || num >= page + BBS_PAGESIZE) {
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			show_brdlist(page, 0, newflag);
			update_endline();
		}
		move(3 + num - page, 0);
		prints(">", number);
		if (loop_mode == 0) {
			ch = egetch();
		}
		move(3 + num - page, 0);
		prints(" ");
		if (ch == 'q' || ch == 'e' || ch == KEY_LEFT || ch == EOF)
			break;
		switch (ch) {
		case 'P':
		case 'b':
		case Ctrl('B'):
		case KEY_PGUP:
			if (num == 0)
				num = brdnum - 1;
			else
				num -= BBS_PAGESIZE;
			break;
		case 'C':
		case 'c':
			if (newflag == 1)
				newflag = 0;
			else
				newflag = 1;
			show_brdlist(page, 1, newflag);
			break;
		case 'L':	/* ppfoong */
			show_allmsgs();
			page = -1;
			break;
		case 'N':
		case ' ':
		case Ctrl('F'):
		case KEY_PGDN:
			if (num == brdnum - 1)
				num = 0;
			else
				num += BBS_PAGESIZE;
			break;
		case 'p':
		case 'k':
		case KEY_UP:
			if (num-- <= 0)
				num = brdnum - 1;
			break;
		case 'n':
		case 'j':
		case KEY_DOWN:
			if (++num >= brdnum)
				num = 0;
			break;
		case '$':
			num = brdnum - 1;
			break;
		case '!':	/* youzi leave */
			return Q_Goodbye();
			break;
		case 'h':
			show_help(F_HELP_BRDREAD);
			page = -1;
			break;
		case '/':
			move(3 + num - page, 0);
			prints(">", number);
			tmpnum = num;
			tmp = search_board(&num);
			move(3 + tmpnum - page, 0);
			prints(" ", number);
			if (tmp == 1)
				loop_mode = 1;
			else {
				loop_mode = 0;
				update_endline();
			}
			break;
		case 's':	/* sort/unsort -mfchen */
			currentuser.flags[0] ^= BRDSORT_FLAG;
			qsort(nbrd, brdnum, sizeof(nbrd[0]), cmpboard);
			page = 999;
			break;
		case 'y':
			yank_flag = !yank_flag;
			brdnum = -1;
			break;
		case 'z':
			if (HAS_PERM(PERM_BASIC) && !(nbrd[num].flag & NOZAP_FLAG)) {
				ptr = &nbrd[num];
				ptr->zap = !ptr->zap;
				ptr->total = -1;
				zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
				page = 999;
			}
			break;
		case KEY_HOME:
			num = 0;
			break;
		case KEY_END:
			num = brdnum - 1;
			break;
		case '\n':
		case '\r':
			if (number > 0) {
				num = number - 1;
				break;
			}
			/* fall through */
		case KEY_RIGHT:
			{
				char    buf[STRLEN];
				ptr = &nbrd[num];
				brc_initial(ptr->name);
				memcpy(currBM, ptr->BM, BM_LEN - 1);
				if (DEFINE(DEF_FIRSTNEW)) {
					setbdir(buf, currboard);
					tmp = unread_position(buf, ptr);
					page = tmp - t_lines / 2;
					getkeep(buf, page > 1 ? page : 1, tmp + 1);
				}
				Read();

				if (zapbuf[ptr->pos] > 0 && brc_num > 0) {
					zapbuf[ptr->pos] = brc_list[0];
				}
				ptr->total = page = -1;
				modify_user_mode(newflag ? READNEW : READBRD);
				break;
			}
		case 'S':	/* sendmsg ... youzi */
			if (!HAS_PERM(PERM_PAGE))
				break;
			s_msg();
			page = -1;
			break;
		case 'f':	/* show friends ... youzi */
			if (!HAS_PERM(PERM_BASIC))
				break;
			t_friends();
			page = -1;
			break;
		default:
			;
		}
		if (ch >= '0' && ch <= '9') {
			number = number * 10 + (ch - '0');
			ch = '\0';
		} else {
			number = 0;
		}
	}
	clear();
	save_zapbuf();
	return -1;
}

char   *
brc_getrecord(ptr, name, pnum, list)
char   *ptr, *name;
int    *pnum, *list;
{
	int     num;
	char   *tmp;
	strncpy(name, ptr, BRC_STRLEN);
	ptr += BRC_STRLEN;
	num = (*ptr++) & 0xff;
	tmp = ptr + num * sizeof(int);
	if (num > BRC_MAXNUM) {
		num = BRC_MAXNUM;
	}
	*pnum = num;
	memcpy(list, ptr, num * sizeof(int));
	return tmp;
}

char   *
brc_putrecord(ptr, name, num, list)
char   *ptr, *name;
int     num, *list;
{
	if (num > 0 /* && list[0] > UNREAD_TIME */ ) {
		if (num > BRC_MAXNUM) {
			num = BRC_MAXNUM;
		}
/*        while( num > 1 && list[num-1] < UNREAD_TIME ) {
            num--;
        }*/
		strncpy(ptr, name, BRC_STRLEN);
		ptr += BRC_STRLEN;
		*ptr++ = num;
		memcpy(ptr, list, num * sizeof(int));
		ptr += num * sizeof(int);
	}
	return ptr;
}

void
brc_update()
{
	char    dirfile[STRLEN], *ptr;
	char    tmp_buf[BRC_MAXSIZE - BRC_ITEMSIZE], *tmp;
	char    tmp_name[BRC_STRLEN];
	int     tmp_list[BRC_MAXNUM], tmp_num;
	int     fd, tmp_size;
	if (brc_changed == 0) {
		return;
	}
	ptr = brc_buf;
	if (brc_num > 0) {
		ptr = brc_putrecord(ptr, brc_name, brc_num, brc_list);
	}
	if (1) {
		setuserfile(dirfile, ".boardrc");
		if ((fd = open(dirfile, O_RDONLY)) != -1) {
			tmp_size = read(fd, tmp_buf, sizeof(tmp_buf));
			close(fd);
		} else {
			tmp_size = 0;
		}
	}
	tmp = tmp_buf;
	while (tmp < &tmp_buf[tmp_size] && (*tmp >= ' ' && *tmp <= 'z')) {
		tmp = brc_getrecord(tmp, tmp_name, &tmp_num, tmp_list);
		if (strncmp(tmp_name, currboard, BRC_STRLEN) != 0) {
			ptr = brc_putrecord(ptr, tmp_name, tmp_num, tmp_list);
		}
	}
	brc_size = (int) (ptr - brc_buf);

	if ((fd = open(dirfile, O_WRONLY | O_CREAT, 0644)) != -1) {
		ftruncate(fd, 0);
		write(fd, brc_buf, brc_size);
		close(fd);
	}
	brc_changed = 0;
}

int
brc_initial(boardname)
char   *boardname;
{
	char    dirfile[STRLEN], *ptr;
	int     fd;
	if (strcmp(currboard, boardname) == 0) {
		return brc_num;
	}
	brc_update();
	strcpy(currboard, boardname);
	brc_changed = 0;
	if (brc_buf[0] == '\0') {
		setuserfile(dirfile, ".boardrc");
		if ((fd = open(dirfile, O_RDONLY)) != -1) {
			brc_size = read(fd, brc_buf, sizeof(brc_buf));
			close(fd);
		} else {
			brc_size = 0;
		}
	}
	ptr = brc_buf;
	while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
		ptr = brc_getrecord(ptr, brc_name, &brc_num, brc_list);
		if (strncmp(brc_name, currboard, BRC_STRLEN) == 0) {
			return brc_num;
		}
	}
	strncpy(brc_name, boardname, BRC_STRLEN);
	brc_list[0] = 1;
	brc_num = 1;
	return 0;
}

void
brc_addlist(filename)
char   *filename;
{
	int     ftime, n, i;
	if (!strcmp(currentuser.userid, "guest"))
		return;
	ftime = atoi(&filename[2]);
	if ((filename[0] != 'M' && filename[0] != 'G') || filename[1] != '.'	/* || ftime <=
		    UNREAD_TIME */ ) {
		return;
	}
	if (brc_num <= 0) {
		brc_list[brc_num++] = ftime;
		brc_changed = 1;
		return;
	}
	for (n = 0; n < brc_num; n++) {
		if (ftime == brc_list[n]) {
			return;
		} else if (ftime > brc_list[n]) {
			if (brc_num < BRC_MAXNUM)
				brc_num++;
			for (i = brc_num - 1; i > n; i--) {
				brc_list[i] = brc_list[i - 1];
			}
			brc_list[n] = ftime;
			brc_changed = 1;
			return;
		}
	}
	if (brc_num < BRC_MAXNUM) {
		brc_list[brc_num++] = ftime;
		brc_changed = 1;
	}
}

int
brc_unread(filename)
char   *filename;
{
	int     ftime, n;
	ftime = atoi(&filename[2]);
	if ((filename[0] != 'M' && filename[0] != 'G') || filename[1] != '.'	/* || ftime <=
		    UNREAD_TIME */ ) {
		return 0;
	}
	if (brc_num <= 0)
		return 1;
	for (n = 0; n < brc_num; n++) {
		if (ftime > brc_list[n]) {
			return 1;
		} else if (ftime == brc_list[n]) {
			return 0;
		}
	}
	return 0;
}
