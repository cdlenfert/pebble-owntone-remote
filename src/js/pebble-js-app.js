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
  FAVORITE_TYPE_BASE: 130,
  
  QUEUE_COUNT: 140,
  QUEUE_SELECTED: 141,
  QUEUE_TITLE_BASE: 150,
  QUEUE_ARTIST_BASE: 160,
  QUEUE_ITEM_ID_BASE: 170,
  
  QUEUE_ITEM_ID: 180,
  PLAYER_AUTO_CLOSE_TIMEOUT: 190,
  APP_AUTO_CLOSE_TIMEOUT: 191
  ,
  VIBRATION: 192
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
  PAUSE: 15,
  GET_QUEUE: 16,
  PLAY_QUEUE_ITEM: 17
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
          var isStream = track.data_kind === 'url';

          if (isStream) {
            // OwnTone maps radio streams as station name in title and the current song in album.
            dict[MessageKeys.PLAYER_TRACK] = track.album || track.title || 'Unknown';
            dict[MessageKeys.PLAYER_ALBUM] = track.title || track.album || 'Unknown';
          } else {
            dict[MessageKeys.PLAYER_TRACK] = track.title || 'Unknown';
            dict[MessageKeys.PLAYER_ALBUM] = track.album || 'Unknown';
          }

          dict[MessageKeys.PLAYER_ARTIST] = track.artist || 'Unknown';
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
    if (status === 200 || status === 204) {
      // Delay to allow server to transition to playing state
      setTimeout(getPlayerState, 200);
    }
  });
}

function pause() {
  httpPut(Config.OWNTONE_BASE + '/api/player/pause', function(status, response) {
    console.log('Pause: ' + status);
    if (status === 200 || status === 204) {
      // Delay to allow server to transition to paused state
      setTimeout(getPlayerState, 200);
    }
  });
}

function playPause() {
  httpPut(Config.OWNTONE_BASE + '/api/player/toggle', function(status, response) {
    console.log('OwnTone Remote: Play/Pause: ' + status);
    if (status === 200 || status === 204) {
      // Notify watch that toggle succeeded, then sync state after delay
      var dict = {};
      dict[MessageKeys.STATUS] = 1; // Success
      sendToPebble(dict);
      setTimeout(getPlayerState, 200);
    }
  });
}

function next() {
  httpPut(Config.OWNTONE_BASE + '/api/player/next', function(status, response) {
    console.log('Next: ' + status);
    if (status === 200 || status === 204) {
      // Longer delay to allow server to load and start playing next track
      setTimeout(getPlayerState, 500);
    }
  });
}

function previous() {
  httpPut(Config.OWNTONE_BASE + '/api/player/previous', function(status, response) {
    console.log('Previous: ' + status);
    if (status === 200 || status === 204) {
      // Longer delay to allow server to load and start playing previous track
      setTimeout(getPlayerState, 500);
    }
  });
}

function setVolume(volume) {
  httpPut(Config.OWNTONE_BASE + '/api/player/volume?volume=' + volume, function(status, response) {
    console.log('Set volume to ' + volume + ': ' + status);
  });
}

function search(type, query) {
  // Sanitize query: remove common punctuation and collapse whitespace so
  // server receives clean terms (helps dictation results).
  if (typeof query === 'string') {
    // Remove characters that are not word chars or whitespace
    query = query.replace(/[^\w\s]/g, ' ');
    // Collapse multiple spaces and trim
    query = query.replace(/\s+/g, ' ').trim();
  }

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
      // Longer delay to allow server to load and start playing queued items
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
    // Send both name and type as strings to ensure consistent encoding across the bridge
    dict[nameKey] = String(allFavorites[i].name);
    dict[typeKey] = String(allFavorites[i].type);
  }
  
  console.log('Sending ' + allFavorites.length + ' favorites to watch');
  sendToPebble(dict);
}

function getQueue() {
  // First get player state to find current item
  var playerXhr = new XMLHttpRequest();
  playerXhr.open('GET', Config.OWNTONE_BASE + '/api/player', true);
  playerXhr.onload = function() {
    if (playerXhr.readyState === 4 && playerXhr.status === 200) {
      var playerData = JSON.parse(playerXhr.responseText);
      var currentItemId = playerData.item_id || 0;
      
      // Now get queue
      var queueXhr = new XMLHttpRequest();
      queueXhr.open('GET', Config.OWNTONE_BASE + '/api/queue', true);
      queueXhr.onload = function() {
        if (queueXhr.readyState === 4 && queueXhr.status === 200) {
          var response = JSON.parse(queueXhr.responseText);
          sendQueue(response.items || [], currentItemId);
        } else {
          console.log('Queue request failed: ' + queueXhr.status);
        }
      };
      queueXhr.send(null);
    } else {
      console.log('Player request failed: ' + playerXhr.status);
    }
  };
  playerXhr.send(null);
}

function sendQueue(items, currentItemId) {
  // Find position of currently playing item
  var currentPos = -1;
  for (var i = 0; i < items.length; i++) {
    if (items[i].id === currentItemId) {
      currentPos = i;
      break;
    }
  }
  
  // If not found, start from the beginning
  if (currentPos === -1) {
    currentPos = 0;
  }
  
  // Show up to 4 before current, current, up to 5 after (10 total)
  var startPos = Math.max(0, currentPos - 4);
  var endPos = Math.min(items.length, startPos + 10);

  // Adjust start if we're near the end
  if (endPos - startPos < 10) {
    startPos = Math.max(0, endPos - 10);
  }
  
  var slicedItems = items.slice(startPos, endPos);
  var selectedIndex = currentPos - startPos;
  
  var count = slicedItems.length;
  var dict = {};
  dict[MessageKeys.QUEUE_COUNT] = count;
  dict[MessageKeys.QUEUE_SELECTED] = selectedIndex;
  
  for (var i = 0; i < count; i++) {
    var item = slicedItems[i];
    dict[MessageKeys.QUEUE_TITLE_BASE + i] = item.title || 'Unknown';
    dict[MessageKeys.QUEUE_ARTIST_BASE + i] = item.artist || '';
    dict[MessageKeys.QUEUE_ITEM_ID_BASE + i] = item.id;
  }
  
  console.log('Sending ' + count + ' queue items (selected: ' + selectedIndex + ') to watch');
  sendToPebble(dict);
}

function playQueueItem(itemId) {
  var xhr = new XMLHttpRequest();
  xhr.open('PUT', Config.OWNTONE_BASE + '/api/player/play?item_id=' + itemId, true);
  xhr.onload = function() {
    if (xhr.readyState === 4) {
      console.log('Play queue item ' + itemId + ': ' + xhr.status);
      if (xhr.status === 200 || xhr.status === 204) {
        // Longer delay to allow server to load and start playing queue item
        setTimeout(getPlayerState, 500);
      }
    }
  };
  xhr.send(null);
}

function sendPlayerAutoCloseTimeout(timeoutSeconds) {
  var dict = {};
  dict[MessageKeys.PLAYER_AUTO_CLOSE_TIMEOUT] = timeoutSeconds;
  sendToPebble(dict);
  console.log('Sent player auto-close timeout: ' + timeoutSeconds + 's');
}

function sendAppAutoCloseTimeout(timeoutSeconds) {
  var dict = {};
  dict[MessageKeys.APP_AUTO_CLOSE_TIMEOUT] = timeoutSeconds;
  sendToPebble(dict);
  console.log('Sent app auto-close timeout: ' + timeoutSeconds + 's');
}

function sendVibrationSetting(vib) {
  var dict = {};
  dict[MessageKeys.VIBRATION] = vib;
  sendToPebble(dict);
  console.log('Sent vibration setting: ' + vib);
}

// Pebble event handlers
Pebble.addEventListener('ready', function(e) {
  console.log('OwnTone Remote JS ready');
  console.log('Server: ' + Config.OWNTONE_BASE);
  
  // Send player auto-close timeout after a delay to ensure watch is ready.
  // Use 2 seconds to be reliable on Aplite which no longer has a splash screen
  // acting as a warmup buffer.
  setTimeout(function() {
    var timeout = localStorage.getItem('owntone_player_auto_close_timeout');
    if (timeout !== null) {
      sendPlayerAutoCloseTimeout(parseInt(timeout));
    } else {
      // Send default on first launch (matches default in config.html)
      sendPlayerAutoCloseTimeout(30);
    }

    var appTimeout = localStorage.getItem('owntone_app_auto_close_timeout');
    if (appTimeout !== null) {
      sendAppAutoCloseTimeout(parseInt(appTimeout));
    } else {
      // Default to 5 minutes (300s) on first launch
      sendAppAutoCloseTimeout(300);
    }
  }, 2000);
});

  // Also send vibration setting on ready (default to 1 = lighter)
  setTimeout(function() {
    var vib = localStorage.getItem('owntone_vibration');
    if (vib !== null) sendVibrationSetting(parseInt(vib));
    else sendVibrationSetting(1);
  }, 2200);

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload;
  
  if (!payload) {
    return;
  }
  
  // PebbleKit JS converts numeric keys to their string names from appinfo.json
  var cmd = payload.CMD || payload[MessageKeys.CMD];
  
  switch (cmd) {
    case Commands.GET_PLAYER_STATE:
      // Send immediate small status to indicate connectivity, then fetch full state
      var statusDict = {};
      statusDict[MessageKeys.STATUS] = 1;
      sendToPebble(statusDict);
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
      
    case Commands.GET_QUEUE:
      getQueue();
      break;
      
    case Commands.PLAY_QUEUE_ITEM:
      playQueueItem(payload.QUEUE_ITEM_ID || payload[MessageKeys.QUEUE_ITEM_ID]);
      break;
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Build current settings from PebbleKit JS localStorage and pass to the
  // configuration page so it can pre-populate fields.
  try {
    var cfg = {};

    var favs = localStorage.getItem('owntone_favorites');
    if (favs) {
      try { cfg.favorites = JSON.parse(favs); } catch (e) { cfg.favorites = null; }
    }

    var ptimeout = localStorage.getItem('owntone_player_auto_close_timeout');
    if (ptimeout !== null) cfg.playerAutoCloseTimeout = parseInt(ptimeout);

    var atimeout = localStorage.getItem('owntone_app_auto_close_timeout');
    if (atimeout !== null) cfg.appAutoCloseTimeout = parseInt(atimeout);

    var url = 'https://cdlenfert.github.io/pebble-owntone-remote/config.html';
    url += '?settings=' + encodeURIComponent(JSON.stringify(cfg || {}));
    Pebble.openURL(url);
  } catch (err) {
    // Fallback to opening without settings if anything goes wrong
    Pebble.openURL('https://cdlenfert.github.io/pebble-owntone-remote/config.html');
  }
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
      
      if (settings.playerAutoCloseTimeout !== undefined) {
        localStorage.setItem('owntone_player_auto_close_timeout', settings.playerAutoCloseTimeout.toString());
        sendPlayerAutoCloseTimeout(settings.playerAutoCloseTimeout);
      }

      if (settings.appAutoCloseTimeout !== undefined) {
        localStorage.setItem('owntone_app_auto_close_timeout', settings.appAutoCloseTimeout.toString());
        sendAppAutoCloseTimeout(settings.appAutoCloseTimeout);
      }

      if (settings.vibration !== undefined) {
        localStorage.setItem('owntone_vibration', String(settings.vibration));
        sendVibrationSetting(settings.vibration);
      }
    } catch (err) {
      console.log('Error parsing config response: ' + err);
    }
  }
});
