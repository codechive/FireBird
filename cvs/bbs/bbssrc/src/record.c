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
$Id: record.c,v 1.5 2001/12/31 08:13:13 chinsan Exp $
*/

#include "bbs.h"

#define BUFSIZE (8192)

int
safewrite(fd, buf, size)
int     fd;
char   *buf;
int     size;
{
	int     cc, sz = size, origsz = size;
	char   *bp = buf;
#ifdef POSTBUG
	if (size == sizeof(struct fileheader)) {
		char    tmp[80];
		struct stat stbuf;
		struct fileheader *fbuf = (struct fileheader *) buf;
		setbpath(tmp, fbuf->filename);
		if (!isalpha(fbuf->filename[0]) || stat(tmp, &stbuf) == -1)
			if (fbuf->filename[0] != 'M' || fbuf->filename[1] != '.') {
				report("safewrite: foiled attempt to write bugged record\n");
				return origsz;
			}
	}
#endif
	do {
		cc = write(fd, bp, sz);
		if ((cc < 0) && (errno != EINTR)) {
			report("safewrite err!");
			return -1;
		}
		if (cc > 0) {
			bp += cc;
			sz -= cc;
		}
	} while (sz > 0);
	return origsz;
}
#ifdef POSTBUG

char    bigbuf[10240];
int     numtowrite;
int     bug_possible = 0;

int
saverecords(filename, size, pos)
char   *filename;
int     size, pos;
{
	int     fd;
	if (!bug_possible)
		return 0;
	if ((fd = open(filename, O_RDONLY)) == -1)
		return -1;
	if (pos > 5)
		numtowrite = 5;
	else
		numtowrite = 4;
	lseek(fd, (off_t) ((pos - numtowrite - 1) * size), SEEK_SET);
	read(fd, bigbuf, numtowrite * size);
}

int
restorerecords(filename, size, pos)
char   *filename;
int     size, pos;
{
	int     fd;
	if (!bug_possible)
		return 0;
	if ((fd = open(filename, O_WRONLY)) == -1)
		return -1;
	f_exlock(fd);
	lseek(fd, (off_t) ((pos - numtowrite - 1) * size), SEEK_SET);
	safewrite(fd, bigbuf, numtowrite * size);
	report("post bug poison set out!");
	f_unlock(fd);
	bigbuf[0] = '\0';
	close(fd);
}
#endif

long
get_num_records(filename, size)
char   *filename;
int     size;
{
	struct stat st;
	if (stat(filename, &st) == -1)
		return 0;
	return (st.st_size / size);
}

/* chinsan.20011231: For the calc of mail sum  (port from bbs.ilc.edu.tw) */
int
get_sum_records(char* fpath, int size)
{
	struct stat st;
	long ans = 0;
	FILE*	fp;
	struct fileheader fhdr;
	char buf[200], *p;

	if (!(fp = fopen(fpath, "r")))
		return -1;

	strcpy(buf, fpath);
	p = strrchr(buf, '/') + 1;

	while (fread(&fhdr, size, 1, fp) == 1) {
      		strcpy(p, fhdr.filename);
      		if (stat(buf, &st) == 0 && S_ISREG(st.st_mode) && st.st_nlink == 1)
         		ans += st.st_size;
	}
   	fclose(fp);
   	return ans / 1024;
}

int
append_record(filename, record, size)
char   *filename;
char   *record;
int     size;
{
	int     fd;
#ifdef POSTBUG
	int     numrecs = (int) get_num_records(filename, size);
	bug_possible = 1;
	if (size == sizeof(struct fileheader) && numrecs && (numrecs % 4 == 0))
		saverecords(filename, size, numrecs + 1);
#endif
	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {
		perror("open");
		return -1;
	}
	f_exlock(fd);
	lseek(fd, 0, SEEK_END);
	if (safewrite(fd, record, size) == -1)
		report("apprec write err!");
	f_unlock(fd);
	close(fd);
#ifdef POSTBUG
	if (size == sizeof(struct fileheader) && numrecs && (numrecs % 4 == 0))
		restorerecords(filename, size, numrecs + 1);
	bug_possible = 0;
#endif
	return 0;
}

void
toobigmesg()
{
	fprintf(stderr, "record size too big!!\n");
}

int
apply_record(filename, fptr, size)
char   *filename;
int     (*fptr) ();
int     size;
{
	char    abuf[BUFSIZE];
	int     fd;
	if (size > BUFSIZE) {
		toobigmesg();
		return -1;
	}
	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	while (read(fd, abuf, size) == size)
		if ((*fptr) (abuf) == QUIT) {
			close(fd);
			return QUIT;
		}
	close(fd);
	return 0;
}

int
search_record(filename, rptr, size, fptr, farg)
char   *filename;
char   *rptr;
int     size;
int     (*fptr) ();
char   *farg;
{
	int     fd;
	int     id = 1;
	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return 0;
	while (read(fd, rptr, size) == size) {
		if ((*fptr) (farg, rptr)) {
			close(fd);
			return id;
		}
		id++;
	}
	close(fd);
	return 0;
}

int
get_record(filename, rptr, size, id)
char   *filename;
char   *rptr;
int     size, id;
{
	int     fd;
	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		close(fd);
		return -1;
	}
	if (read(fd, rptr, size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int
get_records(filename, rptr, size, id, number)
char   *filename;
char   *rptr;
int     size, id, number;
{
	int     fd;
	int     n;
	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		close(fd);
		return 0;
	}
	if ((n = read(fd, rptr, size * number)) == -1) {
		close(fd);
		return -1;
	}
	close(fd);
	return (n / size);
}

int
substitute_record(filename, rptr, size, id)
char   *filename;
char   *rptr;
int     size, id;
{
	int     fd;
#ifdef POSTBUG
	if (size == sizeof(struct fileheader) && (id > 1) && ((id - 1) % 4 == 0))
		saverecords(filename, size, id);
#endif
	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1)
		return -1;
	f_exlock(fd);
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1)
		report("subrec seek err");
	if (safewrite(fd, rptr, size) != size)
		report("subrec write err");
	f_unlock(fd);
	close(fd);
#ifdef POSTBUG
	if (size == sizeof(struct fileheader) && (id > 1) && ((id - 1) % 4 == 0))
		restorerecords(filename, size, id);
#endif
	return 0;
}

void
tmpfilename(filename, tmpfile, deleted)
char   *filename, *tmpfile, *deleted;
{
	char   *ptr, *delfname, *tmpfname;
	strcpy(tmpfile, filename);
	delfname = ".deleted";
	tmpfname = ".tmpfile";
	if ((ptr = strrchr(tmpfile, '/')) != NULL) {
		strcpy(ptr + 1, delfname);
		strcpy(deleted, tmpfile);
		strcpy(ptr + 1, tmpfname);
	} else {
		strcpy(deleted, delfname);
		strcpy(tmpfile, tmpfname);
	}
}

int
delete_record(filename, size, id)
char   *filename;
int     size, id;
{
	char    tmpfile[STRLEN], deleted[STRLEN];
	char    abuf[BUFSIZE];
	int     fdr, fdw, fd;
	int     count;
	if (size > BUFSIZE) {
		toobigmesg();
		return -1;
	}
	tmpfilename(filename, tmpfile, deleted);
	if ((fd = open(".dellock", O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
		return -1;
	f_exlock(fd);

	if ((fdr = open(filename, O_RDONLY, 0)) == -1) {
		report("delrec open err");
		f_unlock(fd);
		close(fd);
		return -1;
	}
	if ((fdw = open(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) == -1) {
		f_unlock(fd);
		report("delrec tmp err");
		close(fd);
		close(fdr);
		return -1;
	}
	count = 1;
	while (read(fdr, abuf, size) == size)
		if (id != count++ && (safewrite(fdw, abuf, size) == -1)) {
			unlink(tmpfile);
			close(fdr);
			close(fdw);
			report("delrec write err");
			f_unlock(fd);
			close(fd);
			return -1;
		}
	close(fdr);
	close(fdw);
	if (rename(filename, deleted) == -1 ||
		rename(tmpfile, filename) == -1) {
		f_unlock(fd);
		report("delrec rename err");
		close(fd);
		return -1;
	}
	f_unlock(fd);
	close(fd);
	return 0;
}

int
delete_range(filename, id1, id2)
char   *filename;
int     id1, id2;
{
	struct fileheader fhdr;
	char    tmpfile[STRLEN], deleted[STRLEN];
	int     fdr, fdw, fd;
	int     count;
	tmpfilename(filename, tmpfile, deleted);
	if ((fd = open(".dellock", O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
		return -1;
	f_exlock(fd);

	if ((fdr = open(filename, O_RDONLY, 0)) == -1) {
		f_unlock(fd);
		close(fd);
		return -1;
	}
	if ((fdw = open(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) == -1) {
		close(fdr);
		f_unlock(fd);
		close(fd);
		return -1;
	}
	count = 1;
	while (read(fdr, &fhdr, sizeof(fhdr)) == sizeof(fhdr)) {
		if (count < id1 || count > id2 || fhdr.accessed[0] & FILE_MARKED) {
			if ((safewrite(fdw, &fhdr, sizeof(fhdr)) == -1)) {
				unlink(tmpfile);
				close(fdw);
				close(fdr);
				f_unlock(fd);
				close(fd);
				return -1;
			}
		} else {
			char   *t;
			char    buf[STRLEN], fullpath[STRLEN];
			strcpy(buf, filename);
			if ((t = strrchr(buf, '/')) != NULL)
				*t = '\0';
			sprintf(fullpath, "%s/%s", buf, fhdr.filename);
			unlink(fullpath);
		}
		count++;
	}
	close(fdw);
	close(fdr);
	if (rename(filename, deleted) == -1) {
		f_unlock(fd);
		close(fd);
		return -1;
	}
	if (rename(tmpfile, filename) == -1) {
		f_unlock(fd);
		close(fd);
		return -1;
	}
	f_unlock(fd);
	close(fd);
	return 0;
}

int
update_file(dirname, size, ent, filecheck, fileupdate)
char   *dirname;
int     size, ent;
int     (*filecheck) ();
void    (*fileupdate) ();
{
	char    abuf[BUFSIZE];
	int     fd;
	if (size > BUFSIZE) {
		toobigmesg();
		return -1;
	}
	if ((fd = open(dirname, O_RDWR)) == -1)
		return -1;
	f_exlock(fd);
	if (lseek(fd, (off_t) (size * (ent - 1)), SEEK_SET) != -1) {
		if (read(fd, abuf, size) == size)
			if ((*filecheck) (abuf)) {
				lseek(fd, (off_t) (-size), SEEK_CUR);
				(*fileupdate) (abuf);
				if (safewrite(fd, abuf, size) != size) {
					report("update err");
					f_unlock(fd);
					close(fd);
					return -1;
				}
				f_unlock(fd);
				close(fd);
				return 0;
			}
	}
	lseek(fd, 0, SEEK_SET);
	while (read(fd, abuf, size) == size) {
		if ((*filecheck) (abuf)) {
			lseek(fd, (off_t) (-size), SEEK_CUR);
			(*fileupdate) (abuf);
			if (safewrite(fd, abuf, size) != size) {
				report("update err");
				f_unlock(fd);
				close(fd);
				return -1;
			}
			f_unlock(fd);
			close(fd);
			return 0;
		}
	}
	f_unlock(fd);
	close(fd);
	return -1;
}

int
delete_file(dirname, size, ent, filecheck)
char   *dirname;
int     size, ent;
int     (*filecheck) ();
{
	char    abuf[BUFSIZE];
	int     fd;
	struct stat st;
	long    numents;
	if (size > BUFSIZE) {
		toobigmesg();
		return -1;
	}
	if ((fd = open(dirname, O_RDWR)) == -1)
		return -1;
	f_exlock(fd);
	fstat(fd, &st);
	numents = ((long) st.st_size) / size;
	if (((long) st.st_size) % size != 0)
		fprintf(stderr, "align err\n");
	if (lseek(fd, (off_t) (size * (ent - 1)), SEEK_SET) != -1) {
		if (read(fd, abuf, size) == size)
			if ((*filecheck) (abuf)) {
				int     i;
				for (i = ent; i < numents; i++) {
					if (lseek(fd, (off_t) (i * size), SEEK_SET) == -1)
						break;
					if (read(fd, abuf, size) != size)
						break;
					if (lseek(fd, (off_t) ((i - 1) * size), SEEK_SET) == -1)
						break;
					if (safewrite(fd, abuf, size) != size)
						break;
				}
				ftruncate(fd, (off_t) size * (numents - 1));
				f_unlock(fd);
				close(fd);
				return 0;
			}
	}
	lseek(fd, 0, SEEK_SET);
	ent = 1;
	while (read(fd, abuf, size) == size) {
		if ((*filecheck) (abuf)) {
			int     i;
			for (i = ent; i < numents; i++) {
				if (lseek(fd, (off_t) ((i + 1) * size), SEEK_SET) == -1)
					break;
				if (read(fd, abuf, size) != size)
					break;
				if (lseek(fd, (off_t) (i * size), SEEK_SET) == -1)
					break;
				if (safewrite(fd, abuf, size) != size)
					break;
			}
			ftruncate(fd, (off_t) size * (numents - 1));
			f_unlock(fd);
			close(fd);
			return 0;
		}
		ent++;
	}
	f_unlock(fd);
	close(fd);
	return -1;
}
