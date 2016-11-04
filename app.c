#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "socket.h"

int main(int argc, char **argv) {
  loop = uv_default_loop();
  if (server_start(80)) return 1;
  uv_run(loop, UV_RUN_DEFAULT);
  return 0;
}