// myobject.cc
#include <node.h>
#include "tinysocket.h"

namespace tiny {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Integer;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

Persistent<Function> Socket::constructor;

Socket::Socket(double value) : value_(value) {
}

Socket::~Socket() {
}


inline void context_init (uv_stream_t* handle, _context* ctx) {
  ctx->handle = handle;
  ctx->fd = fd++;
  handle->data = ctx;
}

inline void context_free (uv_handle_t* handle) {
  _context* context = (_context*)handle->data;
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
  Isolate * isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  _context* ctx = (_context*)peer->data;
  Socket* obj = (Socket*)ctx->data;
  const unsigned int argc = 1;
  Local<Value> argv[argc] = { Integer::New(isolate, ctx->fd) };
  Local<Function> onClose = Local<Function>::New(isolate, obj->onClose);
  onClose->Call(isolate->GetCurrentContext()->Global(), 1, argv);
  context_free(peer);
}

void after_shutdown(uv_shutdown_t* req, int status) {
  uv_close((uv_handle_t*)req->handle, on_close);
  free(req);
}

void after_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
  Isolate * isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
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
  _context* ctx = (_context*)handle->data;
  Socket* obj = (Socket*)ctx->data;
  const unsigned int argc = 2;
  Local<Value> argv[argc] = { Integer::New(isolate, ctx->fd), Number::New(isolate, nread) };
  Local<Function> onData = Local<Function>::New(isolate, obj->onData);
  onData->Call(isolate->GetCurrentContext()->Global(), 2, argv);

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
  buf->base = (char*)malloc(size);
  buf->len = size;
}

void on_connection(uv_stream_t* server, int status) {
  uv_stream_t* stream;

  uv_tcp_t* tcpServer = (uv_tcp_t*)server;
  baton_t* baton = (baton_t*)tcpServer->data;
  cb foo = (cb)baton->callback;

  assert(status == 0);
  stream = (uv_stream_t*)malloc(sizeof(uv_tcp_t));
  assert(stream != NULL);
  status = uv_tcp_init(uv_default_loop(), (uv_tcp_t*)stream);
  assert(status == 0);
  status = uv_accept(server, stream);
  assert(status == 0);
  _context* ctx = (_context*)malloc(sizeof(_context));
  context_init(stream, ctx);
  ctx->data = baton->object;
  foo(ctx);
  status = uv_read_start(stream, echo_alloc, after_read);
  assert(status == 0);
}

int server_start(int port, uv_tcp_t* tcpServer, cb on_context) {
  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", port, &addr);
  baton_t* baton = (baton_t*)malloc(sizeof(baton_t));
  baton->callback = (void*)on_context;
  baton->object = tcpServer->data;
  tcpServer->data = baton;
  int status;
  status = uv_tcp_init(uv_default_loop(), tcpServer);
  if (status) {
    fprintf(stderr, "Socket creation error\n");
    return 1;
  }
  status = uv_tcp_bind(tcpServer, (const struct sockaddr*) &addr, 0);
  if (status) {
    fprintf(stderr, "Bind error\n");
    return 1;
  }
  status = uv_listen((uv_stream_t*)tcpServer, 4096, on_connection);
  if (status) {
    fprintf(stderr, "Listen error\n");
    return 1;
  }
  return 0;
}

void Socket::Init(Isolate* isolate) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "Socket"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "listen", Listen);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "in", In);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "out", Out);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "slice", Slice);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "write", Write);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "writeCopy", WriteCopy);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "push", Push);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "connect", Connect);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "pause", Pause);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "resume", Resume);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "upgrade", Upgrade);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "upgradeClient", Upgrade2);

  //NODE_SET_PROTOTYPE_METHOD(tpl, "bind", Bind);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "listenHandle", ListenHandle);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "getQueueSize", QueueSize);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "setNoDelay", SetNoDelay);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "setKeepAlive", SetKeepAlive);
  //NODE_SET_PROTOTYPE_METHOD(tpl, "getPeerName", GetPeerName);

  constructor.Reset(isolate, tpl->GetFunction());
}

void Socket::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  if (args.IsConstructCall()) {
    double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
    Socket* obj = new Socket(value);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    const int argc = 1;
    Local<Value> argv[argc] = { args[0] };
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> instance =
        cons->NewInstance(context, argc, argv).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
}

void Socket::NewInstance(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  const unsigned argc = 1;
  Local<Value> argv[argc] = { args[0] };
  Local<Function> cons = Local<Function>::New(isolate, constructor);
  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> instance =
      cons->NewInstance(context, argc, argv).ToLocalChecked();
  args.GetReturnValue().Set(instance);
}

void Socket::Close(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Socket* obj = ObjectWrap::Unwrap<Socket>(args.Holder());
/*
  HandleScope scope;
  int fd = args[0]->Int32Value();
  _context *ctx = contexts[fd];
  uv_shutdown_t *req;
  req = (uv_shutdown_t *)malloc(sizeof *req);
  uv_shutdown(req, ctx->handle, after_shutdown);
  return scope.Close(Integer::New(1));
*/
}

int onNewConnection(_context* ctx) {
  Socket* obj = (Socket*)ctx->data;

  Isolate * isolate = Isolate::GetCurrent();
  v8::HandleScope handleScope(isolate);
  const unsigned int argc = 1;
  Local<Value> argv[argc] = { Integer::New(isolate, ctx->fd) };
  Local<Function> foo = Local<Function>::New(isolate, obj->onConnect);
  foo->Call(isolate->GetCurrentContext()->Global(), 1, argv);

  return 0;
}

void Socket::Listen(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Socket* obj = ObjectWrap::Unwrap<Socket>(args.Holder());

  const unsigned int port = args[0]->IntegerValue();

  Local<Function> onConnect = Local<Function>::Cast(args[1]);
  obj->onConnect.Reset(isolate, onConnect);

  Local<Function> onData = Local<Function>::Cast(args[2]);
  obj->onData.Reset(isolate, onData);

  Local<Function> onClose = Local<Function>::Cast(args[3]);
  obj->onClose.Reset(isolate, onClose);

  obj->server_ = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  obj->server_->data = obj;

  int r = server_start(port, obj->server_, onNewConnection);
  args.GetReturnValue().Set(Integer::New(isolate, r));
}
}  // namespace tiny