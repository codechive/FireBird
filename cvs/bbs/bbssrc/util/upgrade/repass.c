/*
 * repass.c             -- translate older .PASSWDS format to Firebird 3.0
 *
 * A part of Firebird BBS 3.0 utility kit
 *
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@firebird.dhs.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * CVS: $Id: repass.c,v 1.2 2000/11/14 15:33:33 edwardc Exp $
 */

#define NO_STRUCT_H             /* do not mess struct.h with old_struct.h */
#include "bbs.h"
#undef NO_STRUCT_H

#include "new_struct.h" /* new structure defines .. */

#ifdef STRUCT_VERSION
  #include "struct."STRUCT_VERSION".h"
#else
  #error "No STRUCT_VERSION defined .."
#endif

typedef enum _types { FB266M , FB262M , FB25 , unknown , MAPLE_2 , PH4 } types;
struct userec old;
struct newuserec new;

//#define X86_TO_NON_X86	/* ex: 從 Pentium 轉換到 Sparc */
//#define NON_X86_TO_X86	/* ex: 從 Sparc 轉換到 Pentium */

/* 如果是 x86 轉換到 x86, Ex: Linux/ix86 -> FreeBSD/i386 或
 * 非 x86 轉換到 非 x86, Ex: Sparc -> HP-UX 
 * 則以上兩個 define 都不須打開 
 *
 ***** note ***** 必須要在 x86 平台上轉換, 不論你要轉至何種平台
 */
   
#define RESET_USERDEFINE

/* 因為 FB3 在 userdefine 方面做了一些部份的調整, 所以使用者的 define
   有必要重新設定, 把此項 comment 起來則不重設, 對於升級使用者而言, 這樣
   也許會造成使用者的問題 */

/* The following code is contributed by ANFORNEE.bbs@redbbs.cc.ntut.edu.tw */
/* It's worked for big-endian to little-endian translation */

#define A1      0xff000000
#define A2      0x00ff0000
#define A3      0x0000ff00
#define A4      0x000000ff

unsigned int
convert(unsigned int src)
{ 
	unsigned int dst;
	unsigned int tmp1,tmp2,tmp3,tmp4;
	
	tmp1 = (src << 3*8) & A1;
	tmp2 = (src << 8) & A2;
	tmp3 = (src >> 8) & A3;
	tmp4 = (src >> 3*8) & A4;
	dst = (tmp1 | tmp2 | tmp3 | tmp4);
	
	return dst;
}

#if defined(X86_TO_NON_X86) || defined(NON_X86_TO_X86)
unsigned int
endian(unsigned int x)
{
#ifdef X86_TO_NON_X86
	return ntohl(x);
#else
	return convert(x);
#endif
}    
#endif

int
doit(types type)
{
        int i, j;
        FILE *src, *dst;

        src = fopen(".PASSWDS", "rb");
        dst = fopen(".PASSWDS.new", "wb");

        j = 1;
        for ( i = 0 ; ; i++ ) {

                if ( fread(&old,sizeof(old),1,src) <= 0 )
                        break;
                if ( strlen(old.userid) <= 0 )
                        continue;       /* drop out! */

                printf("Processing #%d userid: %s\n", i+1, old.userid);
                memset( &new, 0, sizeof( new ) );
                memcpy( new.userid, old.userid, sizeof(old.userid));
                memcpy( &new.lasthost, &old.lasthost, sizeof(old.lasthost));                
                memcpy( &new.passwd, &old.passwd, sizeof(old.passwd));
                memcpy( &new.username, &old.username, sizeof(old.username));
                memcpy( &new.termtype, &old.termtype, sizeof(old.termtype));
/* **** */
                memcpy( &new.reginfo, &old.termtype+16, sizeof(old.termtype+16));
/* **** */
                memcpy( &new.realname, &old.realname, sizeof(old.realname));
                memcpy( &new.address, &old.address, sizeof(old.address));
                memcpy( &new.email, &old.email, sizeof(old.email));

                new.nummails = 0;       /* 上站會自動更新信件篇數 */

/* **** lastjustify 使用 firstlogin, 這樣會強迫一些老使用者進行重新認證 **** */
                memcpy( &new.lastjustify, &old.firstlogin, sizeof(old.firstlogin));

#ifdef _MAPLE_2
				new.flags[0] = CURSOR_FLAG | PAGER_FLAG;
				new.flags[1] = 0;
				new.ident[0] = '\0';
				new.signature = 0;
				new.notedate = time(0);
				new.noteline = 0;
				new.userdefine &=~(DEF_FRIENDCALL | DEF_ALLMSG | DEF_FRIENDMSG);
#else				
                memcpy( &new.ident, &old.ident, sizeof(old.ident));
               	memcpy( &new.flags, &old.flags, sizeof(old.flags));
                memcpy( &new.signature, &old.signature, sizeof(old.signature));
	#ifdef RESET_USERDEFINE
				new.userdefine =~0;
	#else
                memcpy( &new.userdefine, &old.userdefine, sizeof(old.userdefine));
	#endif                
                memcpy( &new.notedate, &old.notedate, sizeof(old.notedate));
                memcpy( &new.noteline, &old.noteline, sizeof(old.noteline));
#endif                

#if defined(X86_TO_NON_X86) || defined(NON_X86_TO_X86)
				new.userlevel = endian(old.userlevel);
				new.lastlogin = endian(old.lastlogin);
				new.firstlogin = endian(old.firstlogin);
				new.numlogins = endian(old.numlogins);
				new.numposts = endian(old.numposts);
  #ifdef _MAPLE_2
				new.stay = 0;
  #else				
				new.stay = endian(old.stay);
  #endif				
#else
                memcpy( &new.numlogins, &old.numlogins, sizeof(old.numlogins));
                memcpy( &new.numposts, &old.numposts, sizeof(old.numposts));
                memcpy( &new.firstlogin, &old.firstlogin, sizeof(old.firstlogin));
                memcpy( &new.lastlogin, &old.lastlogin, sizeof(old.lastlogin));
  #ifdef _MAPLE_2
				new.stay = 0;
				if ( strcasecmp(old.userid, "guest") )
					new.userlevel = PERM_BASIC;
				else
					new.userlevel = PERM_DEFAULT;
  #else			                
                memcpy( &new.userlevel, &old.userlevel, sizeof(old.userlevel));
                memcpy( &new.stay, &old.stay, sizeof(old.stay));
  #endif
#endif                

/* **** 3.0 新增的欄位, 性別, 出生年月日 **** */
                new.gender = 'X';
                new.birthyear = 0;
                new.birthmonth = 0;
                new.birthday = 0;

#ifdef _25CCU
                if(old.notemode != 3)
                        new.userdefine |= DEF_NOTEPAD;
                else
                        new.userdefine &=~ DEF_NOTEPAD;
#endif

                printf("  writing, sizeof(old)=%d sizeof(new)=%d ..",
                                sizeof(old), sizeof(new));

                if ( !fwrite(&new, sizeof(new), 1, dst) ) {
                        printf("failed!!\n");
                        exit(0);
                } else {
                        printf("done.\n");
                }

                j++;
        }

        fclose(src);
        fclose(dst);

        printf("done. total %d of %d users transfeerd\n",j ,i+1);
        printf("remember clean shm before replace new PASSWDS file!\n");
        return 0;
}

int
main(int argc, char **argv)
{
        types tp;

        if ( argc != 2 ) {
                printf("Error(2), you must select a target.\n");
                printf("Ex:\n");
                printf("   %s [ 2.66M | 2.62M | 2.5 | Maple2 | PH4 | unknown ] \n", argv[0]);
                exit(2);
        }

        if ( !dashf(".PASSWDS") ) {
                printf("Error(3), you must put your old .PASSWDS into this directory.\n");
                exit(3);
        } else {
                f_cp(".PASSWDS", ".PASSWDS.backup", O_TRUNC);
        }

        if ( strcmp(argv[1], "2.66M") ) {
            printf("Running 2.66M . 3.0 translation procedure.\n");
            printf(".PASSWDS.backup is backup for .PASSWDS\n");
            tp = FB266M;
        } else if ( strcmp(argv[1], "2.62M") ) {
            printf("Running 2.62M . 3.0 translation procedure.\n");
            printf(".PASSWDS.backup is backup for .PASSWDS\n");
            tp = FB262M;
        } else if ( strcmp(argv[1], "2.5") ) {
            printf("Running 2.5 (CCU) . 3.0 translation procedure.\n");
            printf(".PASSWDS.backup is backup for .PASSWDS\n");
            tp = FB25;
        } else if ( strcmp(argv[1], "unknown") ) {
            printf("Running 3.0 translation procedure from unknown .\n");
            printf(".PASSWDS.backup is backup for .PASSWDS\n");
            tp = unknown;
        } else if ( strcmp(argv[1], "Maple2") ) {
        	printf("Running Maple 2.36 translation procedude. \n");
        	printf(".PASSWDS.backup is backup for origin .PASSWDS\n");
        	tp = MAPLE_2;
        } else if ( strcmp(argv[1], "PH4") ) {
        	printf("Running translation from Phoenix 4 series. \n");
        	printf(".PASSWDS.backup is backup for origin .PASSWDS\n");
        	tp = PH4;
        } else {
                printf("Your target is not support yet.\n");
                exit(1);
        }

        return doit(tp);
}
