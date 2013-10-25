#define BUILDING_NODE_EXTENSION
#include <cstdint> // INT32_MAX
#include <node.h>
#include <sqlcli1.h>

#include "buffer-helper.hh"
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
    case SQL_ARD_TYPE:
        fprintf(stderr,
                "%s:%d:%s():FIXME:improve type information for ARD\n",
                __FILE__, __LINE__, __FUNCTION__);
        return "application row descriptor";
    default:
        fprintf(stderr,
                "%s:%d:%s():FIXME:unsupported type %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                type);
        return "unknown";
    }
}

static Local<Value> getException (SQLSMALLINT handleType,
                                  SQLHANDLE   handle,
                                  SQLRETURN const  statusCode,
                                  char const* function)
{
    HandleScope   scope;
    size_t        allocated = 256;
    char        * buffer = (char*)malloc(allocated);
    Local<Value>  exception;
    size_t        length;
    Local<Value>  status;

    SQLRETURN     internStatus = 0;
    SQLCHAR       state[7] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
    SQLINTEGER    nativeError = 0;
    SQLSMALLINT   written = 0;

    switch (statusCode) {
    case SQL_ERROR:
        status = String::New("error");
        internStatus = SQLGetDiagRec(handleType, handle, 1, state, &nativeError, NULL, 0, &written);
        allocated = written + 1;
        buffer = (char*)realloc(buffer, allocated);
        internStatus = SQLGetDiagRec(handleType, handle, 1, state, &nativeError, (SQLCHAR*)buffer, allocated, &written);
        if ((size_t)written >= allocated) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:increase buffer size to %lu+1 (from %lu)\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    (size_t)written, allocated);
        }
        if (!SQL_SUCCEEDED(internStatus)) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:unexpected status code: %d\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    internStatus);
        }
        exception = Exception::Error(String::New(buffer));
        {
            Local<Object> exceptionObject = exception->ToObject();
            exceptionObject->Set(String::New("nativeError"), Number::New(nativeError));
            exceptionObject->Set(String::New("sqlState"), String::New((char const*)state));
        }
        break;
    default:
        length = snprintf(buffer, allocated,
                          "unknown (%d)", statusCode);
        status = String::New(buffer);
        length = snprintf(buffer, allocated,
                          "%s:%d:%s():FIXME:unsupported statusCode: %d",
                          __FILE__, __LINE__, __FUNCTION__,
                          statusCode); // always NUL-terminated
        if (length >= allocated) {
            fprintf(stderr,
                    "%s:%d:%s():FIXME:increase buffer size to %lu+1 (from %lu)\n",
                    __FILE__, __LINE__, __FUNCTION__,
                    length, allocated);
        }
        exception = Exception::Error(String::New(buffer));
        break;
    }

    free(buffer);

    Local<Object> o = exception->ToObject();
    o->Set(String::New("status"), status);
    o->Set(String::New("statusCode"), Number::New(statusCode));

    // FIXME: add the status code
    // FIXME: add more information to the object
    return exception;
}

static char const* getSqlState (SQLSMALLINT  handleType,
                                SQLHANDLE    handle)
{
    SQLRETURN  internStatus;
    static SQLCHAR  state[7];

    bzero(state, sizeof(state));
    internStatus = SQLGetDiagRec(handleType, handle, 1, state, NULL, NULL, 0, NULL);
    if (!SQL_SUCCEEDED(internStatus)) {
        fprintf(stderr,
                "%s:%d:%s():FIXME:unexpected status code: %d\n",
                __FILE__, __LINE__, __FUNCTION__,
                internStatus);
    }

    return (char const*)state;
}
 /*************************************************************************
	The Following DATE/TIMESTAMP to V8 Date Conversion functions taken from 
	https://github.com/nearinfinity/node-oracle connection.cpp src file
    with a few changes to account for odd behavior conv from SQL_DATE_STRUCT.
	
	The Other option was to return a V8::STRING. That is what the node-postgres
	module does. Not a satisfying option to me.
  *************************************************************************/
void CallDateMethod(Local<Date> date, const char* methodName, int val) {
  Handle<Value> args[1];
  args[0] = Number::New(val);
  Local<Function>::Cast(date->Get(String::New(methodName)))->Call(date, 1, args);
}

Local<Date> DB2DateToV8Date(SQL_DATE_STRUCT* d) {

  Local<Date> date = Date::Cast(*Date::New(0.0));
  CallDateMethod(date, "setSeconds", 0);	//Strange behavior if sec,min,hrs not set
  CallDateMethod(date, "setMinutes",0);		// even with SQL_DATE_STRUCT
  CallDateMethod(date, "setHours", 0);
  CallDateMethod(date, "setDate", d->day);
  CallDateMethod(date, "setMonth", d->month - 1);
  CallDateMethod(date, "setFullYear", d->year);
  return date;
}
Local<Date> DB2TimestampToV8Date(SQL_TIMESTAMP_STRUCT* d) {
  
  Local<Date> date = Date::Cast(*Date::New(0.0));
  CallDateMethod(date, "setSeconds", d->second);
  CallDateMethod(date, "setMinutes",d->minute);
  CallDateMethod(date, "setHours", d->hour);
  CallDateMethod(date, "setDate", d->day);
  CallDateMethod(date, "setMonth", d->month - 1);
  CallDateMethod(date, "setFullYear", d->year);
  return date;
}



Connection::Connection(char const* server, 
                       char const* user = NULL, 
                       char const* password = NULL) {
    SQLRETURN  statusCode;
   
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
    statusCode = SQLConnect(connection, (SQLCHAR*)server, SQL_NTS, (SQLCHAR*)user, SQL_NTS, (SQLCHAR*)password, SQL_NTS);

    if (!SQL_SUCCEEDED(statusCode)) {
        ThrowException(getException(SQL_HANDLE_DBC, connection, statusCode, "SQLConnect()"));
    }
}

Handle<Value> Connection::Execute (const Arguments& args) {
    HandleScope scope;
    size_t      indexCallback = 1;

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
        ThrowException(getException(SQL_HANDLE_STMT, hstmt, statusCode, "SQLPrepare()"));
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

    void**params = NULL;
    if (parameterCount) {
        params = (void**)malloc(sizeof(void*) * parameterCount);
        bzero(params, sizeof(void*) * parameterCount);
        indexCallback += parameterCount;
        if ((size_t)args.Length() < indexCallback) {
            ThrowException(Exception::Error(String::New("FIXME: improve binding argument length exception")));
            return scope.Close(Undefined());
        }

        for (SQLSMALLINT parameter = 1; parameter <= parameterCount; parameter++) {
            if (args[parameter]->IsString()) {
                String::Utf8Value  value(args[parameter]);
                params[parameter - 1] = strdup(*value);
                statusCode = SQLBindParameter(hstmt, parameter, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, INT32_MAX /* column size */, 0, (char*)params[parameter - 1], value.length(), NULL);
            } else {
                ThrowException(Exception::Error(String::New("handle parameter binding")));
                return scope.Close(Undefined());
            }

            if (!SQL_SUCCEEDED(statusCode)) {
                ThrowException(getException(SQL_HANDLE_STMT, hstmt, statusCode, "SQLBindParameter()"));
                return scope.Close(Undefined());
            }
        }
    }

    statusCode = SQLExecute(hstmt);
    if (!SQL_SUCCEEDED(statusCode)) {
        ThrowException(getException(SQL_HANDLE_STMT, hstmt, statusCode, "SQLExecute()"));
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
        } columns[256];			// 256 column limit
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

        if (!SQL_SUCCEEDED(statusCode)) {
            if (statusCode == SQL_ERROR && !strcmp(getSqlState(SQL_HANDLE_STMT, hstmt), "24000")) {
                // SQLFetch() was called for a non-query statement (e.g. "CREATE"/"DROP"); treat this as "no data"
                // FIXME: try to avoid calling SQLFetch() in these cases
                statusCode = SQL_NO_DATA;
            }

            if (statusCode != SQL_NO_DATA) {
                ThrowException(getException(SQL_HANDLE_STMT, hstmt, statusCode, "SQLFetch()"));
                return scope.Close(Undefined());
            }
        }
        
        /* here we can only have SQL_NO_DATA || SQL_SUCCEEDED() */
        while (SQL_SUCCEEDED(statusCode)) {
            if (!args[indexCallback]->IsFunction()) {
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
				char		 stringTime[8];
                int          integerValue = 0;
				double			 dblValue = 0.0;
				SQL_DATE_STRUCT  date;
				SQL_TIMESTAMP_STRUCT datetime;
                SQL_TIME_STRUCT time;
                SQLINTEGER       length = 0;
                Local<Value> value;
				SQLINTEGER  lobLoc ;
				SQLINTEGER *loc_ind = 0;
                size_t   allocated;
                

                switch (columns[column - 1].type) {
				case SQL_DECIMAL:
				case SQL_REAL:
				case SQL_NUMERIC:
				case SQL_DOUBLE:
				case SQL_FLOAT:
					statusCode = SQLGetData(hstmt, column, SQL_C_DOUBLE, &dblValue, 0, &length);

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
                        value = Local<Value>::New(NumberObject::New((double)dblValue));
                    }
                    break;
				
				case SQL_DECFLOAT:
					statusCode = SQLGetData(hstmt, column, SQL_C_DEFAULT, &dblValue, 0, &length);

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
                        value = Local<Value>::New(NumberObject::New((double)dblValue));
                    }
                    break;
				case SQL_SMALLINT:
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
				case SQL_CHAR:
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
				case SQL_TYPE_TIME:
					statusCode = SQLGetData(hstmt,column,SQL_C_TYPE_TIME,(SQLPOINTER)&time,0,&length);
					if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return scope.Close(Undefined());
                    }

                    if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else if ((size_t)length > sizeof(time)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u+1 bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                length, sizeof(time));
                        return scope.Close(Undefined());
                    } else {
						int cx = snprintf (stringTime,8,"%02d:%02d:%02d",time.hour,time.minute,time.second);
						if(cx>8){
							fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u+1 bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                cx, 8);
                        return scope.Close(Undefined());
						}
                        value = Local<Value>::New(String::New(stringTime));
                    }
                    break;
				case SQL_TYPE_TIMESTAMP:
					statusCode = SQLGetData(hstmt,column,SQL_C_TYPE_TIMESTAMP,(SQLPOINTER)&datetime,0,&length);
					if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return scope.Close(Undefined());
                    }

                    if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else if ((size_t)length > sizeof(datetime)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u+1 bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                length, sizeof(date));
                        return scope.Close(Undefined());
                    } else {
                        value = DB2TimestampToV8Date(&datetime);
                    }
                    break;
				case SQL_TYPE_DATE:
					statusCode = SQLGetData(hstmt,column,SQL_C_TYPE_DATE,(SQLPOINTER)&date,0,&length);
					if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return scope.Close(Undefined());
                    }

                    if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else if ((size_t)length > sizeof(date)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:increase buffer size to %u+1 bytes (from %lu bytes)\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                length, sizeof(date));
                        return scope.Close(Undefined());
                    } else {
                        value = DB2DateToV8Date(&date);
                    }
                    break;
				case SQL_XML:
				case SQL_BLOB:
                case SQL_ARD_TYPE:
                    statusCode = SQLGetData(hstmt, column, SQL_C_BINARY, NULL, 0, &length);
                    if (!SQL_SUCCEEDED(statusCode)) {
                        fprintf(stderr,
                                "%s:%d:%s():FIXME:handle status code: %d\n",
                                __FILE__, __LINE__, __FUNCTION__,
                                statusCode);
                        return scope.Close(Undefined());
                    } else if (length == SQL_NULL_DATA) {
                        value = Local<Value>::New(Null());
                    } else {
                        allocated = length + 1;
                        SQLCHAR* data = (SQLCHAR*)malloc(allocated);
                        statusCode = SQLGetData(hstmt, column, SQL_C_BINARY, data, allocated, &length);
                        if (!SQL_SUCCEEDED(statusCode)) {
                            fprintf(stderr,
                                    "%s:%d:%s():FIXME:handle status code: %d\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    statusCode);
                            return scope.Close(Undefined());
                        }
                        if ((size_t)length >= allocated) {
                            fprintf(stderr,
                                    "%s:%d:%s():FIXME:still too small?: %lu (looks like we need %u)\n",
                                    __FILE__, __LINE__, __FUNCTION__,
                                    allocated, length + 1);
                            return scope.Close(Undefined());
                        }
                        // FIXME: having a chain of reusable buffers would be nicer, I guess
                        data[allocated - 1] = '\0'; // FIXME: research whether this is neccessary
                        value = makeBuffer((char*)data, length);
                        free(data);
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

            Local<Function>::Cast(args[indexCallback])->Call(args.This(), sizeof(argv) / sizeof(*argv), argv);

            statusCode = SQLFetch(hstmt);
        }

        if (args[indexCallback]->IsFunction()) {
            Local<Value> argv[] = {
                Local<Value>::New(Null()),
                Local<Value>::New(Null())
            };
            Local<Function>::Cast(args[indexCallback])->Call(args.This(), sizeof(argv) / sizeof(*argv), argv);
        }

        statusCode = SQLMoreResults(hstmt);
    } while (statusCode != SQL_NO_DATA);

    for (SQLSMALLINT parameter = 0; parameter < parameterCount; parameter += 1) {
        if (params[parameter]) {
            free(params[parameter]);
        }
    }
    if (params) {
        free(params);
    }

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

    if (args.Length() != 1 && args.Length() != 3) {
        // FIXME: specify the number given and the number expected
        ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
        return scope.Close(Undefined());
    }

    String::Utf8Value server(args[0]->ToString());
    Local<String> tUser;
    Local<String> tPassword;
    if (args.Length() == 3) {
        tUser = args[1]->ToString();
        tPassword = args[2]->ToString();
    }
    String::Utf8Value user(tUser);
    String::Utf8Value password(tPassword);
    
    Connection* obj = new Connection(*server, *user, *password);
    obj->Wrap(args.This());

    return args.This();
}

Handle<Value> Connection::Connect(const Arguments& args) {
    HandleScope scope;
#ifdef _MSC_VER
//	Handle<Value>  argv = (Handle<Value>)_alloca(args.Length() * sizeof(Value));
	Handle<Value> *	argv;
#else
   Handle<Value> argv[args.Length()];
#endif
	
    size_t  argc;
    for (argc = 0; argc < (size_t)args.Length(); argc++) {
        argv[argc] = args[argc];
    }
    Local<Object> instance = constructor->NewInstance(argc, argv);

    return scope.Close(instance);
}

/* vim:set sw=4 et: */
