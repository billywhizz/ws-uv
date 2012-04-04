#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "uv.h"
#include "http_parser.h"
#include "wsparser.h"
#include "sha1.h"

#define MAX_HEADERS 20
#define MAX_ELEMENT_SIZE 500
#define MAX_URL_SIZE 255

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef struct request request;
struct request {
  uint64_t id;
  char url[MAX_URL_SIZE];
  enum http_method method;
  unsigned short http_major;
  unsigned short http_minor;
  int num_headers;
  enum { NONE=0, FIELD, VALUE } last_header_element;
  char headers [MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  char wskey[61];
  uint8_t upgrade;
  uint8_t handshake;
  uint8_t keepalive;
};

typedef struct _context _context;
struct _context {
  int fd;
  http_parser* parser;
  ws_parser* wsparser;
  request* request;
  uv_stream_t* handle;
};

static inline void context_init (uv_stream_t* handle);
static inline void context_free (uv_handle_t* handle);

static int ws_header_cb(ws_parser* p);
static int ws_chunk_cb(ws_parser* p, const char* at, size_t len);
static int ws_complete_cb(ws_parser* p);

static int message_begin_cb (http_parser *p);
static int url_cb (http_parser *p, const char *buf, size_t len);
static int header_field_cb (http_parser *p, const char *buf, size_t len);
static int header_value_cb (http_parser *p, const char *buf, size_t len);
static int headers_complete_cb (http_parser *p);
static int message_complete_cb (http_parser *p);

static void on_connection(uv_stream_t* server, int status);
static void after_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf);
static void after_write(uv_write_t* req, int status);
static void on_close(uv_handle_t* peer);
static void after_shutdown(uv_shutdown_t* req, int status);
static uv_buf_t echo_alloc(uv_handle_t* handle, size_t suggested_size);

static int server_start(int port);

static http_parser_settings settings =
{
  .on_message_begin = message_begin_cb,
  .on_header_field = header_field_cb,
  .on_header_value = header_value_cb,
  .on_url = url_cb,
  .on_headers_complete = headers_complete_cb,
  .on_message_complete = message_complete_cb
};

static ws_settings wssettings = {
  .on_header = ws_header_cb,
  .on_chunk = ws_chunk_cb,
  .on_complete = ws_complete_cb,
};

static uv_tcp_t tcpServer;
static uv_loop_t* loop;
static const char* wshash = "                        258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static char r400[96];
static char r403[90];
static char r101[129];

static void xxdprint(const char *buff, uint64_t offset, uint64_t size) {
	uint64_t i = offset;
  uint64_t j;
	while(i < size)
	{
		uint8_t val = buff[i] & 0xff;
		if(i%8==0 && i > 0) {
			printf(" ");
		}
		if(i%16==0) {
			if(i>0) {
				printf(" ");
				for(j=i-16;j<i;j++) {
					int vv = buff[j];
					if(vv > 0x20 && vv < 0x7e) {
						printf("%c", vv);
					}
					else {
						printf(".");
					}
				}
				printf("\n");
			}
			printf("%.8lu: ", i);
		}
		printf("%.2x ", val);
		i++;
	}
	if(size%16!=0) {
		for(j=0; j<(16-(size%16)); j++) {
			printf("   ");
		}
		printf("  ");
		if(size%16<=8) printf(" ");
		for(j=i-(size%16);j<i;j++) {
			int vv = buff[j];
			if(vv > 0x20 && vv < 0x7e) {
				printf("%c", vv);
			}
			else {
				printf(".");
			}
		}
	}
	printf("\n");
}

static void print_request(request* req) {
  fprintf(stderr, "---------------------------------------------------\n");
  fprintf(stderr, "id: %lu\n", req->id);
  fprintf(stderr, "method: %i\n", req->method);
  fprintf(stderr, "major: %i\n", req->http_major);
  fprintf(stderr, "minor: %i\n", req->http_minor);
  fprintf(stderr, "url: %s\n", req->url);
  fprintf(stderr, "upgrade: %i\n", req->upgrade);
  fprintf(stderr, "keepalive: %i\n", req->keepalive);
  fprintf(stderr, "wskey: %s\n", req->wskey);
  fprintf(stderr, "handshake: %i\n", req->handshake);
  fprintf(stderr, "headers: %i\n", req->num_headers);
  int i=0;
  for(i=0; i<req->num_headers; i++) {
    fprintf(stderr, "\tname: %s\n", req->headers[i][0]);
    fprintf(stderr, "\tvalue: %s\n", req->headers[i][1]);
  }
  fprintf(stderr, "---------------------------------------------------\n");
}

static void print_ws_header(ws_header* header) {
  fprintf(stderr, "fin: %i\n", header->fin);
  fprintf(stderr, "rsv0: %i\n", header->reserved[0]);
  fprintf(stderr, "rsv1: %i\n", header->reserved[1]);
  fprintf(stderr, "rsv2: %i\n", header->reserved[2]);
  fprintf(stderr, "opcode: %i\n", header->opcode);
  fprintf(stderr, "length: %i\n", header->length);
  fprintf(stderr, "masking: %i\n", header->mask);
  fprintf(stderr, "mask0: %i\n", header->maskkey[0]);
  fprintf(stderr, "mask1: %i\n", header->maskkey[1]);
  fprintf(stderr, "mask2: %i\n", header->maskkey[2]);
  fprintf(stderr, "mask3: %i\n", header->maskkey[3]);
}