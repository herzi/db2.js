var db2 = require('./build/Release/db2');

var connection = db2.connect();
connection.execute();
