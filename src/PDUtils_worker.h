/*
  This code is derived from PDPCLIB, the public domain C runtime
  library by Paul Edwards. http://pdos.sourceforge.net/
  This code is released to the public domain.
*/
#include <pebble_worker.h>

time_t p_mktime(struct tm *tmptr);
char *p_strtok(char *s1, const char *s2);
long int p_strtol(const char *nptr, char **endptr, int base);