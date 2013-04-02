var db2;
try {
   db2 = require('db2');
} catch (exception) {
   db2 = require('./db2');
}


function dataParse(o){
var rStr='';
if(o!=null){
	var Keys=Object.keys(o) //returns column names
	for (var i=0; i<Keys.length; i++){
			if(i==0){rStr+='{';}
			
			if(Buffer.isBuffer(o[Keys[i]])){/* Is this a BLOB/XML/CLOB column returned in buffer*/
			if(i==0){
				rStr+='"'+Keys[i]+'":'+JSON.stringify(o[Keys[i]].toString());
				}else{
				rStr+=',"'+Keys[i]+'":'+JSON.stringify(o[Keys[i]].toString());
				}
			}else{
			if(i==0){
				rStr+='"'+Keys[i]+'":'+JSON.stringify(o[Keys[i]]);
			}else{
				rStr+=',"'+Keys[i]+'":'+JSON.stringify(o[Keys[i]]); //multiple all property values of object
				}
			//console.log(rStr.length);
			}
			if(i==Keys.length-1){rStr+='},';}
		}
	}else{
			//rStr[rStr.length-2]='}';
			rStr+='}';
			
		}
	return rStr.substr(0,rStr.length-1);
};
var connection = db2.connect("SAMPLE");

/*Loop through rows and return in Array*/
var GetSQLDataParse = function(sql){
 var datadic=[];
  if(sql==null){return null;}
	connection.execute(sql,function(e,o){
	if(!e){
		if(o!=null)
			datadic.push(dataParse(o));
		   
		}else{
			
			}
})
  return datadic;
};
console.log(' Current System time '+GetSQLDataParse("SELECT CURRENT TIMESTAMP FROM SYSIBM.SYSDUMMY1"));
console.log('----------------------------------------');
var sqlString = "SELECT e.EMPNO,e.FIRSTNME,e.MIDINIT as initial,e.LASTNAME,d.DEPTNAME,e.HIREDATE,e.SALARY,e.COMM FROM EMPLOYEE as e"+
						" LEFT JOIN DEPARTMENT as d ON (e.WORKDEPT=d.DEPTNO)"
/*Pass in String of SQL*/
console.log(GetSQLDataParse(sqlString));
console.log('----------------------------------------');

/*Use the built in forEach to filter SALES > 7*/
GetSQLDataParse("SELECT * FROM SALES").forEach(function(element){var f=JSON.parse(element);if(f.SALES>7) console.log(f)});


// vim:set sw=4 et:
