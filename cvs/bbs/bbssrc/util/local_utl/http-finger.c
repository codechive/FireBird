/*

  功能     :  以 HTML 格式列出站內使用者資料
  注意事項 :  須要目前的 bbs.h

  Compile  :  gcc -o http-finger http-finger.c modetype.c

  Export   :
   1. 將 http-finger 執行檔拷貝去 /usr/local/sbin

   2. 修改 /etc/inetd.conf, 加入:

    hfinger stream tcp nowait bbs /usr/sbin/tcpd /usr/local/sbin/http-finger

   3. 再修改 /etc/services, 加入:

    hfinger         1992/tcp

   4. kill -1 `cat /var/run/inetd.pid`

*/
/* $Id: http-finger.c,v 1.1 2000/01/15 01:45:40 edwardc Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bbs.h"

#define MYUTMPFILE  BBSHOME"/.UTMP."BBSID
#define field_count 5

struct user_info aman;
char field_str[ field_count ][ 128 ];
int  field_lst_no [ field_count ];
int  user_num = 0;

int  field_lst_size [ field_count ] = {
   12, 20, 18, 15,  8
};

char *field_name[] = {
    "User ID", 
    "Nick name",
    "From", 
    "Mode",
    "Idle",
    NULL
};

print_head()
{
    int i, size;

    printf("<TD bgcolor=blue>No.\n");
    for (i = 0; i < field_count; i++) {
        size  = field_lst_size[ i ];
        printf("<TD bgcolor=blue>%-*.*s ", size, size, field_name[ i ] );
    }
    printf("<TR>\n");
}

print_record()
{
    int i, size;

    for (i = 0 ; i < field_count; i++) {
        size  = field_lst_size[ i ];
        if(i==0) 
           printf("<TD bgcolor=blue>%d<TD bgcolor=red><A HREF=\"mailto:%s.bbs@%s\">%-*.*s</A>",user_num+1, field_str[i], BBSHOST, size, size, field_str[i]);
        else printf("<TD bgcolor=black>%-*.*s ", size, size, field_str[ i ] );
    }
    printf("<TR>\n");
}

char *idle_str( tty )
char *tty;
{
    
    struct stat buf;
    static char hh_mm_ss[ 80 ];
    time_t now, diff;
    int    hh, mm ;

    if ( (stat( tty, &buf ) != 0) ||
         (strstr( tty, "tty" ) == NULL)) {
        strcpy( hh_mm_ss, "Unknown");
        return hh_mm_ss;
    };

    now = time( 0 );

    diff = now - buf.st_atime;

    hh = diff / 3600;
    mm = (diff / 60) % 60;

    if ( hh > 0 )
        sprintf( hh_mm_ss, "%d:%02d", hh, mm );
    else if ( mm > 0 )
        sprintf( hh_mm_ss, "%d", mm );
    else sprintf ( hh_mm_ss, "<PRE>   </PRE>");

    return hh_mm_ss;

}

dump_record(serial_no, p)
int serial_no;
struct user_info *p;
{
    int i = 0;

    sprintf( field_str[ i++ ], "%s", p->userid );
    sprintf( field_str[ i++ ], "%s", p->username );
    sprintf( field_str[ i++ ], "%s", p->from );
    sprintf( field_str[ i++ ], "%s", ModeType(p->mode) );
    sprintf( field_str[ i++ ], "%s", idle_str( p->tty ) );
}

main()
{
    FILE *inf;
    int  i;

    inf = fopen( MYUTMPFILE, "rb" );
    if (inf == NULL) {
        printf("Error open %s\n", MYUTMPFILE); 
        exit( 0 );
    }
printf("<HTML><HEAD><TITLE>Online users at %s</TITLE></HEAD><BODY bgcolor=#97694f text=#ffff00 link=white>\n", BBSNAME);
printf("<CENTER><TABLE BORDER=1 bgcolor=\"#ebc79e\"><TD><FONT SIZE=5 COLOR=red>ONLINE USERS AT %s</FONT><TR>\n", BBSNAME);
printf("<TD><CENTER><FONT SIZE=5 COLOR=red>%s目前線上使用者狀態一覽</FONT></CENTER>\n", BBSNAME);
printf("</TABLE><P><HR>\n");
printf("<TABLE border=1>");
    print_head(); 
    for (i=0; ; i++) {
        if (fread(&aman, sizeof( aman ), 1, inf ) <= 0) break;
        if (aman.active && !aman.invisible ) {
            dump_record(i, &aman);    
            print_record();
            user_num++;
        }
    }
    fclose( inf );

    printf("<TD COLSPAN=6 BGCOLOR=#7fff00><FONT COLOR=black><CENTER>*** Total [non-cloak] online users = %d ***</CENTER></FONT><TR></TABLE>\n", user_num );
    printf("<P><HR><A href=\"telnet://%s\">",BBSHOST);
    printf("Login to the %s</a>\n", BBSNAME);
    printf("<BR><A href=\"mailto:SYSOP.bbs@%s\">",BBSHOST);
    printf("Send comment/suggestion to SYSOP</a><HR></CENTER></BODY></HTML>\n");
}
