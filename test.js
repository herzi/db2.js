var db2;
try {
   db2 = require('db2');
} catch (exception) {
   db2 = require('./db2');
}

var connection = db2.connect("olapTest");

connection.execute("SELECT * FROM requests", function () {
    //*
    console.log("Got %d arguments:", arguments.length);
    for (var i = 0; i < arguments.length; i++) {
        console.log(" %d: %s", i, JSON.stringify(arguments[i]));
    }
    // */
});
//*
//connection.execute("SELECT * FROM requests FETCH FIRST 10 ROWS ONLY", function (error, result) {if (error) {return console.log(error);} console.log(require('util').inspect(result));});

var testCreate = function () {
    var run = false;
    try {
	connection.execute("CREATE TABLE sliff (sloff CLOB)", function (error, row) {
	    if (error) {
		throw error;
	    }

	    if (row) {
		throw new Error("CREATE TABLE cannot produce a row result");
	    }

	    run = true;
	});
	if (!run) {
	    throw new Error("the CREATE TABLE callback needs to be run");
	}

        connection.execute("INSERT INTO sliff VALUES (?)", 'sloff', function (error, row) {
            if (error) {
                throw error;
            }
            if (row) {
                throw new Error('row must be null');
            }
        });
        connection.execute("SELECT * FROM sliff", function (error, row) {
            if (error) {
                throw error;
            }

            if (row && row.SLOFF.toString() !== 'sloff') {
                throw new Error('row.SLOFF: ' + JSON.stringify(row.SLOFF.toString()) + ' !== "sloff"');
            }
        });
    } catch (exception) {
        if (exception.nativeError === -601) { // object already exists
            connection.execute("DROP TABLE sliff");
            testCreate();
        } else {
	    console.error("%s: %s", exception, JSON.stringify(exception));
            throw exception;
        }
    }
};
testCreate();

testCreate = function () {
    try {
        connection.execute("CREATE TABLE argumentBinding (cString VARCHAR(255))", function (error, row) {
            if (error) {
	        throw error;
	    }

	    if (row !== null) {
	        throw new Error("row must be null: " + JSON.stringify(row));
	    }
        });
        // FIXME: write a test case for inserting too long data
        connection.execute("INSERT INTO argumentBinding VALUES (?)", 'test argument', function (error, row) {
            if (error) {
                throw error;
            }
            if (row) {
                throw new Error('row must be null');
            }
        });
        connection.execute("SELECT * FROM argumentBinding", function (error, row) {
            if (error) {
                throw error;
            }

            if (row && row.CSTRING.toString() !== 'test argument') {
                throw new Error('row.CSTRING: ' + JSON.stringify(row.CSTRING.toString()) + ' !== "sloff"');
            }
        });
    } catch (exception) {
        if (exception.nativeError === -601) { // object already exists
            connection.execute("DROP TABLE argumentBinding");
            return testCreate();
        }
        throw exception;
    }
};
testCreate();

connection.execute("SELECT httpMethod FROM requests");
connection.execute("SELECT count(httpMethod) FROM requests GROUP BY httpMethod");
connection.execute("SELECT httpMethod, count(httpMethod) FROM requests GROUP BY httpMethod");
connection.execute("SELECT httpMethod, count(*) FROM requests GROUP BY httpMethod");
connection.execute("SELECT httpMethod, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL))");
connection.execute("SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode))");
connection.execute("SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode), (httpMethod))");
connection.execute("SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, statusCode), (httpMethod)) ORDER BY httpMethod");
connection.execute("SELECT httpMethod, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod");
connection.execute("SELECT httpMethod, httpURL, statusCode, count(*) AS count FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod");
connection.execute("SELECT httpMethod, httpURL, statusCode, sum(bodySize) AS data FROM requests GROUP BY GROUPING SETS ((httpMethod, httpURL, statusCode), (httpMethod, httpURL), (httpMethod)) ORDER BY httpMethod");
// */

// vim:set sw=4 et:
