/*
 * Taken from https://gist.github.com/2732711
 * Based off of http://sambro.is-super-awesome.com/2011/03/03/creating-a-proper-buffer-in-a-node-c-addon/
 */

#include <node.h>
#include <node_buffer.h>

static inline v8::Local<v8::Object> makeBuffer(char* data, size_t size) {
  v8::HandleScope scope;

  // It ends up being kind of a pain to convert a slow buffer into a fast
  // one since the fast part is implemented in JavaScript.
  v8::Local<node::Buffer> slowBuffer = node::Buffer::New(data, size);
  // First get the Buffer from global scope...
  v8::Local<v8::Object> global = v8::Context::GetCurrent()->Global();
  v8::Local<v8::Value> bv = global->Get(v8::String::NewSymbol("Buffer"));
  assert(bv->IsFunction());
  v8::Local<v8::Function> b = v8::Local<v8::Function>::Cast(bv);
  // ...call Buffer() with the slow buffer and get a fast buffer back...
  v8::Handle<v8::Value> argv[] = { slowBuffer->handle_, v8::Integer::New(size), v8::Integer::New(0) };
  v8::Local<v8::Object> fastBuffer = b->NewInstance(sizeof(argv) / sizeof(*argv), argv);

  return scope.Close(fastBuffer);
}
