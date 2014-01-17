/* $Id: usage.c,v 1.5 2002/06/12 12:37:37 edwardc Exp $ */

#include <time.h>
#include <stdio.h>
#include "bbs.h"
#include "config.h"

#define BBSBOARDS BBSHOME"/.BOARDS"

struct binfo
{
  char  boardname[18];
  char  expname[28];
  int times;
  int sum;
} st[MAXBOARD];

int numboards=0;

int 
brd_cmp(b, a)
struct binfo *a, *b;
{
    if(a->times!=b->times)
            return (a->times - b->times);
    return a->sum-b->sum;
}

int report()
{
        return ;
}

char *
	Ctime(clock)
	time_t *clock;
{
	static char chinese[STRLEN];
	static char wday[7][3] = {"¤é", "¤@", "¤G", "¤T", "¥|", "¤­", "¤»"};
	struct tm *t = localtime(clock);
	sprintf(chinese, "%4d/%.2d/%.2d (%s) %.2d:%.2d",
	t->tm_year+1900, t->tm_mon + 1, t->tm_mday, wday[t->tm_wday],
	t->tm_hour, t->tm_min);
	return (chinese);
}

int
record_data(board,sec)
char *board;
int sec;
{
        int i;
        for(i=0;i<numboards;i++)
        {
                if(!strcmp(st[i].boardname,board))
                {
                        st[i].times++;
                        st[i].sum+=sec;
                        return;
                }
        }
        return ;
}

int
fillbcache(fptr)
struct boardheader *fptr ;
{

    if( numboards >= MAXBOARD )
        return 0;
    if(((fptr->level!=0)&&!(fptr->level&PERM_NOZAP||fptr->level&PERM_POSTMASK))||strlen(fptr->filename)==0)
        return;
    strcpy(st[numboards].boardname,fptr->filename);
    strcpy(st[numboards].expname,fptr->title+8);
/*    printf("%s %s\n",st[numboards].boardname,st[numboards].expname);*/
    st[numboards].times=0;
    st[numboards].sum=0;
    numboards++;
    return 0 ;
}

int 
fillboard()
{
        apply_record( BBSBOARDS, fillbcache, sizeof(struct boardheader) );
}

char *
timetostr(i)
int i;
{
        static char str[30];
        int minute,sec,hour;

        minute=(i/60);
        hour=minute/60;
        minute=minute%60;
        sec=i&60;
        sprintf(str,"%2d:%2d:%2d",hour,minute,sec);
        return str;
}

main(argc, argv)
char *argv[];
{
  FILE *fp;
  FILE *op;
  char buf[256], *p,bname[20];
  char date[80];
  int mode;
  int c[3];
  int max[3];
  unsigned int ave[3];
  int now,sec;
  int i, j,k;
  char    *blk[10] =
  {
      "  ","¢j", "¢k", "¢l", "¢m",
      "¢n","¢o", "¢p", "¢i", "¢i",
  };

  if( argc !=2 ){
	printf("usage : %s <mode>\n",argv[0]);
	printf("mode : 1:text  2:graphic\n");
	return 0;
  }
  mode=atoi(argv[1]);

  sprintf(buf,"%s/log/use_board", BBSHOME);
  
  if ((fp = fopen(buf, "r")) == NULL)
  {
    printf("cann't open use_board\n");
    return 1;
  }

  sprintf(buf,"%s/0Announce/bbslist/board%d", BBSHOME, ( mode == 1 ) ? 1 : 2);

  if ((op = fopen(buf, "w")) == NULL)
  {
    printf("Can't Write file\n");
    return 1;
  }

  fillboard();
  now=time(0);
  sprintf(date,"%s",Ctime(&now));
  while (fgets(buf, 256, fp))
  {
   if(strlen(buf)<57)
       continue;
    if ( !strncmp(buf+22, "USE", 3))
    {
      p=strstr(buf,"USE");
      p+=4;
      p=strtok(p," ");
      strcpy(bname,p);
      if ( p = (char *)strstr(buf+47, "Stay: "))
      {
        sec=atoi( p + 6);
      }
      else
          sec=0;
      record_data(bname,sec);
    }
   }
   fclose(fp);
   qsort(st, numboards, sizeof( st[0] ), brd_cmp);
   ave[0]=0;
   ave[1]=0;
   ave[2]=0;
   max[1]=0;
   max[0]=0;
   max[2]=0;
   for(i=0;i<numboards;i++)
   {
        ave[0]+=st[i].times;
        ave[1]+=st[i].sum;
        ave[2]+=st[i].times==0?0:st[i].sum/st[i].times;
        if(max[0]<st[i].times)
        {
                max[0]=st[i].times;
        }
        if(max[1]<st[i].sum)
        {
                max[1]=st[i].sum;
        }
        if(max[2]<(st[i].times==0?0:st[i].sum/st[i].times))
        {
                max[2]=(st[i].times==0?0:st[i].sum/st[i].times);
        }
   }
   c[0]=max[0]/30+1;
   c[1]=max[1]/30+1;
   c[2]=max[2]/30+1;
   numboards++;
   st[numboards-1].times=ave[0]/numboards;
   st[numboards-1].sum=ave[1]/numboards;
   strcpy(st[numboards-1].boardname,"Average");
   strcpy(st[numboards-1].expname,"Á`¥­§¡");
   if(mode==1)
   {
        fprintf(op,"[1;37m¦W¦¸ %-15.15s%-28.28s %5s %8s %10s[m\n","°Q½×°Ï¦WºÙ","¤¤¤å±Ô­z","¤H¦¸","²Ö¿n®É¶¡","¥­§¡®É¶¡");
   }else
   {
        fprintf(op,"      [1;37m1 [m[34m%2s[1;37m= %d (Á`¤H¦¸) [1;37m1 [m[32m%2s[1;37m= %s (²Ö¿nÁ`®É¼Æ) [1;37m1 [m[31m%2s[1;37m= %d ¬í(¥­§¡®É¼Æ)\n\n",
                blk[9],c[0],blk[9],timetostr(c[1]),blk[9],c[2]);
   }

   for(i=0;i<numboards;i++)
   {
      if(mode==1)
        fprintf(op,"[1m%4d[m %-15.15s%-28.28s %5d %-.8s %10d\n",i+1,st[i].boardname,st[i].expname,st[i].times,timetostr(st[i].sum),st[i].times==0?0:st[i].sum/st[i].times);
      else
      {
        fprintf(op,"      [1;37m²Ä[31m%3d [37m¦W °Q½×°Ï¦WºÙ¡G[31m%s [35m%s[m\n",i+1,st[i].boardname,st[i].expname);
        fprintf(op,"[1;37m    ¢z¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w\n");
        fprintf(op,"[1;37m¤H¦¸¢x[m[34m");
        for(j=0;j<st[i].times/c[0];j++)
        {
                fprintf(op,"%2s",blk[9]);
        }
                fprintf(op,"%2s [1;37m%d[m\n",blk[(st[i].times%c[0])*10/c[0]],st[i].times);
        fprintf(op,"[1;37m®É¶¡¢x[m[32m");
        for(j=0;j<st[i].sum/c[1];j++)
        {
                fprintf(op,"%2s",blk[9]);
        }
                fprintf(op,"%2s [1;37m%s[m\n",blk[(st[i].sum%c[1])*10/c[1]],timetostr(st[i].sum));
        j=st[i].times==0?0:st[i].sum/st[i].times;
        fprintf(op,"[1;37m¥­§¡¢x[m[31m");
        for(k=0;k<j/c[2];k++)
        {
                fprintf(op,"%2s",blk[9]);
        }
                fprintf(op,"%2s [1;37m%s[m\n",blk[(j%c[2])*10/c[2]],timetostr(j));
        fprintf(op,"[1;37m    ¢|¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w[m\n\n");
      }
   }
   fclose(op);
}
