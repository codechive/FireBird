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
$Id: modetype.c,v 1.1 2000/01/15 01:45:28 edwardc Exp $
*/

#include "modes.h"

char   *
ModeType(mode)
int     mode;
{
	switch (mode) {
	case IDLE:
		return "";
	case NEW:
		return "新站友註冊";
	case LOGIN:
		return "進入本站";
	case DIGEST:
		return "瀏覽精華區";
	case MMENU:
		return "主選單";
	case ADMIN:
		return "管理者選單";
	case SELECT:
		return "選擇討論區";
	case READBRD:
		return "覽遍天下";
	case READNEW:
		return "覽新文章";
	case READING:
		return "品味文章";
	case POSTING:
		return "發表文章";
	case MAIL:
		return "處理信箋";
	case SMAIL:
		return "寄語信鴿";
	case RMAIL:
		return "閱\覽信箋";
	case TMENU:
		return "聊天選單";
	case LUSERS:
		return "環顧四方";
	case FRIEND:
		return "尋找好友";
	case MONITOR:
		return "探視民情";
	case QUERY:
		return "查詢網友";
	case TALK:
		return "聊天";
	case PAGE:
		return "呼叫";
	case CHAT1:
		return "Chat1";
	case CHAT2:
		return "Chat2";
	case CHAT3:
		return "Chat3";
	case CHAT4:
		return "Chat4";
	case IRCCHAT:
		return "會談IRC";
	case LAUSERS:
		return "探視網友";
	case XMENU:
		return "系統資訊";
	case VOTING:
		return "投票";
	case BBSNET:
		return "BBSNET";
	case EDITWELC:
		return "編輯Welc";
	case EDITUFILE:
		return "編輯個人檔";
	case EDITSFILE:
		return "編修系統檔";
	case ZAP:
		return "訂閱\討論區";
	case GAME:
		return "腦力激盪";
	case SYSINFO:
		return "檢查系統";
	case ARCHIE:
		return "ARCHIE";
	case DICT:
		return "翻查字典";
	case LOCKSCREEN:
		return "螢幕鎖定";
	case NOTEPAD:
		return "留言板";
	case GMENU:
		return "工具箱";
	case MSG:
		return "送訊息";
	case USERDEF:
		return "自訂參數";
	case EDIT:
		return "修改文章";
	case OFFLINE:
		return "自殺中..";
	case EDITANN:
		return "編修精華";
	case WWW:
		return "悠遊 WWW";
	case HYTELNET:
		return "Hytelnet";
	case CCUGOPHER:
		return "他站精華";
	case LOOKMSGS:
		return "察看訊息";
	case WFRIEND:
		return "尋人名冊";
	case WNOTEPAD:
		return "欲走還留";
	case BBSPAGER:
		return "網路傳呼";
	default:
		return "去了那裡!?";
	}
}
