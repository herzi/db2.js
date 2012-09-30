{
  "targets": [
    {
      "include_dirs": ["/opt/ibm/db2/V9.7/include"],
      "link_settings": {
        "libraries": [
          "-L/opt/ibm/db2/V9.7/lib64",
          "-ldb2"
        ],
      },
      "target_name": "db2",
      "sources": [ "src/binding.cc", "src/connection.cc", "src/connection.hh" ]
    }
  ]
}
