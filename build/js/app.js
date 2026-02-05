var Config = {
  OWNTONE_BASE: "http://owntone.local:3689",
  MAX_RESULTS: 8
};

// Message key constants (must match appinfo.json)
var MessageKeys = {
  CMD: 0,
  TYPE: 1,
  QUERY: 2,
  URI: 3,
  VOLUME: 4,
  OUTPUT_ID: 5,
  
  RESULT_COUNT: 10,
  RESULT_TITLE_BASE: 20,
  RESULT_URI_BASE: 30,
  
  PLAYER_STATE: 40,
  PLAYER_TRACK: 41,
  PLAYER_ARTIST: 42,
  PLAYER_ALBUM: 43,
  PLAYER_VOLUME: 44,
  
  OUTPUT_COUNT: 50,
  OUTPUT_NAME_BASE: 60,
  OUTPUT_ID_BASE: 70,
  OUTPUT_VOLUME_BASE: 80,
  OUTPUT_ENABLED_BASE: 90,
  
  STATUS: 100
};

// Command types
var Commands = {
  GET_PLAYER_STATE: 1,
  PLAY_PAUSE: 2,
  NEXT: 3,
  PREVIOUS: 4,
  SET_VOLUME: 5,
  SEARCH: 6,
  RANDOM: 7,
  ADD_TO_QUEUE: 8,
  GET_OUTPUTS: 9,
  SET_OUTPUT_EXCLUSIVE: 10,
  TOGGLE_OUTPUT: 11,
  SET_OUTPUT_VOLUME: 12
};

// Content types
var ContentTypes = {
  PLAYLIST: 0,
  ARTIST: 1,
  ALBUM: 2
};

var ContentTypeNames = ["playlist", "artist", "album"];

function sendToPebble(dict) {
  Pebble.sendAppMessage(dict, 
    function() { console.log('Message sent successfully'); },
    function(e) { console.log('Message failed: ' + JSON.stringify(e)); }
  );
}

function httpGet(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10000;
  xhr.open('GET', url, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      callback(xhr.status, xhr.responseText);
    }
  };
  xhr.send(null);
}

function httpPut(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10000;
  xhr.open('PUT', url, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      callback(xhr.status, xhr.responseText);
    }
  };
  xhr.send(null);
}

function httpPost(url, callback) {
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10000;
  xhr.open('POST', url, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      callback(xhr.status, xhr.responseText);
    }
  };
  xhr.send(null);
}

// API Handlers
function getPlayerState() {
  httpGet(Config.OWNTONE_BASE + '/api/player', function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var dict = {};
        
        // Map player state
        var state = 0; // stopped
        if (data.state === 'play') state = 1;
        else if (data.state === 'pause') state = 2;
        
        dict[MessageKeys.PLAYER_STATE] = state;
        dict[MessageKeys.PLAYER_VOLUME] = data.volume || 50;
        
        // Get current track info
        getCurrentTrack(function(trackDict) {
          for (var key in trackDict) {
            dict[key] = trackDict[key];
          }
          sendToPebble(dict);
        });
      } catch (e) {
        console.log('Error parsing player state: ' + e);
      }
    }
  });
}

function getCurrentTrack(callback) {
  httpGet(Config.OWNTONE_BASE + '/api/queue?id=now_playing', function(status, response) {
    var dict = {};
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        if (data.items && data.items.length > 0) {
          var track = data.items[0];
          dict[MessageKeys.PLAYER_TRACK] = track.title || 'Unknown';
          dict[MessageKeys.PLAYER_ARTIST] = track.artist || 'Unknown';
          dict[MessageKeys.PLAYER_ALBUM] = track.album || 'Unknown';
        } else {
          dict[MessageKeys.PLAYER_TRACK] = 'No track';
          dict[MessageKeys.PLAYER_ARTIST] = '';
          dict[MessageKeys.PLAYER_ALBUM] = '';
        }
      } catch (e) {
        console.log('Error parsing current track: ' + e);
        dict[MessageKeys.PLAYER_TRACK] = 'Error';
        dict[MessageKeys.PLAYER_ARTIST] = '';
        dict[MessageKeys.PLAYER_ALBUM] = '';
      }
    } else {
      dict[MessageKeys.PLAYER_TRACK] = 'No track';
      dict[MessageKeys.PLAYER_ARTIST] = '';
      dict[MessageKeys.PLAYER_ALBUM] = '';
    }
    callback(dict);
  });
}

function playPause() {
  httpPut(Config.OWNTONE_BASE + '/api/player/toggle', function(status, response) {
    console.log('Play/Pause: ' + status);
    getPlayerState(); // Refresh state
  });
}

function next() {
  httpPut(Config.OWNTONE_BASE + '/api/player/next', function(status, response) {
    console.log('Next: ' + status);
    setTimeout(getPlayerState, 500); // Delay to let track change
  });
}

function previous() {
  httpPut(Config.OWNTONE_BASE + '/api/player/previous', function(status, response) {
    console.log('Previous: ' + status);
    setTimeout(getPlayerState, 500);
  });
}

function setVolume(volume) {
  httpPut(Config.OWNTONE_BASE + '/api/player/volume?volume=' + volume, function(status, response) {
    console.log('Set volume to ' + volume + ': ' + status);
  });
}

function search(type, query) {
  var typeName = ContentTypeNames[type] || 'playlist';
  var url = Config.OWNTONE_BASE + '/api/search?type=' + encodeURIComponent(typeName) + 
            '&query=' + encodeURIComponent(query) + '&limit=' + Config.MAX_RESULTS;
  
  httpGet(url, function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var items = [];
        
        // Extract items based on type
        if (data.playlists && data.playlists.items) {
          items = data.playlists.items;
        } else if (data.artists && data.artists.items) {
          items = data.artists.items;
        } else if (data.albums && data.albums.items) {
          items = data.albums.items;
        } else if (data.tracks && data.tracks.items) {
          items = data.tracks.items;
        }
        
        sendResults(items);
      } catch (e) {
        console.log('Error parsing search results: ' + e);
        sendResults([]);
      }
    } else {
      sendResults([]);
    }
  });
}

function random(type) {
  var typeName = ContentTypeNames[type] || 'playlist';
  var url;
  
  if (typeName === 'playlist') {
    var letters = 'abcdefghijklmnopqrstuvwxyz';
    var randomLetter = letters[Math.floor(Math.random() * letters.length)];
    var randomOffset = Math.floor(Math.random() * 20);
    url = Config.OWNTONE_BASE + '/api/search?type=playlist&query=' + randomLetter + 
          '&limit=' + Config.MAX_RESULTS + '&offset=' + randomOffset;
  } else {
    url = Config.OWNTONE_BASE + '/api/search?type=' + encodeURIComponent(typeName) + 
          '&expression=' + encodeURIComponent('media_kind is music order by random desc') + 
          '&limit=' + Config.MAX_RESULTS;
  }
  
  httpGet(url, function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var items = [];
        
        if (data.playlists && data.playlists.items) {
          items = data.playlists.items;
        } else if (data.artists && data.artists.items) {
          items = data.artists.items;
        } else if (data.albums && data.albums.items) {
          items = data.albums.items;
        }
        
        sendResults(items);
      } catch (e) {
        console.log('Error parsing random results: ' + e);
        sendResults([]);
      }
    } else {
      sendResults([]);
    }
  });
}

function sendResults(items) {
  var dict = {};
  dict[MessageKeys.RESULT_COUNT] = items.length;
  
  for (var i = 0; i < items.length && i < Config.MAX_RESULTS; i++) {
    var title = items[i].name || items[i].title || 'Unknown';
    var uri = items[i].uri || items[i].path || '';
    
    dict[MessageKeys.RESULT_TITLE_BASE + i] = title;
    dict[MessageKeys.RESULT_URI_BASE + i] = uri;
  }
  
  sendToPebble(dict);
}

function addToQueue(uri, type) {
  var shuffle = 'true';
  if (type === ContentTypes.ALBUM) shuffle = 'false';
  
  var url = Config.OWNTONE_BASE + '/api/queue/items/add?uris=' + encodeURIComponent(uri) + 
            '&clear=true&playback=start&shuffle=' + shuffle;
  
  httpPost(url, function(status, response) {
    console.log('Add to queue: ' + status);
    if (status === 200 || status === 204) {
      setTimeout(getPlayerState, 500);
    }
  });
}

function getOutputs() {
  httpGet(Config.OWNTONE_BASE + '/api/outputs', function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var outputs = data.outputs || [];
        
        var dict = {};
        dict[MessageKeys.OUTPUT_COUNT] = Math.min(outputs.length, Config.MAX_RESULTS);
        
        for (var i = 0; i < outputs.length && i < Config.MAX_RESULTS; i++) {
          dict[MessageKeys.OUTPUT_NAME_BASE + i] = outputs[i].name || 'Unknown';
          dict[MessageKeys.OUTPUT_ID_BASE + i] = String(outputs[i].id || '0');
          dict[MessageKeys.OUTPUT_VOLUME_BASE + i] = outputs[i].volume || 0;
          dict[MessageKeys.OUTPUT_ENABLED_BASE + i] = outputs[i].selected ? 1 : 0;
        }
        
        sendToPebble(dict);
      } catch (e) {
        console.log('Error parsing outputs: ' + e);
      }
    }
  });
}

function setOutputExclusive(outputId) {
  // First get all outputs, disable them, then enable only the selected one
  httpGet(Config.OWNTONE_BASE + '/api/outputs', function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var outputs = data.outputs || [];
        
        // Disable all outputs except the selected one
        outputs.forEach(function(output) {
          var shouldEnable = String(output.id) === outputId;
          httpPut(Config.OWNTONE_BASE + '/api/outputs/' + output.id + '?selected=' + shouldEnable, function() {});
        });
        
        // Start playback
        setTimeout(function() {
          httpPut(Config.OWNTONE_BASE + '/api/player/play', function() {
            getOutputs();
          });
        }, 500);
      } catch (e) {
        console.log('Error setting exclusive output: ' + e);
      }
    }
  });
}

function toggleOutput(outputId) {
  httpGet(Config.OWNTONE_BASE + '/api/outputs/' + outputId, function(status, response) {
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var newState = !data.selected;
        
        httpPut(Config.OWNTONE_BASE + '/api/outputs/' + outputId + '?selected=' + newState, function() {
          getOutputs();
        });
      } catch (e) {
        console.log('Error toggling output: ' + e);
      }
    }
  });
}

function setOutputVolume(outputId, volume) {
  httpPut(Config.OWNTONE_BASE + '/api/outputs/' + outputId + '?volume=' + volume, function(status, response) {
    console.log('Set output volume: ' + status);
  });
}

// Pebble event handlers
Pebble.addEventListener('ready', function(e) {
  console.log('OwnTone Remote JS ready');
  console.log('Server: ' + Config.OWNTONE_BASE);
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload;
  console.log('Received message: ' + JSON.stringify(payload));
  
  if (!payload) {
    console.log('No payload in message');
    return;
  }
  
  var cmd = payload[MessageKeys.CMD];
  console.log('Command: ' + cmd);
  
  switch (cmd) {
    case Commands.GET_PLAYER_STATE:
      getPlayerState();
      break;
      
    case Commands.PLAY_PAUSE:
      playPause();
      break;
      
    case Commands.NEXT:
      next();
      break;
      
    case Commands.PREVIOUS:
      previous();
      break;
      
    case Commands.SET_VOLUME:
      setVolume(payload[MessageKeys.VOLUME]);
      break;
      
    case Commands.SEARCH:
      search(payload[MessageKeys.TYPE], payload[MessageKeys.QUERY]);
      break;
      
    case Commands.RANDOM:
      random(payload[MessageKeys.TYPE]);
      break;
      
    case Commands.ADD_TO_QUEUE:
      addToQueue(payload[MessageKeys.URI], payload[MessageKeys.TYPE]);
      break;
      
    case Commands.GET_OUTPUTS:
      getOutputs();
      break;
      
    case Commands.SET_OUTPUT_EXCLUSIVE:
      setOutputExclusive(payload[MessageKeys.OUTPUT_ID]);
      break;
      
    case Commands.TOGGLE_OUTPUT:
      toggleOutput(payload[MessageKeys.OUTPUT_ID]);
      break;
      
    case Commands.SET_OUTPUT_VOLUME:
      setOutputVolume(payload[MessageKeys.OUTPUT_ID], payload[MessageKeys.VOLUME]);
      break;
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Phase 2: Open configuration page
  console.log('Configuration requested');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Phase 2: Handle configuration changes
  console.log('Configuration closed');
});
