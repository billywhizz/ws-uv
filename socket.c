#include "socket.h"

inline void context_init (uv_stream_t* handle) {
  _context* context = malloc(sizeof(_context));
  context->handle = handle;
  handle->data = context;
}

inline void context_free (uv_handle_t* handle) {
  _context* context = handle->data;
  if(context) {
    free(context);
  }
  free(handle);
}

void after_write(uv_write_t* req, int status) {
  write_req_t* wr;
  if (status) {
    fprintf(stderr, "uv_write error: %s\n", strerror(status));
  }
  wr = (write_req_t*) req;
  //TODO: if we are allocating a new buffer on each write we need to free it here
  free(wr->buf.base);
  free(wr);
}

void on_close(uv_handle_t* peer) {
  context_free(peer);
}

void after_shutdown(uv_shutdown_t* req, int status) {
  uv_close((uv_handle_t*)req->handle, on_close);
  free(req);
}

void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
  if (nread < 0) {
    if (buf->base) {
      free(buf->base);
    }
    uv_close((uv_handle_t*)handle, on_close);
    return;
  }
  if (nread == 0) {
    free(buf->base);
    return;
  }
  _context* ctx = handle->data;
  write_req_t *wr;
  wr = (write_req_t*) malloc(sizeof *wr);
#ifdef TRACE
  xxdprint(buf->base, 0, nread);
#endif
  wr->buf = uv_buf_init(buf->base, nread);
  if (uv_write(&wr->req, ctx->handle, &wr->buf, 1, after_write)) {
    uv_shutdown_t* req;
    req = (uv_shutdown_t*) malloc(sizeof *req);
    uv_shutdown(req, handle, after_shutdown);
  }
}

void echo_alloc(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  buf->base = malloc(size);
  buf->len = size;
}

void on_connection(uv_stream_t* server, int status) {
  uv_stream_t* stream;
  assert(status == 0);
  stream = malloc(sizeof(uv_tcp_t));
  assert(stream != NULL);
  status = uv_tcp_init(uv_default_loop(), (uv_tcp_t*)stream);
  assert(status == 0);
  status = uv_accept(server, stream);
  assert(status == 0);
  context_init(stream);
  status = uv_read_start(stream, echo_alloc, after_read);
  assert(status == 0);
}

int server_start(int port) {
  struct sockaddr_in addr;
  uv_tcp_t tcpServer;
  uv_ip4_addr("0.0.0.0", port, &addr);
  int status;
  status = uv_tcp_init(uv_default_loop(), &tcpServer);
  if (status) {
    fprintf(stderr, "Socket creation error\n");
    return 1;
  }
  status = uv_tcp_bind(&tcpServer, (const struct sockaddr*) &addr, 0);
  if (status) {
    fprintf(stderr, "Bind error\n");
    return 1;
  }
  status = uv_listen((uv_stream_t*)&tcpServer, 4096, on_connection);
  if (status) {
    fprintf(stderr, "Listen error\n");
    return 1;
  }
  return 0;
}
/*
int main(int argc, char **argv) {
  loop = uv_default_loop();
  if (server_start(80)) return 1;
  uv_run(loop, UV_RUN_DEFAULT);
  return 0;
}
*/