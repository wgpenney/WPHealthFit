Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS is ready.');
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  console.log('Opening Configuration.');
  Pebble.openURL('https://www.googledrive.com/host/0B125SeGC8a2GZkdNRlRhNGMxYlE/');
});

Pebble.addEventListener('webviewclosed',  function(e) {
  if (e.response !== undefined && e.response !== '' && e.response !== 'CANCELLED') {
    console.log('Configuration closed!');
    console.log("Response = " + e.response.length + "   " + e.response);
    var options = JSON.parse(decodeURIComponent(e.response));
    console.log('Configuration window returned: ', JSON.stringify(options));
    
    Pebble.sendAppMessage(options,
                         function (e) {
                            console.log("Successfully delivered message with transactionId=" + e.data.transactionId + " " +JSON.stringify(options));
                         },
                         function (e) {
                           console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.data.error.message);
                         }
                         );
  } else if (e.response === 'CANCELLED') {
        console.log("Android misbehaving on save due to embedded space in e.response... ignoring");
  }
} 
);