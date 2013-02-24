{
  "targets": [
    {
      "cflags": [
        "-std=c++0x",
        "-Wall",
        "-g",
        "-Wextra"
      ],
		'conditions': [
        ['OS=="win"', {
          'libraries': [
            '-l C:\Program Files\IBM\SQLLIB\lib\db2api.lib'
          ],
          'include_dirs': [
            'C:\Program Files\IBM\SQLLIB\include'
          ]}],
		['OS!="win"', {
			"link_settings": {
				"libraries": [
				"-L/opt/ibm/db2/V9.7/lib64",
				"-ldb2"
					]
				}
				
		
			}
		]
		],
      "target_name": "db2",
      "sources": [ "src/binding.cc", "src/connection.cc", "src/connection.hh" ]
    }
  ]
}