var db2 = require('./build/Release/db2');

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

// vim::set sw=4 et
