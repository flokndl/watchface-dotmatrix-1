// This file is included with the Pebble SDK
// It's optional and can be used for JavaScript functionality

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Show configuration page
  Pebble.openURL('https://example.com/config.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));
  
  // Send settings to watchface
  if (configData.backgroundColor) {
    Pebble.sendAppMessage({
      'backgroundColor': parseInt(configData.backgroundColor, 16)
    }, function(e) {
      console.log('Settings sent successfully');
    }, function(e) {
      console.log('Settings failed to send');
    });
  }
});

