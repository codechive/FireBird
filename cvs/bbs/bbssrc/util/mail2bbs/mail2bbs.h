/* mail2bbs.h for mail2bbs configuring */
/* (c)Copyleft 2000 by Firebird BBS Project */
/* $Id: mail2bbs.h,v 1.3 2000/07/21 04:12:55 edwardc Exp $ */

#ifndef _MAIL2BBS_H_
#define _MAIL2BBS_H_

/* mail2bbs 特殊功能 */

/* 不產生任何退信: 非必要不建議使用, 因為將幾乎無法 debug .. */

#define DONT_RETURN_ANYTHING	

/* 將以下的註解移除以開啟 mailing list to bbs 功能 */

//#define MAILLIST_MODULE			
//#define MAILLIST_RECEIVER	"maillist"		/* 收信者 XXXXXX.bbs@YOUR.BBS.HOST */
//#define MAILLIST_RETURNBRD "junk"			/* 退信的版面 */

/* mail2bbs anti-spam 功能 (by: rexchen) */

#define RULE  3    /* rule 2 .... 10 (不建議超過 10 ) */
#define SPAM_SHMKEY		(31000L)
#define SPAMTABLE		2048

struct SPAM
{
	int spam_a;
	int spam_b;
	int spam_c;
	int spam_times;
};
                                
struct SPAM_MAIL
{
	struct SPAM mail_flag[SPAMTABLE];
	time_t update_time;
};
                                                
#endif/*_MAIL2BBS_H_*/
