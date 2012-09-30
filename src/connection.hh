#define BUILDING_NODE_EXTENSION
#ifndef MYOBJECT_HH
#define MYOBJECT_HH

#include <node.h>

class Connection : public node::ObjectWrap {
 public:
  static void Init();
  static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);

 private:
  Connection();
  ~Connection();

  static v8::Persistent<v8::Function> constructor;
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> PlusOne(const v8::Arguments& args);
  double counter_;
};

#endif
