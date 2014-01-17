/*-------------------------------------------------------*/
/* struct.h	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : all definitions about data structure	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/
/* $Id: struct.M2.h,v 1.1 2000/01/15 01:45:43 edwardc Exp $ */

#ifndef _STRUCT_H_
#define _STRUCT_H_


#define STRLEN   80		/* Length of most string data */
#define BTLEN    48		/* Length of board title */
#define TTLEN    72		/* Length of title */
#define NAMELEN  40		/* Length of username/realname */
#define FNLEN    33		/* Length of filename  */
#define IDLEN	 12		/* Length of bid/uid */
#define PASSLEN  14		/* Length of encrypted passwd field */


typedef unsigned char uschar;	/* length = 1 */
typedef unsigned int usint;	/* length = 4 */


/* ----------------------------------------------------- */
/* .PASSWDS struct : 256 bytes				 */
/* ----------------------------------------------------- */

struct userec
{
  char userid[IDLEN + 1];
  char realname[20];
  char username[24];
  char passwd[PASSLEN];
  uschar uflag;
  usint userlevel;
  ushort numlogins;
  ushort numposts;
  time_t firstlogin;
  time_t lastlogin;
  char lasthost[16];
  char termtype[8];
  char email[50];
  char address[50];
  char justify[44];
};
typedef struct userec userec;

/* these are flags in userec.uflag */
/* edwardc.990529 I dont want to implement flags right now.
   it doesn't take any extra effects between M2 and FB3 */
#if 0
#define SIG_FLAG	0x3	/* signature number, 2 bits */
#define PAGER_FLAG	0x4	/* true if pager was OFF last session */
#define CLOAK_FLAG	0x8	/* true if cloak was ON last session */
#define FRIEND_FLAG	0x10	/* true if show friends only */
#define BRDSORT_FLAG	0x20	/* true if the boards sorted alphabetical */
#define	MOVIE_FLAG	0x40	/* true if show movie */
#define	COLOR_FLAG	0x80	/* true if the color mode open */
#endif

/* ----------------------------------------------------- */
/* DIR of board struct : 128 bytes			 */
/* ----------------------------------------------------- */

struct fileheader
{
  char filename[FNLEN];		/* M.9876543210.A */
  char savemode;		/* file save mode */
  char owner[IDLEN + 2];	/* uid[.] */
  char date[6];			/* [02/02] or space(5) */
  char title[TTLEN + 1];
  uschar filemode;		/* must be last field @ boards.c */
};
typedef struct fileheader fileheader;

/* edwardc.990529 reverse for trans board ..... */
#if 0 
#define FILE_LOCAL	0x1	/* local saved */
#define FILE_READ	0x1	/* already read : mail only */
#define FILE_MARKED	0x2	/* opus: 0x8 */
#define FILE_DIGEST	0x4	/* digest */
#endif

/* ----------------------------------------------------- */
/* Menu Commands struct					 */
/* ----------------------------------------------------- */

struct commands
{
  int (*cmdfunc) ();
  int level;
  char *desc;			/* next/key/description */
};
typedef struct commands commands;


/* ----------------------------------------------------- */
/* Structure used in UTMP file : 128 bytes		 */
/* ----------------------------------------------------- */

struct user_info
{
  int uid;			/* Used to find user name in passwd file */
  pid_t pid;			/* kill() to notify user of talk request */
  int sockaddr;			/* ... */
  int destuid;			/* talk uses this to identify who called */
  uschar active;		/* When allocated this field is true */
  uschar invisible;		/* Used by cloaking function in Xyz menu */
  uschar sockactive;		/* Used to coordinate talk requests */
  uschar mode;			/* UL/DL, Talk Mode, Chat Mode, ... */
  uschar pager;			/* pager toggle, YEA, or NA */
  uschar in_chat;		/* for in_chat commands   */
  char userid[IDLEN + 1];
  char chatid[9];		/* chat id, if in chat mode */
  char realname[20];
  char username[24];
  char from[29];		/* machine name the user called in from */
  char tty[11];			/* tty port */
  ushort friend[MAXFRIENDS];
  ushort reject[MAXREJECTS];
};
typedef struct user_info user_info;


/* ----------------------------------------------------- */
/* BOARDS struct : 128 bytes				 */
/* ----------------------------------------------------- */

struct boardheader
{
  char brdname[IDLEN + 1];	/* bid */
  char title[BTLEN + 1];
  char BM[IDLEN * 3 + 3];	/* BMs' uid, token '/' */
  char pad[11];
  time_t bupdate;		/* note update time */
  char pad2[3];
  uschar bvote;			/* Vote flags */
  time_t vtime;			/* Vote close time */
  usint level;
};
typedef struct boardheader boardheader;


struct one_key
{				/* Used to pass commands to the readmenu */
  int key;
  int (*fptr) ();
};


/* ----------------------------------------------------- */
/* cache.c 中運用的資料結構				 */
/* ----------------------------------------------------- */


#define USHM_SIZE       (MAXACTIVE + 4)
struct UTMPFILE
{
  user_info uinfo[USHM_SIZE];
  time_t uptime;
  int number;
  int busystate;
};

struct BCACHE
{
  boardheader bcache[MAXBOARD];
  time_t uptime;
  time_t touchtime;
  int number;
  int busystate;
};

struct UCACHE
{
  char userid[MAXUSERS][IDLEN + 1];
  time_t uptime;
  time_t touchtime;
  int number;
  int busystate;
};


/* ----------------------------------------------------- */
/* screen.c 中運用的資料結構				 */
/* ----------------------------------------------------- */

#define ANSILINELEN (160)	/* Maximum Screen width in chars */

/* Line buffer modes */
#define MODIFIED (1)		/* if line has been modifed, screen output */
#define STANDOUT (2)		/* if this line has a standout region */

struct screenline
{
  uschar oldlen;		/* previous line length */
  uschar len;			/* current length of line */
  uschar mode;			/* status of line, as far as update */
  uschar smod;			/* start of modified data */
  uschar emod;			/* end of modified data */
  uschar sso;			/* start stand out */
  uschar eso;			/* end stand out */
  unsigned char data[ANSILINELEN + 1];
};
typedef struct screenline screenline;


/* ----------------------------------------------------- */
/* name.c 中運用的資料結構				 */
/* ----------------------------------------------------- */

struct word
{
  char *word;
  struct word *next;
};


/* ----------------------------------------------------- */
/* edit.c 中運用的資料結構				 */
/* ----------------------------------------------------- */

#define WRAPMARGIN (159)

struct textline
{
  struct textline *prev;
  struct textline *next;
  int len;
  char data[WRAPMARGIN + 1];
};
typedef struct textline textline;

#endif				/* _STRUCT_H_ */
