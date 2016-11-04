// addon.cc
#include <node.h>
#include "tinysocket.h"

namespace tiny {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

void CreateSocket(const FunctionCallbackInfo<Value>& args) {
  Socket::NewInstance(args);
}

void InitAll(Local<Object> exports, Local<Object> module) {
  Socket::Init(exports->GetIsolate());

  NODE_SET_METHOD(module, "exports", CreateSocket);
}

NODE_MODULE(tiny, InitAll)

}  // namespace tiny