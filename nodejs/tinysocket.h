// tinysocket.h
#ifndef TINYSOCKET_H
#define TINYSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <node.h>
#include <node_object_wrap.h>
#include "http_parser.h"
#include "uv.h"

namespace tiny {


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
};

int server_start(int port, uv_tcp_t* tcpServer, cb on_context);

class Socket : public node::ObjectWrap {
 public:
  static void Init(v8::Isolate* isolate);
  static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
  v8::Persistent<v8::Function> onConnect;
  v8::Persistent<v8::Function> onData;
  v8::Persistent<v8::Function> onClose;

 private:
  explicit Socket(double value = 0);
  ~Socket();

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Listen(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Close(const v8::FunctionCallbackInfo<v8::Value>& args);
  static v8::Persistent<v8::Function> constructor;
  double value_;
  uv_tcp_t* server_;

};

}  // namespace tiny

#endif