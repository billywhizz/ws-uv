#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "sha1.h"
int main(int argc, char **argv) {
  const char* hash = "hello";
  char* digest = malloc(28);
  char* expected = "qvTGHdzF6KLavt4PO0gs2a6pQ00=";
  shacalc(hash, digest);
  assert(strcmp(expected, digest) == 0);
  fprintf(stderr, "%s\n", digest);
  return 0;
}