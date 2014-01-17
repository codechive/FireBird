/* an elementary DLM support.*/
/* CVS: $Id: dlm.c,v 1.1 2000/01/15 01:45:26 edwardc Exp $ */

#ifndef BBS
  #include <dlfcn.h>
  #include <varargs.h>
  #include <strings.h>
  #include <stdlib.h>
  #include <stdio.h>
#else
  #include "bbs.h"
#endif

char 	dl_lib_c[] =
	"$Id: dlm.c,v 1.1 2000/01/15 01:45:26 edwardc Exp $";

void   *
DL_get(name, errno)
char   *name;
int    *errno;
{
	char    buf[512], symbol[256];
	void	*handle;
	int		x = 0;
	
	if ( !strchr(name, ':') ) {
		*errno = -1;
		return NULL;
	}

	strcpy(buf, (char *)stringtoken(name, ':', &x));
	strcpy(symbol, (char *)stringtoken(name, ':', &x));

	if ( !dashf(buf) ) {
		*errno = -2;
		return NULL;
	}
	
	report(buf);
	report(symbol);
		
	handle = dlopen(buf, RTLD_LAZY);

	if ( !handle ) {
		*errno = -3;
		report(dlerror());
		return NULL;
	}

	return dlsym(handle, symbol);
}
