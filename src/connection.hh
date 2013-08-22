#define BUILDING_NODE_EXTENSION
#ifndef MYOBJECT_HH
#define MYOBJECT_HH

#include <node.h>
#include <sqlcli.h>

class Connection : public node::ObjectWrap {
    public:
        static void Init();
        static v8::Handle<v8::Value> Connect(const v8::Arguments& args);

    private:
        Connection(char const* server, 
                       char const* user, 
                       char const* password);
        ~Connection();

        SQLHANDLE  environment;
        SQLHANDLE  connection;

        static v8::Persistent<v8::Function> constructor;
        static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> Execute(const v8::Arguments& args);
};

#endif
