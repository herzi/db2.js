Installation instructions for db2.js
====================================

Make sure you have 'node-gyp' installed:

    npm install -g node-gyp

Configuration
-------------

Configure the package using node-gyp:

    node-gyp configure

If you're running a "pre" version of node, you need to pass the node folder, e.g.:

    node-gyp configure --nodedir "$HOME/node"

Compilation
-----------

Compile the package using node-gyp:

    node-gyp build

Installation
------------

You can easily install this package with npm:

    npm install db2

Testing
-------

The current test code depends on a database I have set up within my development vm. The code will be updated to use the sample database from DB2 later.

Development
-----------
Requires gcc and g++, node-gyp, node.js.
