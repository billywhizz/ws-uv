#include <stdint.h>
#include <sys/types.h>

typedef struct ws_parser ws_parser;
typedef struct ws_settings ws_settings;
typedef struct ws_header ws_header;

typedef int (*ws_data_cb) (ws_parser*, const char *at, size_t len);
typedef int (*ws_cb) (ws_parser*);

enum state {
  WS_HEADER = 0,
  WS_BODY = 1
};

struct ws_settings {
  ws_cb on_header;
  ws_data_cb on_chunk;
  ws_cb on_complete;
};

struct ws_header {
  uint8_t fin;
  uint8_t reserved[3];
  uint8_t opcode;
  uint32_t length;
  uint8_t mask;
  uint8_t maskkey[4];
};

struct ws_parser {
  void* data;
  ws_header header;
  uint64_t index;
  int state;
  int payload16;
  int payload64;
  int maskpos;
  uint64_t bodypos;
};

void ws_init(ws_parser* parser);
size_t ws_execute(ws_parser* parser, const ws_settings* settings, \
  const char* data, size_t start, size_t end);
void ws_reset(ws_parser* parser);