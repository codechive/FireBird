#############################################################
####   Setup the following site-specific information     ####
#############################################################
# $Id: Makefile,v 1.4 2003/01/05 00:18:02 edwardc Exp $

#############################################################
###        You needn't modify the following stuff      ######
#############################################################

SO_P	 = paging.o	thread.o
SO		 = paging.so thread.so

GARBAGE  = a.out core *~ *.BAK

.SUFFIXES: .o .c .so

.c.o:	;	$(CC) $(CFLAGS) $(DEFINES) $(EXTRADEF) -c $*.c
.o.so:	;	ld -s -G $*.o -o $*.so -L../../lib -lBBS

#--------------------- Dependency starts here -------------------
all: so

so: $(SO_P) $(SO)

installso: so
	echo "install .so packages .."
	$(INSTALL) -s -m 550 -g $(BBSGRP) -o $(BBSUID) $(SO) $(BBSHOME)/bin/SO

clean: /tmp
	-rm -fr $(GARBAGE) $(SO) $(SO_P) 

cleanall: clean
	-rm -f Install.sh
	-rm -f ../include/config.h
	-rm -f ../include/chat.h
	-rm -f bbs bbsrf chatd thread

tags: /tmp
	ctags $(CFILE)

update: installso

# DO NOT DELETE
