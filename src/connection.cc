#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "connection.hh"

using namespace v8;

Connection::Connection() {};
Connection::~Connection() {};

Persistent<Function> Connection::constructor;

void Connection::Init() {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("Connection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("plusOne"),
      FunctionTemplate::New(PlusOne)->GetFunction());

  constructor = Persistent<Function>::New(tpl->GetFunction());
}

Handle<Value> Connection::New(const Arguments& args) {
  HandleScope scope;

  Connection* obj = new Connection();
  obj->counter_ = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> Connection::Connect(const Arguments& args) {
  HandleScope scope;

  const unsigned argc = 1;
  Handle<Value> argv[argc] = { args[0] };
  Local<Object> instance = constructor->NewInstance(argc, argv);

  return scope.Close(instance);
}

Handle<Value> Connection::PlusOne(const Arguments& args) {
  HandleScope scope;

  Connection* obj = ObjectWrap::Unwrap<Connection>(args.This());
  obj->counter_ += 1;

  return scope.Close(Number::New(obj->counter_));
}
