#!/bin/sh -
# make version.h by using a shell program :D
# CVS: $Id: ver.sh,v 1.1 2000/01/15 01:45:30 edwardc Exp $

if [ ".$1" = "." ]; then
	echo "no specify output file"
	exit 3
fi

rm -f $1

echo "#ifndef _VERSION_H_" >> $1
echo "#define _VERSION_H_" >> $1

if [ -f "../.cvs_tag" ]; then
	echo "#define CVS 1 " >> $1
fi

if [ -f "../.reldate" ]; then
	echo "#define RELDATE \"`cat ../.reldate`\" " >> $1
fi

echo "#define COMPILEDATE \"`date`\" " >> $1
echo "#define RELEASETIME (938960203L) " >> $1

echo "#endif /* _VERSION_H_ */ " >> $1
