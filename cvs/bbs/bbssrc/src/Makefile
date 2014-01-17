#############################################################
####   Setup the following site-specific information     ####
#############################################################
# $Id: Makefile,v 1.7 2003/01/05 00:18:02 edwardc Exp $

# The home directory for user 'bbs'
BBSHOME=$(BBS_HOME)

# The uid/gid info for user 'bbs' and 'bbsadm'
BBSUID=$(BBS_UID)
BBSGRP=$(BBSGID)

OS_DEF   = $(OS_DEF)
CC       = $(CC)
CFLAGS   = $(CFLAGS)
LIBS     = $(LIBS)

INSTALL  = $(INSTALL)

CSIE_DEF = $(CSIE_DEF)

#############################################################
###        You needn't modify the following stuff      ######
#############################################################
PROGNAME = bbsd
CFILE	 = admintool.c announce.c bbs.c bbsd.c bbsgopher.c bcache.c bm.c \
		   boards.c chat.c comm_lists.c delete.c edit.c fileshm.c goodbye.c \
		   help.c io.c list.c mail.c main.c maintain.c modetype.c more.c \
		   namecomplete.c pass.c postheader.c read.c record.c \
		   register.c screen.c sendmsg.c stuff.c talk.c term.c \
		   userinfo.c vote.c xyz.c

COBJS	 = admintool.o announce.o bbs.o bbsd.o bbsgopher.o bcache.o bm.o \
		   boards.o chat.o comm_lists.o delete.o edit.o fileshm.o goodbye.o \
		   help.o io.o list.o mail.o main.o maintain.o modetype.o more.o \
		   namecomplete.o pass.o postheader.o read.o record.o \
		   register.o screen.o sendmsg.o stuff.o talk.o term.o \
		   userinfo.o vote.o xyz.o

SO_P	 = paging.o		
SO		 = paging.so thread.so

GARBAGE  = a.out core installchatd bbs.chatd chatd bbsrf *~ *.BAK

# The -DINVISIBLE makes bbs users invisible to 'finger' and 'who'.
# Simply remove it if you don't want this feature.

DEFINES	=  $(CSIE_DEF) $(OS_DEF)
EXTRADEF = -DHAVE_VERSION_H

# to active no admin tool bbs client, add -DWITHOUT_ADMIN_TOOLS
# into EXTRADEF, program will generate an admintool less bbsd.
 
.SUFFIXES: .o .c .so

.c.o:	;	$(CC) $(CFLAGS) $(DEFINES) $(EXTRADEF) -c $*.c
.o.so:	;	ld -s -G $*.o -o $*.so -L../lib -lBBS

#--------------------- Dependency starts here -------------------
all: version.h $(PROGNAME) chatd expire so
	rm -f ../include/version.h

so: 
	(cd SO; $(MAKE) CC=$(CC) CFLAGS="$(CFLAGS) -I- -I. -I../../include" DEFINES="$(DEFINES)" EXTRADEF="$(EXTRADEF)" ; cd .. )

installso: so
	echo "install .so packages .."
	(cd SO; $(MAKE) INSTALL="$(INSTALL)" BBSUID="$(BBSUID)" BBSGRP="$(BBSGRP)" installso ; cd ..)

version.h:
	echo "Generate version info.."
	sh ver.sh ../include/version.h

$(PROGNAME): $(COBJS)
	$(CC) -o $(PROGNAME) $(CFLAGS) $(CSIE_DEF) $(COBJS) $(LIBS) -L../lib -lBBS $(LD_PARA)
	@echo "Program size: `cat $(PROGNAME)|wc -c` bytes"

#disable bbsrf, it's can be replaced by in.zbbsd and native bbsd
#bbsrf: bbsrf.c 
#	$(CC) $(CFLAGS) -o bbsrf $(DEFINES) bbsrf.c $(LIBS)

chatd: station.c
	$(CC) $(CFLAGS) -o chatd $(OS_DEF) station.c $(LIBS) -L../lib -lBBS

install:
	sh Install.sh

installbbs: bbsd
	$(INSTALL) -s -m 550 -g $(BBSGRP) -o $(BBSUID) bbsd $(BBSHOME)/bin/bbsd.new
	-rm -f $(BBSHOME)/bin/bbsd.old
	-mv $(BBSHOME)/bin/bbsd $(BBSHOME)/bin/bbsd.old
	mv $(BBSHOME)/bin/bbsd.new $(BBSHOME)/bin/bbsd

installchatd: chatd
	$(INSTALL) -s -m 550 -g $(BBSGRP) -o $(BBSUID) chatd $(BBSHOME)/bin/chatd.new
	-rm -f $(BBSHOME)/bin/chatd.old
	-mv $(BBSHOME)/bin/chatd $(BBSHOME)/bin/chatd.old
	mv $(BBSHOME)/bin/chatd.new $(BBSHOME)/bin/chatd

clean: /tmp
	-rm -fr $(GARBAGE) $(PROGNAME) $(LNFILES) \
	../include/version.h expire

cleanall: clean
	-rm -f Install.sh
	-rm -f ../include/config.h
	-rm -f ../include/chat.h
	-rm -f bbs bbsrf chatd

tags: /tmp
	ctags $(CFILE)

update: installbbs installchatd installexpire installso

expire: expire.c record.c
	$(CC) $(CFLAGS) $(OS_DEF) -o expire expire.c

modestat:
	$(CC) $(CFLAGS) -o modestat modestat.c

installexpire: expire
	$(INSTALL) -s -m 550 -g $(BBSGRP) -o $(BBSUID) expire $(BBSHOME)/bin/expire.new
	-rm -f $(BBSHOME)/bin/expire.old
	-mv $(BBSHOME)/bin/expire $(BBSHOME)/bin/expire.old
	mv $(BBSHOME)/bin/expire.new $(BBSHOME)/bin/expire

# DO NOT DELETE
