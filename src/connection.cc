#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "connection.hh"

using namespace v8;

static char const*
typeToString(SQLSMALLINT  type)
{
    switch (type) {
    case SQL_INTEGER:
        return "integer";
    case SQL_VARCHAR:
        return "string";
    default:
        fprintf(stderr,
                "%s:%d:%s():FIXME:unsupported type %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                type);
        return "unknown";
    }
}

Connection::Connection() {
    SQLRETURN  statusCode;
    uint8_t* password = NULL;
    uint8_t* server = (uint8_t*)"olaptest";
    uint8_t* user = NULL;

    statusCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);

        // FIXME: consider having a defunct state (and do not return defunct instances from Connect())
        return;
    }

    // FIXME: check whether we want to use SQLSetEnvAttr();

    statusCode = SQLAllocHandle(SQL_HANDLE_DBC, environment, &connection);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    // FIXME: check whether we want to use SQLSetConnectAttr();

    // FIXME: also implement this with: SQLDriverConnect(hdbc, (SQLHWND)NULL, "DSN=SAMPLE;UID=;PWD=;", NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    statusCode = SQLConnect(connection, server, SQL_NTS, user, SQL_NTS, password, SQL_NTS);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }
}

Handle<Value> Connection::Execute (const Arguments& args) {
    HandleScope scope;

    Connection* connection = ObjectWrap::Unwrap<Connection>(args.This());

    SQLHANDLE  hstmt;
    SQLRETURN  statusCode;

    if (args.Length() < 1) {
        // FIXME: specify the number given and the number expected
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    statusCode = SQLAllocHandle(SQL_HANDLE_STMT, connection->connection, &hstmt);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    // FIXME: optional: SQLSetStmtAttr()

    String::Utf8Value query(args[0]->ToString());

    statusCode = SQLPrepare(hstmt, (SQLCHAR *)*query, SQL_NTS);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    SQLSMALLINT  parameterCount = 0;
    statusCode = SQLNumParams(hstmt, &parameterCount);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    for (SQLSMALLINT parameter = 0; parameter < parameterCount; parameter++) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle parameter binding\n",
                __FILE__, __LINE__, __FUNCTION__);
        return scope.Close(Undefined());

        char stringParam[] = "POST";
        statusCode = SQLBindParameter(hstmt, 1 /* argumentNumber */, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, stringParam, 20, NULL);
        if (!SQL_SUCCEEDED(statusCode)) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:handle status code: %d\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    statusCode);
            return scope.Close(Undefined());
        }
    }

    statusCode = SQLExecute(hstmt);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    do {
    SQLSMALLINT  columnCount = 0;
    statusCode = SQLNumResultCols(hstmt, &columnCount);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    struct {
        Local<String>  name;
        SQLSMALLINT    type;
    } columns[11];
    if ((size_t)columnCount > sizeof(columns) / sizeof(*columns)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:increase column count to %u (from %lu)\n",
                __FILE__, __LINE__, __FUNCTION__,
                columnCount, sizeof(columns) / sizeof(*columns));
        return scope.Close(Undefined());
    }

    SQLSMALLINT column;
    for (column = 1; column <= columnCount; column++) {
        SQLCHAR nameBuffer[128];
        SQLSMALLINT  nameLength = 0;
        SQLSMALLINT  columnType = 0;
        SQLULEN      precision = 0;
        SQLSMALLINT  decimals = 0;
        SQLSMALLINT  nullable = SQL_NULLABLE;
        statusCode = SQLDescribeCol(hstmt, column,
                                    nameBuffer, sizeof(nameBuffer),
                                    &nameLength, &columnType,
                                    &precision, &decimals, &nullable);

        if (!SQL_SUCCEEDED(statusCode)) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:handle status code: %d\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    statusCode);
            return scope.Close(Undefined());
        }

        if ((size_t)nameLength >= sizeof(nameBuffer)) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:name buffer too small: %lu bytes (need at least %u)\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    sizeof(nameBuffer), nameLength + 1);
            return scope.Close(Undefined());
        }

        columns[column - 1].type = columnType;
        columns[column - 1].name = String::New((char const*)nameBuffer);
    }

    statusCode = SQLFetch(hstmt);

    if (statusCode == SQL_NO_DATA) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:implement\n",
                __FILE__, __LINE__, __FUNCTION__);
        return scope.Close(Undefined());
    } else if (!SQL_SUCCEEDED(statusCode)) {
        if (statusCode == SQL_ERROR) {
            SQLCHAR     stateBuffer[6];
            SQLINTEGER  errorNumber;
            SQLCHAR  messageBuffer[128];
            SQLSMALLINT  length = 0;
            statusCode = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, stateBuffer, &errorNumber, messageBuffer, sizeof(messageBuffer), &length);
            if (!SQL_SUCCEEDED(statusCode)) {
                fprintf(stderr,
                        "%s:%d:%s():FIXME:handle status code: %d\n",
                        __FILE__, __LINE__, __FUNCTION__,
                        statusCode);
                return scope.Close(Undefined());
            }

            if ((size_t)length >= sizeof(messageBuffer)) {
                fprintf(stderr,
                        "%s:%d:%s():FIXME:increase buffer size to %u bytes (only have %lu bytes)\n",
                        __FILE__, __LINE__, __FUNCTION__,
                        length, sizeof(messageBuffer));
                return scope.Close(Undefined());
            }

            fprintf(stderr,
                    "%s:%d:%s():got error: %s\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    (char const*)messageBuffer);
        } else {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:handle status code: %d\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    statusCode);
        }
        return scope.Close(Undefined());
    } else {
        do {
            if (!args[1]->IsFunction()) {
	        statusCode = SQLFetch(hstmt);

                continue;
            }
            Handle<Object> row = Object::New();
            Handle<Value> argv[2] = {
                Local<Value>::New(Null()),
                row
            };
            for (column = 1; column <= columnCount; column++) {
                char         stringValue[128];
                int          integerValue = 0;
                SQLLEN       length = 0;
                Local<Value> value;

                switch (columns[column - 1].type) {
                case SQL_INTEGER:
                    statusCode = SQLGetData(hstmt, column, SQL_C_DEFAULT, &integerValue, 0, &length);

                    if (!SQL_SUCCEEDED(statusCode)) {
                        if (statusCode == SQL_ERROR) {
                            SQLCHAR     stateBuffer[6];
                            SQLINTEGER  errorNumber;
                            SQLCHAR  messageBuffer[128];
                            SQLSMALLINT  length = 0;
                            statusCode = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, stateBuffer, &errorNumber, messageBuffer, sizeof(messageBuffer), &length);
                            if (!SQL_SUCCEEDED(statusCode)) {
                                fprintf(stderr,
                                        "%s:%d:%s():FIXME:handle status code: %d\n",
                                        __FILE__, __LINE__, __FUNCTION__,
                                        statusCode);
                                return scope.Close(Undefined());
                            }

                            if ((size_t)length >= sizeof(messageBuffer)) {
                                fprintf(stderr,
                                        "%s:%d:%s():FIXME:increase buffer size to %u bytes (only have %lu bytes)\n",
                                        __FILE__, __LINE__, __FUNCTION__,
                                        length, sizeof(messageBuffer));
                                return scope.Close(Undefined());
                            }

                            fprintf(stderr,
                                    "%s:%d:%s():got error: %s\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    (char const*)messageBuffer);
                            return scope.Close(Undefined());
                        } else {
                            fprintf(stderr,
                                    "%s:%d:%s():FIXME:handle status code: %d\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    statusCode);
                        }
                        return scope.Close(Undefined());
                    }

                    if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else {
                        value = Local<Value>::New(NumberObject::New((double)integerValue));
                    }
                    break;
                case SQL_VARCHAR:
                    statusCode = SQLGetData(hstmt, column, SQL_C_CHAR, stringValue, sizeof(stringValue), &length);
                    if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return scope.Close(Undefined());
                    }

		    if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else if ((size_t)length >= sizeof(stringValue)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u+1 bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                length, sizeof(stringValue));
                        return scope.Close(Undefined());
                    } else {
                        value = Local<Value>::New(String::New(stringValue));
                    }
                    break;
                default:
                    fprintf(stderr,
                            "%s:%d:%s():FIXME:unexpected column type %s\n",
                            __FILE__, __LINE__, __FUNCTION__,
                            typeToString(columns[column - 1].type));
                    continue;
                }

                row->Set(columns[column - 1].name, value);
            }

            Local<Function>::Cast(args[1])->Call(args.This(), sizeof(argv) / sizeof(*argv), argv);

            statusCode = SQLFetch(hstmt);
        } while (statusCode != SQL_NO_DATA);
    }

        statusCode = SQLMoreResults(hstmt);
    } while (statusCode != SQL_NO_DATA);

    statusCode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return scope.Close(Undefined());
    }

    return scope.Close(Undefined());
}

Connection::~Connection() {
    SQLRETURN  statusCode;

    statusCode = SQLDisconnect(connection);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
    }

    statusCode = SQLFreeHandle(SQL_HANDLE_DBC, connection);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
    }

    statusCode = SQLFreeHandle(SQL_HANDLE_ENV, environment);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
    }
}

Persistent<Function> Connection::constructor;

void Connection::Init() {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("Connection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("execute"),
      FunctionTemplate::New(Execute)->GetFunction());

  constructor = Persistent<Function>::New(tpl->GetFunction());
}

Handle<Value> Connection::New(const Arguments& args) {
  HandleScope scope;

  Connection* obj = new Connection();
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
