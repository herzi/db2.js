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
    SQLHANDLE  hdbc;
    SQLHANDLE  hstmt;
    uint8_t* password = NULL;
    uint8_t* server = (uint8_t*)"olaptest";
    uint8_t* user = NULL;

    statusCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &environment);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    // FIXME: check whether we want to use SQLSetEnvAttr();

    statusCode = SQLAllocHandle(SQL_HANDLE_DBC, environment, &hdbc);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    // FIXME: check whether we want to use SQLSetConnectAttr();

    // FIXME: also implement this with: SQLDriverConnect(hdbc, (SQLHWND)NULL, "DSN=SAMPLE;UID=;PWD=;", NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    statusCode = SQLConnect(hdbc, server, SQL_NTS, user, SQL_NTS, password, SQL_NTS);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    statusCode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    // FIXME: optional: SQLSetStmtAttr()

    /*
     * SELECT * FROM requests
     * SELECT httpMethod FROM requests
     * SELECT count(httpMethod) FROM requests GROUP BY httpMethod
     * SELECT httpMethod, count(httpMethod) FROM requests GROUP BY httpMethod
     * SELECT httpMethod, count(*) FROM requests GROUP BY httpMethod
     * SELECT httpMethod, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL))
     * SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode))
     * SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode), (httpMethod))
     * SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode), (httpMethod)) ORDER BY httpMethod
     * SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod
     * SELECT httpMethod, httpURL, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod
     * SELECT httpMethod, httpURL, statusCode, sum(bodySize) AS data FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod
     */
#if 1
    statusCode = SQLPrepare(hstmt, (SQLCHAR *) "SELECT * FROM requests WHERE httpMethod = ?; SELECT httpMethod FROM requests", SQL_NTS);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    // FIXME: we should use SQLNumParams() to make sure the developer is passing in the correct mount of parameters
    char stringParam[] = "POST";
    statusCode = SQLBindParameter(hstmt, 1 /* argumentNumber */, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 20, 0, stringParam, 20, NULL);
    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    statusCode = SQLExecute(hstmt);
#else
    statusCode = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM requests WHERE httpMethod = 'POST'; SELECT httpMethod FROM requests", SQL_NTS);
#endif

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    do {
    SQLSMALLINT  columnCount = 0;
    statusCode = SQLNumResultCols(hstmt, &columnCount);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    SQLSMALLINT  columns[11];
    if ((size_t)columnCount > sizeof(columns) / sizeof(*columns)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:increase column count to %u (from %lu)\n",
                __FILE__, __LINE__, __FUNCTION__,
                columnCount, sizeof(columns) / sizeof(*columns));
        return;
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
            return;
        }

        if ((size_t)nameLength >= sizeof(nameBuffer)) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:name buffer too small: %lu bytes (need at least %u)\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    sizeof(nameBuffer), nameLength + 1);
            return;
        }

        columns[column - 1] = columnType;
        printf("Column %d: %s (%s), %d, %d, (%s)\n",
               column,
               (char const*)nameBuffer,
               typeToString(columnType),
               precision, decimals,
               nullable == SQL_NULLABLE ? "nullable" : "not nullable");
    }

    statusCode = SQLFetch(hstmt);

    if (statusCode == SQL_NO_DATA) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:implement\n",
                __FILE__, __LINE__, __FUNCTION__);
        return;
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
                return;
            }

            if ((size_t)length >= sizeof(messageBuffer)) {
                fprintf(stderr,
                        "%s:%d:%s():FIXME:increase buffer size to %u bytes (only have %lu bytes)\n",
                        __FILE__, __LINE__, __FUNCTION__,
                        length, sizeof(messageBuffer));
                return;
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
        return;
    } else {
        do {
            printf("{\n");
            for (column = 1; column <= columnCount; column++) {
                char         stringValue[128];
                int          integerValue = 0;
                SQLLEN       length = 0;

                switch (columns[column - 1]) {
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
                                return;
                            }

                            if ((size_t)length >= sizeof(messageBuffer)) {
                                fprintf(stderr,
                                        "%s:%d:%s():FIXME:increase buffer size to %u bytes (only have %lu bytes)\n",
                                        __FILE__, __LINE__, __FUNCTION__,
                                        length, sizeof(messageBuffer));
                                return;
                            }

                            fprintf(stderr,
                                    "%s:%d:%s():got error: %s\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    (char const*)messageBuffer);
                            return;
                        } else {
                            fprintf(stderr,
                                    "%s:%d:%s():FIXME:handle status code: %d\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    statusCode);
                        }
                        return;
                    }

                    if (length == SQL_NULL_DATA) {
                        printf("  NULL%s\n", column == columnCount ? "" : ",");
                    } else {
                        printf("  %d%s\n", integerValue, column == columnCount ? "" : ",");
                    }
                    break;
                case SQL_VARCHAR:
                    statusCode = SQLGetData(hstmt, column, SQL_C_CHAR, stringValue, sizeof(stringValue), &length);
                    if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return;
                    }

                    if ((size_t)length >= sizeof(stringValue)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                length + 1, sizeof(stringValue));
                        return;
                    } else if (length == SQL_NULL_DATA) {
                        printf("  NULL%s\n", column == columnCount ? "" : ",");
                    } else {
                        printf("  \"%s\"%s\n", stringValue, column == columnCount ? "" : ",");
                    }
                    break;
                default:
                    fprintf(stderr,
                            "%s:%d:%s():FIXME:unexpected column type %s\n",
                            __FILE__, __LINE__, __FUNCTION__,
                            typeToString(columns[column - 1]));
                    continue;
                }
            }
            printf("}\n");

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
        return;
    }

    statusCode = SQLDisconnect(hdbc);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }

    statusCode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);

    if (!SQL_SUCCEEDED(statusCode)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:handle status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                statusCode);
        return;
    }
}

Connection::~Connection() {
    SQLRETURN  statusCode;

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
