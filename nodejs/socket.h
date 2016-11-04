// tinysocket.h
#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "uv.h"

#ifdef __cplusplus
extern "C" {
namespace tiny {
#endif

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef struct {
	void* object;
	void* callback;
} baton_t;

typedef struct _context _context;
void context_init (uv_stream_t* handle, _context* ctx);
void context_free (uv_handle_t* handle);

void on_connection(uv_stream_t* server, int status);
void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
void after_write(uv_write_t* req, int status);
void on_close(uv_handle_t* peer);
void after_shutdown(uv_shutdown_t* req, int status);
void echo_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf);

typedef int (*on_data) (_context*, const char *at, size_t len);
typedef int (*cb) (_context*);

static int fd = 0;

struct _context {
  int fd;
  uv_stream_t* handle;
  void* data;
	on_data ondata;
};

int server_start(int port, uv_tcp_t* tcpServer, cb on_context);

#ifdef TRACE
void xxdprint(const char *buff, uint64_t offset, uint64_t size) {
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
#endif
#ifdef __cplusplus
}
}
#endif
#endif