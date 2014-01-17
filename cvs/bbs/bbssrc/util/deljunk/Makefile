#
# CVS: $Id: Makefile,v 1.4 2000/07/22 07:10:46 edwardc Exp $
#
#################################################################

INSTALLDIR = @BBS_HOME@/bin

all: deljunk

deljunk:
		$(CC) -o deljunk deljunk.c -I../../include

install: deljunk
		install -g bbs -m 700 -o bbs deljunk $(INSTALLDIR)

clean: /tmp
		-rm -fr *~ *.o *.BAK

# DO NOT DELETE
