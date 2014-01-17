#!/bin/sh
# 此 shell script 利用 ctm 一次更新多個 ctm Delta
# NAME 為數字之前的檔名
# 用法 : 將 ctm.sh 放到要解開的目錄 ex. /home/bbsadm/bbssrc，CTM檔假設放在
#	 /home/bbsadm/CTM，想要更新編號 2 - 23 的 Delta
#
#	cd /home/bbsadm/bbssrc
#	ctm.sh /home/bbsadm/CTM 2 23
#					任何問題請 mail: skyo@mgt.ncu.edu.tw
#
#   $Id: ctm.sh,v 1.1 2000/01/15 01:45:24 edwardc Exp $    
NAME="fb3src-"
A=$2
while [ $3 -ge $A ]
do
	ctm -v $1/$NAME$A.gz
        A=`expr $A + 1 `
done
