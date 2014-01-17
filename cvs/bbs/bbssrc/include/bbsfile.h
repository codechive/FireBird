/*
 * bbsfile.h		-- include file for bbs config file location definition
 *	
 * Copyright (c) 2000, Edward Ping-Da Chuang <edwardc@firebird.org.tw>
 * Firebird BBS Project, All rights reserved.
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
 * CVS: $Id: bbsfile.h,v 1.6 2002/09/05 06:04:15 edwardc Exp $
 */

#ifndef _BBSFILE_H_
#define	_BBSFILE_H_

#define D_ETC	"etc"		/* etc directory for config file */
#define D_TMP	"tmp"		/* temo file directory */
#define D_HELP	"help"		/* help message file */

/* help files */

#define F_HELP_ANNREAD	D_HELP"/announcereadhelp"
#define F_HELP_BRDREAD	D_HELP"/boardreadhelp"
#define	F_HELP_CHAT		D_HELP"/chathelp"
#define F_HELP_CHATOP	D_HELP"/chatophelp"
#define F_HELP_EDIT		D_HELP"/edithelp"
#define F_HELP_MAINREAD	D_HELP"/mainreadhelp"
#define F_HELP_MAILREAD	D_HELP"/mailreadhelp"
#define F_HELP_USERS	D_HELP"/usershelp"
#define F_HELP_USERLIST	D_HELP"/userlisthelp"
#define F_HELP_MAILERR	D_HELP"/mailerror-explain"
#define F_HELP_MSG		D_HELP"/msghelp"
#define F_HELP_MORE		D_HELP"/morehelp"
#define F_HELP_FRIENDS	D_HELP"/friendshelp"
#define F_HELP_REJECTS	D_HELP"/rejectshelp"
#define F_HELP_VOTE		D_HELP"/votehelp"

/* temp files */
#define F_KILL_USER		D_TMP"/killuser"

/* setting files */
#define F_BAD_ID		D_ETC"/bad_id"
#define F_BAD_EMAIL		D_ETC"/bad_email"
#define F_TRUSTED_HOST	D_ETC"/trusted_host"

/* message files */
#define	F_USER_FULL		D_ETC"/user_full"
#define F_REGISTER		D_ETC"/register"
#define F_SUCC_EMAIL	D_ETC"/smail"
#define F_MENTOR_NOT	D_ETC"/mentor"

/* home files */
#define HF_REGISTER		"register"
#define HF_REGISTER_OLD "register.old"
#define HF_REGPASS		".regpass"
#define HF_MAILCHECK	"mailcheck"
#define	HF_MENTOR		"mentor"
#define HF_DOWNLINE		"downline"

#endif	/* _BBSFILE_H_ */
