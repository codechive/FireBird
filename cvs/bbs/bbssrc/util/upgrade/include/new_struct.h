/*-
 *	new_struct.h		-- for util/upgrade
 *  upgrade your older version to Firebird BBS 3.0
 *
 *  $Id: new_struct.h,v 1.1 2000/01/15 01:45:43 edwardc Exp $
 */

#ifndef _NEW_STRUCT_H_
#define _NEW_STRUCT_H_

struct newuserec {                  /* Structure used to hold information in */
        char            userid[IDLEN+2];   /* PASSFILE */
        time_t          firstlogin;
        char            lasthost[16];
        unsigned int    numlogins;
        unsigned int    numposts;
        char            flags[2];
		char			passwd[PASSLEN];	/* mmmm.. translate just using DES */
        char            username[NAMELEN];
        char            ident[NAMELEN];
        char            termtype[16];
        char            reginfo[STRLEN-16];
        unsigned int    userlevel;
        time_t          lastlogin;
        time_t          stay;
        char            realname[NAMELEN];
        char            address[STRLEN];
        char            email[STRLEN-12];
        unsigned int    nummails;
        time_t          lastjustify;
        char            gender;
        unsigned char   birthyear;
        unsigned char   birthmonth;
        unsigned char   birthday;
        int             signature;
        unsigned int    userdefine;
        time_t          notedate;
        int             noteline;
};
 
struct newoverride {
        char id[13];
        char exp[40];
};


#define BM_LEN 60

struct newboardheader {             /* This structure is used to hold data in */
        char filename[STRLEN];   /* the BOARDS files */
        char owner[STRLEN - BM_LEN];
        char BM[ BM_LEN - 1];
        char flag;
        char title[STRLEN ];
        unsigned level;
        unsigned char accessed[ 12 ];
};

struct newfileheader {             /* This structure is used to hold data in */
        char filename[STRLEN];     /* the DIR files */
        char owner[STRLEN];
        char title[STRLEN];
        unsigned level;
        unsigned char accessed[ 12 ];   /* struct size = 256 bytes */
} ;

struct newshortfile {               /* used for caching files and boards */
        char filename[STRLEN];      /* also will do for mail directories */
        char owner[STRLEN - BM_LEN];
        char BM[ BM_LEN - 1];
        char flag;
        char title[STRLEN];
        unsigned level;
        unsigned char accessed;
};

#endif
