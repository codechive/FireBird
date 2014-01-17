/*
 * functions.h		-- include file for define functions individual
 *	
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
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
 * CVS: $Id: functions.h,v 1.11 2002/09/11 09:59:00 edwardc Exp $
 */

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#define RNDSIGN
/* 亂數簽名檔 */

//#define MAGIC_PASS
/* 站長級 ID 上站詢問 Magic Pass */

#define REFUSE_LESS60SEC
/* 60 秒內不可重覆 login */

#define TALK_LOG
/* 聊天紀錄功能 */

#define CODE_VALID
/* 暗碼認證 */

//#define MSG_CANCLE_BY_CTRL_C
/* 用 ctrl-c 來否略訊息 */

#define LOG_MY_MESG
/* 訊息紀錄中紀錄自己所發出的訊息 */

#define COLOR_POST_DATE
/* 文章日期顏色 */

#define BIGGER_MOVIE
/* 加大活動看板空間 (七行) */

#define ALWAYS_SHOW_BRDNOTE
/* 每次進板都會 show 出進板畫面 */

#define BBSD
/* 使用 BBS daemon, 關掉則可使用 in.zbbsd, telnetd 或使用 bbsrf */

//#define MUDCHECK_BEFORELOGIN
/* 類似某些 MUD server 會要求使用者登入之前按鍵來確定不是使用程式來連線
   Ctrl + C 有時可改為 Ctrl + D, Ctrl + Z .. */

#define DLM
/* .SO 的支援, 將許多不常用的功能包成 .so 檔, 使用動態連結方式呼叫 */

#define NO_DELETED_BOARD
/* 隱藏刪除過的版面資訊 */

#define DISABLE_RFC931
/* 停止要求 identd reponse, 以免遭 WinXP 或其他防火牆產生中斷連線 */

#define Quick_LOGIN
/* 若在輸入帳號處, 直接按 <Enter> 則直接視為使用 guest 上站 */

#define ANTI_HANG_ON_SITE
/* 設定防止掛站 */

#define ANONYBOARD_POST_DEFAULT_ANONY
/* 匿名版要post的時候, 則預設"要"匿名 */

//#define Fuzzy_MENU
/* 設定進入選單功能時不需要另外在按下 enter */

#define USE_CHINESE_DATE
/* 使用中文日期 (buggy!! dont enable!) */

#define FROM_TRANSLATE
/* 中文上站來源 */

#endif /* _FUNCTIONS_H_ */
