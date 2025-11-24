// Import the Clay package
var Clay = require('pebble-clay');

// Load our Clay configuration file
var clayConfig = require('./config');

// Initialize Clay
// Clay will automatically handle 'showConfiguration' and 'webviewclosed' events
// and send settings to the watchface using the messageKeys defined in package.json
var clay = new Clay(clayConfig);
