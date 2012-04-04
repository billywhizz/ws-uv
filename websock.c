#include "websock.h"

inline void context_init (uv_stream_t* handle) {
  _context* context = malloc(sizeof(_context));
  context->parser = malloc(sizeof(http_parser));
  context->request = malloc(sizeof(request));
  strcpy(context->request->wskey, wshash);
  context->request->id = 0;
  assert(context->request);
  context->request->handshake = 0;
  context->handle = handle;
  handle->data = context;
  http_parser_init(context->parser, HTTP_REQUEST);
  context->parser->data = context;
}

inline void context_free (uv_handle_t* handle) {
  _context* context = handle->data;
  if(context) {
    free(context->request);
    free(context->parser);
    free(context->wsparser);
    free(context);
  }
  free(handle);
}

int ws_header_cb(ws_parser* p) {
  fprintf(stderr, "on_header: %li\n", p->index);
  print_ws_header(&p->header);
  return 0;
}

int ws_chunk_cb(ws_parser* p, const char* at, size_t len) {
  fprintf(stderr, "on_chunk: %li\t%li\n", p->index, len);
  xxdprint(at, 0, len);
  return 0;
}

int ws_complete_cb(ws_parser* p) {
  fprintf(stderr, "on_complete: %li\n", p->index);
  return 0;
}

int message_begin_cb (http_parser *p) {
  _context* ctx = p->data;
  int i=0;
  for(i=0; i < MAX_HEADERS; i++) {
    ctx->request->headers[i][0][0] = 0;
    ctx->request->headers[i][1][0] = 0;
  }
  ctx->request->id++;
  ctx->request->num_headers = 0;
  ctx->request->last_header_element = NONE;
  ctx->request->upgrade = 0;
  ctx->request->handshake = 0;
  ctx->request->keepalive = 1;
  ctx->request->url[0] = 0;
  return 0;
}

int url_cb (http_parser *p, const char *buf, size_t len) {
  _context* ctx = p->data;
  strncat(ctx->request->url, buf, len);
  return 0;
}

int header_field_cb (http_parser *p, const char *buf, size_t len) {
  _context* ctx = p->data;
  request* req = ctx->request;
  if(req->last_header_element != FIELD) {
    req->num_headers++;
  }
  strncat(req->headers[req->num_headers-1][0], buf, len);
  req->last_header_element = FIELD;
  return 0;
}

int header_value_cb (http_parser *p, const char *buf, size_t len) {
  _context* ctx = p->data;
  request* req = ctx->request;
  strncat(req->headers[req->num_headers-1][1], buf, len);
  req->last_header_element = VALUE;
  return 0;
}

int headers_complete_cb (http_parser *p) {
  _context* ctx = p->data;
  request* req = ctx->request;
  
  req->keepalive = http_should_keep_alive(p);
  req->http_major = p->http_major;
  req->http_minor = p->http_minor;
  req->method = p->method;
  req->upgrade = p->upgrade;
  return 0;
}

int message_complete_cb (http_parser *p) {
  _context* ctx = p->data;
  request* req = ctx->request;
  write_req_t *wr;
  wr = (write_req_t*) malloc(sizeof *wr);
  int i;
  for(i=0; i<req->num_headers; i++) {
    if(strncasecmp(req->headers[i][0], "Sec-WebSocket-Key", 17) == 0) {
      strncpy(req->wskey, req->headers[i][1], 24);
      break;
    }
  }
  shacalc(req->wskey, r101 + 97);
  wr->buf = uv_buf_init(r101, 129);
  if (uv_write(&wr->req, ctx->handle, &wr->buf, 1, after_write)) {
    exit(1);
  }
  print_request(ctx->request);
  req->handshake = 1;
  //TODO: free the http parser and ensure it is not freed again in context_free
  //free(ctx->parser);
  ctx->wsparser = malloc(sizeof(ws_parser));
  ws_init(ctx->wsparser);
  ctx->wsparser->data = ctx;
  if(!http_should_keep_alive(p)) {
    uv_close((uv_handle_t*)ctx->handle, on_close);
  }
  return 0;
}

void after_write(uv_write_t* req, int status) {
  write_req_t* wr;
  if (status) {
    uv_err_t err = uv_last_error(loop);
    fprintf(stderr, "uv_write error: %s\n", uv_strerror(err));
  }
  wr = (write_req_t*) req;
  //TODO: if we are allocating a new buffer on each write we need to free it here
  //free(wr->buf.base);
  free(wr);
}

void on_close(uv_handle_t* peer) {
  context_free(peer);
}

void after_shutdown(uv_shutdown_t* req, int status) {
  uv_close((uv_handle_t*)req->handle, on_close);
  free(req);
}

void after_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  if (nread < 0) {
    if (buf.base) {
      free(buf.base);
    }
    uv_close((uv_handle_t*)handle, on_close);
    return;
  }
  if (nread == 0) {
    free(buf.base);
    return;
  }
  _context* ctx = handle->data;
  if(ctx->request->handshake == 0) {
    size_t np = http_parser_execute(ctx->parser, &settings, buf.base, nread);
    free(buf.base);
    if(np != nread) {
      uv_shutdown_t* req;
      req = (uv_shutdown_t*) malloc(sizeof *req);
      uv_shutdown(req, handle, after_shutdown);
    }
  }
  else {
    size_t np = ws_execute(ctx->wsparser, &wssettings, buf.base, 0, nread);
    write_req_t *wr;
    wr = (write_req_t*) malloc(sizeof *wr);
    wr->buf = uv_buf_init(buf.base, nread);
    if (uv_write(&wr->req, ctx->handle, &wr->buf, 1, after_write)) {
      exit(1);
    }
    free(buf.base);
    if(np != nread) {
      uv_shutdown_t* req;
      req = (uv_shutdown_t*) malloc(sizeof *req);
      uv_shutdown(req, handle, after_shutdown);
      return;
    }
  }
}

uv_buf_t echo_alloc(uv_handle_t* handle, size_t suggested_size) {
  return uv_buf_init(malloc(suggested_size), suggested_size);
}

void on_connection(uv_stream_t* server, int status) {
  uv_stream_t* stream;
  int r;
  if (status != 0) {
    fprintf(stderr, "Connect error %d\n", uv_last_error(loop).code);
  }
  assert(status == 0);
  stream = malloc(sizeof(uv_tcp_t));
  assert(stream != NULL);
  r = uv_tcp_init(loop, (uv_tcp_t*)stream);
  assert(r == 0);
  r = uv_accept(server, stream);
  assert(r == 0);
  context_init(stream);
  r = uv_read_start(stream, echo_alloc, after_read);
  assert(r == 0);
}

int server_start(int port) {
  strncpy(r101, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept:                             \r\n\r\n", 129);
  strncpy(r400, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\n", 96);
  strncpy(r403, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\n", 90);
  struct sockaddr_in addr = uv_ip4_addr("0.0.0.0", port);
  int r;
  r = uv_tcp_init(loop, &tcpServer);
  if (r) {
    fprintf(stderr, "Socket creation error\n");
    return 1;
  }
  r = uv_tcp_bind(&tcpServer, addr);
  if (r) {
    fprintf(stderr, "Bind error\n");
    return 1;
  }
  r = uv_listen((uv_stream_t*)&tcpServer, 4096, on_connection);
  if (r) {
    fprintf(stderr, "Listen error\n");
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  loop = uv_default_loop();
  if (server_start(80)) return 1;
  uv_run(loop);
  return 0;
}