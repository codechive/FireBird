#include        <stdio.h>
#include        <sys/types.h>
#include        <sys/stat.h>
#include        <dirent.h>
#include        <limits.h>
#include        "bbs.h"

#ifndef BBSHOME
  #define BBSHOME "/home/bbs"
#endif

#define TRUE  1
#define FALSE 0

int     fileflag = 1;
int     totalfound = 0, totalfile = 0;
char    control[80];

struct postnode {
	char    filename[20];
	int     num;
};
report()
{
}
main(argc, argv)
int     argc;
char   *argv[];
{
	char    dir[80];
	if (argc < 2) {
		printf("Usage: %s <BoardName>\n", argv[0]);
		exit(1);
	}
	if (argv[2] == '\0')
		sprintf(dir, "%s/boards/%s", BBSHOME, argv[1]);
	else
		sprintf(dir, "%s/mail/%c/%s", BBSHOME, toupper(argv[1][0]), argv[1]);
	myftw(dir);
}
do_remake(path, file)
char   *path, *file;
{
	FILE   *fp;
	char   *ptr, *ptr2;
	char    filename[80];
	char    buf[256];
	struct fileheader fh;
	int     step = 0;
	sprintf(filename, "%s/%s", path, file);
	if ((fp = fopen(filename, "r")) == NULL) {
		printf("Open error.. \n");
		return;
	}
	strncpy(fh.filename, file, sizeof(fh.filename));
	fh.level = 0;
	memset(&fh.accessed, 0, sizeof(fh.accessed));
	while (fgets(buf, 256, fp) != NULL) {
		if (strstr(buf, "發信人: ") || strstr(buf, "作  者: ") || strstr(buf, "寄信人: ")) {
			ptr = &buf[8];
			ptr2 = strchr(ptr, ' ');
			if (ptr2 != NULL) {
				*ptr2 = '\0';
			}
			ptr2 = strchr(ptr, '@');
			if (ptr2 != NULL) {
				*ptr2 = '\0';
			}
			ptr2 = strchr(ptr, '.');
			if (ptr2 != NULL) {
				*(ptr2 + 1) = '\0';
			}
			strncpy(fh.owner, ptr, sizeof(fh.owner));
			step = 1;
		}
		if (strstr(buf, "標  題: ") || strstr(buf, "題  目: ")) {
			ptr = &buf[8];
			ptr[strlen(ptr) - 1] = 0;
			strncpy(fh.title, ptr, sizeof(fh.title));
			step = 2;
		}
		if (step == 2)
			break;
	}
	fclose(fp);
	if (step == 2) {
		fh.filename[STRLEN - 1] = 'S';
		fh.filename[STRLEN - 2] = 'S';
		append_record(control, &fh, sizeof(fh));
		return 1;
	} else {
		unlink(filename);
		return 0;
	}
}

int
cmpfname(brd, tmp)
struct postnode *brd, *tmp;
{
	/* SuperD.20011010: 解決原本重新排序時會把檔名較小的都一律當成舊的來處理.. */
	if (strlen(brd->filename) != strlen(tmp->filename))
		return (strlen(brd->filename) > strlen(tmp->filename)) ? 
			1 : -1;
	else
		return strcmp(brd->filename, tmp->filename);
}

int
do_sort(pn)
char   *pn;
{
	struct postnode *allnode;
	struct fileheader post;
	char    sfname[STRLEN];
	int     i = 0;
	int     n, total;
	FILE   *tf;
	total = get_num_records(control, sizeof(struct fileheader));
	allnode = (struct postnode *) malloc(sizeof(struct postnode) * total);

	if ((tf = fopen(control, "rb")) == NULL) {
		printf(".DIR cant open...");
		return;
	}
	while (1) {
		if (fread(&post, sizeof(post), 1, tf) <= 0)
			break;
		allnode[i].num = i + 1;
		strncpy(allnode[i].filename, post.filename, 19);
		i++;
	}
	fclose(tf);
	qsort(allnode, i, sizeof(struct postnode), cmpfname);
	sprintf(sfname, "%s/.DIR.sort", pn);
	for (n = 0; n < total; n++) {
		get_record(control, &post, sizeof(post), allnode[n].num);
		append_record(sfname, &post, sizeof(post));
	}
	rename(sfname, control);
	free(allnode);
}

int 
myftw(pathname)
char   *pathname;
{
	DIR    *dp;
	char    buf[80];
	struct dirent *dirp;
	int     all = 0, done = 0;
	printf("1. 進入目錄 %s\n", pathname);
	if ((dp = opendir(pathname)) == NULL) {
		printf("OpenDir error for %s\n", pathname);
		return;
	}
	sprintf(control, "%s/.DIR", pathname);
	sprintf(buf, "%s.bak", control);
	rename(control, buf);
	printf("2. 整理文章，建立 .DIR\n");
	while ((dirp = readdir(dp)) != NULL) {
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..") || dirp->d_name[0] != 'M')
			continue;
		done += do_remake(pathname, dirp->d_name);
		all++;
	}
	printf("3. 排序文章\n");
	do_sort(pathname);
	printf("%d 篇文章重建，%d 文章失敗，已經刪除\n", done, all - done);
	chown(control, 9999, 99);
}
