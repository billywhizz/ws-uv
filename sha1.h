#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/*
most of the code here was taken from:
https://code.google.com/p/smallsha1/source/browse/trunk/sha1.cpp?spec=svn4&r=4
and base64 code was taken from:
http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
*/
void shacalc(const char* src, char* dest);