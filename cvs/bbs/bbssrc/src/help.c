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
$Id: help.c,v 1.2 2002/09/05 06:04:10 edwardc Exp $
*/

#include "bbs.h"
void
show_help(fname)
char   *fname;
{
	ansimore(fname, YEA);
	clear();
}
/*void
standhelp( mesg )
char    *mesg;
{
    prints("[1;32;44m");
    prints( mesg ) ;
    prints("[m");
}*/

int
mainreadhelp()
{
	show_help(F_HELP_MAINREAD);
	return FULLUPDATE;
}

int
mailreadhelp()
{
	show_help(F_HELP_MAILREAD);
	return FULLUPDATE;
}

void
Help()
{
	currentuser.flags[0] ^= CURSOR_FLAG;
	clear();
}
