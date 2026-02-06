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
  GET_FAVORITES: 13
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
  var allFavorites = [];
  
  // Add playlists (type 0)
  for (var i = 0; i < favorites.playlists.length && i < 10; i++) {
    allFavorites.push({ name: favorites.playlists[i], type: ContentTypes.PLAYLIST });
  }
  
  // Add artists (type 1)
  for (var i = 0; i < favorites.artists.length && allFavorites.length < 10; i++) {
    allFavorites.push({ name: favorites.artists[i], type: ContentTypes.ARTIST });
  }
  
  // Add albums (type 2)
  for (var i = 0; i < favorites.albums.length && allFavorites.length < 10; i++) {
    allFavorites.push({ name: favorites.albums[i], type: ContentTypes.ALBUM });
  }
  
  var dict = {};
  dict[MessageKeys.FAVORITE_COUNT] = allFavorites.length;
  
  for (var i = 0; i < allFavorites.length && i < 10; i++) {
    dict[MessageKeys.FAVORITE_NAME_BASE + i] = allFavorites[i].name;
    dict[MessageKeys.FAVORITE_TYPE_BASE + i] = allFavorites[i].type;
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
  var currentSettings = localStorage.getItem('owntone_favorites');
  var url = 'data:text/html;charset=utf-8,%3C%21DOCTYPE%20html%3E%0A%3Chtml%3E%0A%3Chead%3E%0A%20%20%3Cmeta%20charset%3D%22utf-8%22%3E%0A%20%20%3Cmeta%20name%3D%22viewport%22%20content%3D%22width%3Ddevice-width%2C%20initial-scale%3D1%22%3E%0A%20%20%3Ctitle%3EOwnTone%20Remote%20Settings%3C%2Ftitle%3E%0A%20%20%3Cstyle%3E%0A%20%20%20%20body%20%7B%0A%20%20%20%20%20%20font-family%3A%20-apple-system%2C%20BlinkMacSystemFont%2C%20%27Segoe%20UI%27%2C%20Roboto%2C%20sans-serif%3B%0A%20%20%20%20%20%20margin%3A%200%3B%0A%20%20%20%20%20%20padding%3A%2020px%3B%0A%20%20%20%20%20%20background%3A%20%23f5f5f5%3B%0A%20%20%20%20%7D%0A%20%20%20%20.container%20%7B%0A%20%20%20%20%20%20max-width%3A%20600px%3B%0A%20%20%20%20%20%20margin%3A%200%20auto%3B%0A%20%20%20%20%20%20background%3A%20white%3B%0A%20%20%20%20%20%20padding%3A%2020px%3B%0A%20%20%20%20%20%20border-radius%3A%208px%3B%0A%20%20%20%20%20%20box-shadow%3A%200%202px%204px%20rgba%280%2C0%2C0%2C0.1%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20h1%20%7B%0A%20%20%20%20%20%20margin-top%3A%200%3B%0A%20%20%20%20%20%20color%3A%20%23333%3B%0A%20%20%20%20%7D%0A%20%20%20%20h2%20%7B%0A%20%20%20%20%20%20color%3A%20%23666%3B%0A%20%20%20%20%20%20border-bottom%3A%201px%20solid%20%23ddd%3B%0A%20%20%20%20%20%20padding-bottom%3A%2010px%3B%0A%20%20%20%20%7D%0A%20%20%20%20.section%20%7B%0A%20%20%20%20%20%20margin%3A%2020px%200%3B%0A%20%20%20%20%7D%0A%20%20%20%20label%20%7B%0A%20%20%20%20%20%20display%3A%20block%3B%0A%20%20%20%20%20%20margin%3A%2010px%200%205px%3B%0A%20%20%20%20%20%20font-weight%3A%20500%3B%0A%20%20%20%20%20%20color%3A%20%23555%3B%0A%20%20%20%20%7D%0A%20%20%20%20input%5Btype%3D%22text%22%5D%2C%20input%5Btype%3D%22number%22%5D%20%7B%0A%20%20%20%20%20%20width%3A%20100%25%3B%0A%20%20%20%20%20%20padding%3A%208px%3B%0A%20%20%20%20%20%20border%3A%201px%20solid%20%23ddd%3B%0A%20%20%20%20%20%20border-radius%3A%204px%3B%0A%20%20%20%20%20%20box-sizing%3A%20border-box%3B%0A%20%20%20%20%7D%0A%20%20%20%20button%20%7B%0A%20%20%20%20%20%20background%3A%20%23007AFF%3B%0A%20%20%20%20%20%20color%3A%20white%3B%0A%20%20%20%20%20%20border%3A%20none%3B%0A%20%20%20%20%20%20padding%3A%2012px%2024px%3B%0A%20%20%20%20%20%20border-radius%3A%206px%3B%0A%20%20%20%20%20%20font-size%3A%2016px%3B%0A%20%20%20%20%20%20cursor%3A%20pointer%3B%0A%20%20%20%20%20%20width%3A%20100%25%3B%0A%20%20%20%20%20%20margin-top%3A%2020px%3B%0A%20%20%20%20%7D%0A%20%20%20%20button%3Ahover%20%7B%0A%20%20%20%20%20%20background%3A%20%230051D5%3B%0A%20%20%20%20%7D%0A%20%20%20%20.info%20%7B%0A%20%20%20%20%20%20background%3A%20%23e3f2fd%3B%0A%20%20%20%20%20%20padding%3A%2015px%3B%0A%20%20%20%20%20%20border-radius%3A%204px%3B%0A%20%20%20%20%20%20margin%3A%2020px%200%3B%0A%20%20%20%20%20%20color%3A%20%231976d2%3B%0A%20%20%20%20%7D%0A%20%20%20%20.favorite-item%20%7B%0A%20%20%20%20%20%20display%3A%20flex%3B%0A%20%20%20%20%20%20align-items%3A%20center%3B%0A%20%20%20%20%20%20padding%3A%208px%3B%0A%20%20%20%20%20%20background%3A%20%23f9f9f9%3B%0A%20%20%20%20%20%20border-radius%3A%204px%3B%0A%20%20%20%20%20%20margin-bottom%3A%208px%3B%0A%20%20%20%20%7D%0A%20%20%20%20.favorite-item%20span%20%7B%0A%20%20%20%20%20%20flex%3A%201%3B%0A%20%20%20%20%20%20margin-right%3A%2010px%3B%0A%20%20%20%20%7D%0A%20%20%20%20.favorite-item%20button%20%7B%0A%20%20%20%20%20%20background%3A%20%23dc3545%3B%0A%20%20%20%20%20%20padding%3A%204px%2012px%3B%0A%20%20%20%20%20%20margin%3A%200%3B%0A%20%20%20%20%20%20width%3A%20auto%3B%0A%20%20%20%20%20%20font-size%3A%2014px%3B%0A%20%20%20%20%7D%0A%20%20%20%20.add-favorite%20%7B%0A%20%20%20%20%20%20display%3A%20flex%3B%0A%20%20%20%20%20%20gap%3A%208px%3B%0A%20%20%20%20%20%20margin-bottom%3A%2010px%3B%0A%20%20%20%20%7D%0A%20%20%20%20.add-favorite%20input%20%7B%0A%20%20%20%20%20%20flex%3A%201%3B%0A%20%20%20%20%7D%0A%20%20%20%20.add-favorite%20button%20%7B%0A%20%20%20%20%20%20background%3A%20%2328a745%3B%0A%20%20%20%20%20%20padding%3A%208px%2016px%3B%0A%20%20%20%20%20%20margin%3A%200%3B%0A%20%20%20%20%20%20width%3A%20auto%3B%0A%20%20%20%20%7D%0A%20%20%20%20.favorites-list%20%7B%0A%20%20%20%20%20%20max-height%3A%20200px%3B%0A%20%20%20%20%20%20overflow-y%3A%20auto%3B%0A%20%20%20%20%20%20margin-bottom%3A%2015px%3B%0A%20%20%20%20%7D%0A%20%20%3C%2Fstyle%3E%0A%3C%2Fhead%3E%0A%3Cbody%3E%0A%20%20%3Cdiv%20class%3D%22container%22%3E%0A%20%20%20%20%3Ch1%3EOwnTone%20Remote%20Settings%3C%2Fh1%3E%0A%20%20%20%20%0A%20%20%20%20%3Cdiv%20class%3D%22section%22%3E%0A%20%20%20%20%20%20%3Ch2%3EServer%20Settings%3C%2Fh2%3E%0A%20%20%20%20%20%20%3Clabel%20for%3D%22server-url%22%3EOwnTone%20Server%20URL%3C%2Flabel%3E%0A%20%20%20%20%20%20%3Cinput%20type%3D%22text%22%20id%3D%22server-url%22%20value%3D%22http%3A%2F%2Fowntone.local%3A3689%22%20readonly%3E%0A%20%20%20%20%20%20%3Csmall%20style%3D%22color%3A%20%23999%3B%22%3ECurrently%20hardcoded%20to%20owntone.local%3A3689%3C%2Fsmall%3E%0A%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%0A%20%20%20%20%3Cdiv%20class%3D%22section%22%3E%0A%20%20%20%20%20%20%3Ch2%3EFavorites%3C%2Fh2%3E%0A%20%20%20%20%20%20%3Cp%3EAdd%20your%20favorite%20playlists%2C%20artists%2C%20and%20albums%20for%20quick%20access%20from%20your%20watch.%20Enter%20search%20terms%20%28e.g.%2C%20%22Mellow%22%2C%20%22Martin%20Courtney%22%2C%20%22Stolen%20Moments%22%29.%3C%2Fp%3E%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20%3Clabel%3EFavorite%20Playlists%3C%2Flabel%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22add-favorite%22%3E%0A%20%20%20%20%20%20%20%20%3Cinput%20type%3D%22text%22%20id%3D%22playlist-input%22%20placeholder%3D%22Enter%20playlist%20name...%22%3E%0A%20%20%20%20%20%20%20%20%3Cbutton%20onclick%3D%22addFavorite%28%27playlists%27%29%22%3EAdd%3C%2Fbutton%3E%0A%20%20%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22favorites-list%22%20id%3D%22playlists-list%22%3E%3C%2Fdiv%3E%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20%3Clabel%3EFavorite%20Artists%3C%2Flabel%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22add-favorite%22%3E%0A%20%20%20%20%20%20%20%20%3Cinput%20type%3D%22text%22%20id%3D%22artists-input%22%20placeholder%3D%22Enter%20artist%20name...%22%3E%0A%20%20%20%20%20%20%20%20%3Cbutton%20onclick%3D%22addFavorite%28%27artists%27%29%22%3EAdd%3C%2Fbutton%3E%0A%20%20%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22favorites-list%22%20id%3D%22artists-list%22%3E%3C%2Fdiv%3E%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20%3Clabel%3EFavorite%20Albums%3C%2Flabel%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22add-favorite%22%3E%0A%20%20%20%20%20%20%20%20%3Cinput%20type%3D%22text%22%20id%3D%22albums-input%22%20placeholder%3D%22Enter%20album%20name...%22%3E%0A%20%20%20%20%20%20%20%20%3Cbutton%20onclick%3D%22addFavorite%28%27albums%27%29%22%3EAdd%3C%2Fbutton%3E%0A%20%20%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%20%20%3Cdiv%20class%3D%22favorites-list%22%20id%3D%22albums-list%22%3E%3C%2Fdiv%3E%0A%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%0A%20%20%20%20%3Cdiv%20class%3D%22section%22%20style%3D%22opacity%3A%200.5%3B%20pointer-events%3A%20none%3B%22%3E%0A%20%20%20%20%20%20%3Ch2%3EOutput%20Defaults%3C%2Fh2%3E%0A%20%20%20%20%20%20%3Cp%3ESet%20default%20volume%20levels%20for%20each%20audio%20output.%3C%2Fp%3E%0A%20%20%20%20%20%20%3Cdiv%20id%3D%22outputs-list%22%3E%0A%20%20%20%20%20%20%20%20%3Cem%3EOutput%20configuration%20coming%20soon...%3C%2Fem%3E%0A%20%20%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%3C%2Fdiv%3E%0A%20%20%20%20%0A%20%20%20%20%3Cbutton%20onclick%3D%22saveSettings%28%29%22%3ESave%20Settings%3C%2Fbutton%3E%0A%20%20%3C%2Fdiv%3E%0A%20%20%0A%20%20%3Cscript%3E%0A%20%20%20%20var%20favorites%20%3D%20%7B%0A%20%20%20%20%20%20playlists%3A%20%5B%5D%2C%0A%20%20%20%20%20%20artists%3A%20%5B%5D%2C%0A%20%20%20%20%20%20albums%3A%20%5B%5D%0A%20%20%20%20%7D%3B%0A%20%20%20%20%0A%20%20%20%20function%20getQueryParam%28variable%2C%20defaultValue%29%20%7B%0A%20%20%20%20%20%20var%20query%20%3D%20location.search.substring%281%29%3B%0A%20%20%20%20%20%20var%20vars%20%3D%20query.split%28%27%26%27%29%3B%0A%20%20%20%20%20%20for%20%28var%20i%20%3D%200%3B%20i%20%3C%20vars.length%3B%20i%2B%2B%29%20%7B%0A%20%20%20%20%20%20%20%20var%20pair%20%3D%20vars%5Bi%5D.split%28%27%3D%27%29%3B%0A%20%20%20%20%20%20%20%20if%20%28pair%5B0%5D%20%3D%3D%3D%20variable%29%20%7B%0A%20%20%20%20%20%20%20%20%20%20return%20decodeURIComponent%28pair%5B1%5D%29%3B%0A%20%20%20%20%20%20%20%20%7D%0A%20%20%20%20%20%20%7D%0A%20%20%20%20%20%20return%20defaultValue%20%7C%7C%20false%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20loadFavorites%28%29%20%7B%0A%20%20%20%20%20%20var%20stored%20%3D%20localStorage.getItem%28%27owntone_favorites%27%29%3B%0A%20%20%20%20%20%20if%20%28stored%29%20%7B%0A%20%20%20%20%20%20%20%20try%20%7B%0A%20%20%20%20%20%20%20%20%20%20favorites%20%3D%20JSON.parse%28stored%29%3B%0A%20%20%20%20%20%20%20%20%7D%20catch%20%28e%29%20%7B%0A%20%20%20%20%20%20%20%20%20%20console.log%28%27Error%20loading%20favorites%3A%27%2C%20e%29%3B%0A%20%20%20%20%20%20%20%20%20%20favorites%20%3D%20%7B%20playlists%3A%20%5B%5D%2C%20artists%3A%20%5B%5D%2C%20albums%3A%20%5B%5D%20%7D%3B%0A%20%20%20%20%20%20%20%20%7D%0A%20%20%20%20%20%20%7D%0A%20%20%20%20%20%20renderFavorites%28%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20saveFavoritesToStorage%28%29%20%7B%0A%20%20%20%20%20%20localStorage.setItem%28%27owntone_favorites%27%2C%20JSON.stringify%28favorites%29%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20renderFavorites%28%29%20%7B%0A%20%20%20%20%20%20renderCategory%28%27playlists%27%29%3B%0A%20%20%20%20%20%20renderCategory%28%27artists%27%29%3B%0A%20%20%20%20%20%20renderCategory%28%27albums%27%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20renderCategory%28category%29%20%7B%0A%20%20%20%20%20%20var%20list%20%3D%20document.getElementById%28category%20%2B%20%27-list%27%29%3B%0A%20%20%20%20%20%20list.innerHTML%20%3D%20%27%27%3B%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20favorites%5Bcategory%5D.forEach%28function%28item%2C%20index%29%20%7B%0A%20%20%20%20%20%20%20%20var%20div%20%3D%20document.createElement%28%27div%27%29%3B%0A%20%20%20%20%20%20%20%20div.className%20%3D%20%27favorite-item%27%3B%0A%20%20%20%20%20%20%20%20%0A%20%20%20%20%20%20%20%20var%20span%20%3D%20document.createElement%28%27span%27%29%3B%0A%20%20%20%20%20%20%20%20span.textContent%20%3D%20item%3B%0A%20%20%20%20%20%20%20%20%0A%20%20%20%20%20%20%20%20var%20btn%20%3D%20document.createElement%28%27button%27%29%3B%0A%20%20%20%20%20%20%20%20btn.textContent%20%3D%20%27Remove%27%3B%0A%20%20%20%20%20%20%20%20btn.onclick%20%3D%20function%28%29%20%7B%20removeFavorite%28category%2C%20index%29%3B%20%7D%3B%0A%20%20%20%20%20%20%20%20%0A%20%20%20%20%20%20%20%20div.appendChild%28span%29%3B%0A%20%20%20%20%20%20%20%20div.appendChild%28btn%29%3B%0A%20%20%20%20%20%20%20%20list.appendChild%28div%29%3B%0A%20%20%20%20%20%20%7D%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20addFavorite%28category%29%20%7B%0A%20%20%20%20%20%20var%20input%20%3D%20document.getElementById%28category.substring%280%2C%20category.length%20-%201%29%20%2B%20%27-input%27%29%3B%0A%20%20%20%20%20%20var%20value%20%3D%20input.value.trim%28%29%3B%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20if%20%28value%20%26%26%20favorites%5Bcategory%5D.length%20%3C%2010%29%20%7B%0A%20%20%20%20%20%20%20%20if%20%28%21favorites%5Bcategory%5D.includes%28value%29%29%20%7B%0A%20%20%20%20%20%20%20%20%20%20favorites%5Bcategory%5D.push%28value%29%3B%0A%20%20%20%20%20%20%20%20%20%20saveFavoritesToStorage%28%29%3B%0A%20%20%20%20%20%20%20%20%20%20renderCategory%28category%29%3B%0A%20%20%20%20%20%20%20%20%20%20input.value%20%3D%20%27%27%3B%0A%20%20%20%20%20%20%20%20%7D%20else%20%7B%0A%20%20%20%20%20%20%20%20%20%20alert%28%27This%20favorite%20already%20exists%21%27%29%3B%0A%20%20%20%20%20%20%20%20%7D%0A%20%20%20%20%20%20%7D%20else%20if%20%28favorites%5Bcategory%5D.length%20%3E%3D%2010%29%20%7B%0A%20%20%20%20%20%20%20%20alert%28%27Maximum%2010%20favorites%20per%20category%21%27%29%3B%0A%20%20%20%20%20%20%7D%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20removeFavorite%28category%2C%20index%29%20%7B%0A%20%20%20%20%20%20favorites%5Bcategory%5D.splice%28index%2C%201%29%3B%0A%20%20%20%20%20%20saveFavoritesToStorage%28%29%3B%0A%20%20%20%20%20%20renderCategory%28category%29%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20function%20saveSettings%28%29%20%7B%0A%20%20%20%20%20%20var%20settings%20%3D%20%7B%0A%20%20%20%20%20%20%20%20serverUrl%3A%20document.getElementById%28%27server-url%27%29.value%2C%0A%20%20%20%20%20%20%20%20favorites%3A%20favorites%2C%0A%20%20%20%20%20%20%20%20outputDefaults%3A%20%7B%7D%0A%20%20%20%20%20%20%7D%3B%0A%20%20%20%20%20%20%0A%20%20%20%20%20%20var%20location%20%3D%20%27pebblejs%3A%2F%2Fclose%23%27%20%2B%20encodeURIComponent%28JSON.stringify%28settings%29%29%3B%0A%20%20%20%20%20%20window.location.href%20%3D%20location%3B%0A%20%20%20%20%7D%0A%20%20%20%20%0A%20%20%20%20%2F%2F%20Load%20existing%20settings%0A%20%20%20%20var%20currentSettings%20%3D%20JSON.parse%28getQueryParam%28%27settings%27%2C%20%27%7B%7D%27%29%29%3B%0A%20%20%20%20console.log%28%27Current%20settings%3A%27%2C%20currentSettings%29%3B%0A%20%20%20%20%0A%20%20%20%20%2F%2F%20Load%20favorites%20from%20localStorage%0A%20%20%20%20loadFavorites%28%29%3B%0A%20%20%3C%2Fscript%3E%0A%3C%2Fbody%3E%0A%3C%2Fhtml%3E%0A';
  if (currentSettings) {
    url += '?settings=' + encodeURIComponent(currentSettings);
  }
  console.log('Opening configuration');
  Pebble.openURL(url);
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
