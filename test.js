var assert = require('assert');
var binding = require('./build/Release/db2');
assert.equal('world', binding.hello());
console.log('binding.hello() =', binding.hello());
