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
  
  STATUS: 100,
  
  FAVORITE_COUNT: 110,
  FAVORITE_NAME_BASE: 120,
  FAVORITE_TYPE_BASE: 130
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
  SET_OUTPUT_VOLUME: 12,
  GET_FAVORITES: 13,
  PLAY: 14,
  PAUSE: 15
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

function play() {
  httpPut(Config.OWNTONE_BASE + '/api/player/play', function(status, response) {
    console.log('Play: ' + status);
  });
}

function pause() {
  httpPut(Config.OWNTONE_BASE + '/api/player/pause', function(status, response) {
    console.log('Pause: ' + status);
  });
}

function playPause() {
  httpPut(Config.OWNTONE_BASE + '/api/player/toggle', function(status, response) {
    console.log('OwnTone Remote: Play/Pause: ' + status);
    if (status === 200 || status === 204) {
      // Notify watch that toggle succeeded
      var dict = {};
      dict[MessageKeys.STATUS] = 1; // Success
      sendToPebble(dict);
    }
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
  // Use the /api/outputs/set endpoint to enable only the selected output
  console.log('OwnTone Remote: Setting exclusive output: ' + outputId);
  var data = JSON.stringify({ outputs: [outputId] });
  var xhr = new XMLHttpRequest();
  xhr.open('PUT', Config.OWNTONE_BASE + '/api/outputs/set', true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onload = function() {
    console.log('OwnTone Remote: Set exclusive output response: ' + xhr.status);
    // Refresh outputs list after switching
    setTimeout(getOutputs, 500);
  };
  xhr.onerror = function() {
    console.log('OwnTone Remote: Error setting exclusive output');
  };
  
  try {
    xhr.send(data);
  } catch (e) {
    console.log('OwnTone Remote: Error setting exclusive output: ' + e);
  }
}

function toggleOutput(outputId) {
  console.log('OwnTone Remote: Toggling output ' + outputId);
  httpGet(Config.OWNTONE_BASE + '/api/outputs/' + outputId, function(status, response) {
    console.log('OwnTone Remote: Get output status: ' + status);
    if (status === 200) {
      try {
        var data = JSON.parse(response);
        var newState = !data.selected;
        console.log('OwnTone Remote: Setting output selected=' + newState);
        
        // Use proper JSON body for PUT request
        var updateData = JSON.stringify({ selected: newState });
        var xhr = new XMLHttpRequest();
        xhr.open('PUT', Config.OWNTONE_BASE + '/api/outputs/' + outputId, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.onload = function() {
          console.log('OwnTone Remote: Toggle output response: ' + xhr.status);
          getOutputs();
        };
        xhr.send(updateData);
      } catch (e) {
        console.log('OwnTone Remote: Error toggling output: ' + e);
      }
    }
  });
}

function setOutputVolume(outputId, volume) {
  console.log('OwnTone Remote: Setting output ' + outputId + ' volume to ' + volume);
  var data = JSON.stringify({ volume: volume });
  var xhr = new XMLHttpRequest();
  xhr.open('PUT', Config.OWNTONE_BASE + '/api/outputs/' + outputId, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.onload = function() {
    console.log('OwnTone Remote: Set output volume response: ' + xhr.status);
  };
  xhr.send(data);
}

function getFavorites() {
  var stored = localStorage.getItem('owntone_favorites');
  var favorites = { playlists: [], artists: [], albums: [] };
  
  if (stored) {
    try {
      favorites = JSON.parse(stored);
    } catch (e) {
      console.log('Error loading favorites: ' + e);
    }
  }
  
  sendFavorites(favorites);
}

function sendFavorites(favorites) {
  // Flatten favorites into a single array with type info
  // Support up to 10 of each type (30 total)
  var allFavorites = [];
  
  // Add playlists (type 0) - up to 10
  for (var i = 0; i < favorites.playlists.length && i < 10; i++) {
    allFavorites.push({ name: favorites.playlists[i], type: ContentTypes.PLAYLIST });
  }
  
  // Add artists (type 1) - up to 10
  for (var i = 0; i < favorites.artists.length && i < 10; i++) {
    allFavorites.push({ name: favorites.artists[i], type: ContentTypes.ARTIST });
  }
  
  // Add albums (type 2) - up to 10
  for (var i = 0; i < favorites.albums.length && i < 10; i++) {
    allFavorites.push({ name: favorites.albums[i], type: ContentTypes.ALBUM });
  }
  
  var dict = {};
  dict[MessageKeys.FAVORITE_COUNT] = allFavorites.length;
  
  for (var i = 0; i < allFavorites.length && i < 30; i++) {
    // Compute keys to match appinfo.json layout (three groups: 0-9, 10-19, 20-29)
    var nameKey, typeKey;
    if (i < 10) {
      nameKey = MessageKeys.FAVORITE_NAME_BASE + i;               // 120..129
      typeKey = MessageKeys.FAVORITE_TYPE_BASE + i;               // 130..139
    } else if (i < 20) {
      nameKey = MessageKeys.FAVORITE_NAME_BASE + 20 + (i - 10);   // 140..149
      typeKey = MessageKeys.FAVORITE_TYPE_BASE + 30 + (i - 10);   // 160..169
    } else {
      nameKey = MessageKeys.FAVORITE_NAME_BASE + 30 + (i - 20);   // 150..159
      typeKey = MessageKeys.FAVORITE_TYPE_BASE + 40 + (i - 20);   // 170..179
    }

    // Debug: log keys and values to help diagnose any key/collision issues
    console.log('favorite[' + i + '] nameKey=' + nameKey + ' name="' + allFavorites[i].name + '" typeKey=' + typeKey + ' type=' + allFavorites[i].type);

    dict[nameKey] = allFavorites[i].name;
    dict[typeKey] = allFavorites[i].type;
  }
  
  console.log('Sending ' + allFavorites.length + ' favorites to watch');
  sendToPebble(dict);
}

// Pebble event handlers
Pebble.addEventListener('ready', function(e) {
  console.log('OwnTone Remote JS ready');
  console.log('Server: ' + Config.OWNTONE_BASE);
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload;
  
  if (!payload) {
    return;
  }
  
  // PebbleKit JS converts numeric keys to their string names from appinfo.json
  var cmd = payload.CMD || payload[MessageKeys.CMD];
  
  switch (cmd) {
    case Commands.GET_PLAYER_STATE:
      getPlayerState();
      break;
      
    case Commands.PLAY_PAUSE:
      playPause();
      break;
      
    case Commands.PLAY:
      play();
      break;
      
    case Commands.PAUSE:
      pause();
      break;
      
    case Commands.NEXT:
      next();
      break;
      
    case Commands.PREVIOUS:
      previous();
      break;
      
    case Commands.SET_VOLUME:
      setVolume(payload.VOLUME || payload[MessageKeys.VOLUME]);
      break;
      
    case Commands.SEARCH:
      search(payload.TYPE || payload[MessageKeys.TYPE], payload.QUERY || payload[MessageKeys.QUERY]);
      break;
      
    case Commands.RANDOM:
      random(payload.TYPE || payload[MessageKeys.TYPE]);
      break;
      
    case Commands.ADD_TO_QUEUE:
      addToQueue(payload.URI || payload[MessageKeys.URI], payload.TYPE || payload[MessageKeys.TYPE]);
      break;
      
    case Commands.GET_OUTPUTS:
      getOutputs();
      break;
      
    case Commands.SET_OUTPUT_EXCLUSIVE:
      setOutputExclusive(payload.OUTPUT_ID || payload[MessageKeys.OUTPUT_ID]);
      break;
      
    case Commands.TOGGLE_OUTPUT:
      toggleOutput(payload.OUTPUT_ID || payload[MessageKeys.OUTPUT_ID]);
      break;
      
    case Commands.SET_OUTPUT_VOLUME:
      setOutputVolume(payload.OUTPUT_ID || payload[MessageKeys.OUTPUT_ID], payload.VOLUME || payload[MessageKeys.VOLUME]);
      break;
      
    case Commands.GET_FAVORITES:
      getFavorites();
      break;
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('https://cdlenfert.github.io/pebble-owntone-remote/config.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) {
    try {
      var settings = JSON.parse(decodeURIComponent(e.response));
      console.log('Configuration received:', settings);
      
      if (settings.favorites) {
        localStorage.setItem('owntone_favorites', JSON.stringify(settings.favorites));
        sendFavorites(settings.favorites);
      }
    } catch (err) {
      console.log('Error parsing config response: ' + err);
    }
  }
});
