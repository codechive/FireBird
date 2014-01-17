#
#	libBBS (libbbs.a) version 1.0 for FirebirdBBS Series
#	a part of SEEDNetBBS generation 1
#
#	$Id: Makefile,v 1.1 2000/01/15 01:45:25 edwardc Exp $
#
#############################################################
###        You needn't modify the following stuff      ######
#############################################################
LIBNAME	 = libBBS.a

GARBAGE  = a.out core out.* *.BAK DEAD*

CFILE	 = string.c html.c mmdecode.c fileio.c dlm.c

OBJS	 = string.o html.o mmdecode.o fileio.o dlm.o

.SUFFIXES: .o .c

.c.o:   ;   $(CC) -s $(CFLAGS) -Wunused -c $*.c

#--------------------- Dependency starts here -------------------

all: $(OBJS)
	ar rv ../$(LIBNAME) $(OBJS)
	ranlib ../$(LIBNAME)

tag:
	ctags $(CFILE)

clean: /tmp
	@echo --- Cleaning Garbage ... ---
	-rm -fr $(GARBAGE) *.o *~

cleanall: clean
	@echo --- Cleanning Grabage with $(LIBNAME) ---
	-rm -fr $(LIBNAME) ../$(LIBNAME)

install:
	@echo --- lib: skip ---
