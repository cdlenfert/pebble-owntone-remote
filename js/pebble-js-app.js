var OWNTONE_BASE = "http://owntone.local:3689";
var MAX_RESULTS = 8;

// Message key constants must match the C app
var KEY_CMD = 0; // 1=search,2=add,6=playpause
var KEY_TYPE = 1;
var KEY_QUERY = 2;
var KEY_RESULT_COUNT = 3;
var KEY_TITLE_BASE = 10;
var KEY_URI_BASE = 20;
var KEY_STATUS = 40;
var KEY_LAST_ACTION = 41;
var KEY_DEBUG = 42;

function sendDictionaryToPebble(dict) {
  Pebble.sendAppMessage(dict, function() {}, function(e) { console.log('Send failed: ' + JSON.stringify(e)); });
}

function performSearch(type, query) {
  addLog('Search: ' + type + ' "' + query + '"');
  // inform watch that HTTP request is starting
  sendDictionaryToPebble({[KEY_STATUS]:'HTTP_START'});
  // OwnTone expects singular: playlist, artist, album, track
  var searchType = type.toLowerCase();
  var url = OWNTONE_BASE + "/api/search?type=" + encodeURIComponent(searchType) + "&query=" + encodeURIComponent(query) + "&limit=" + MAX_RESULTS;
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10000;
  xhr.open('GET', url, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      sendDictionaryToPebble({[KEY_STATUS]:'HTTP_DONE:' + xhr.status});
      if (xhr.status !== 200) {
        console.log('Search HTTP error: ' + xhr.status + ' ' + (xhr.responseText||''));
        addLog('Search HTTP error: ' + xhr.status + ' ' + (xhr.responseText||''));
        sendDictionaryToPebble({[KEY_RESULT_COUNT]:0});
        sendDictionaryToPebble({[KEY_STATUS]:'Error: HTTP ' + xhr.status});
        return;
      }
      try {
        var resp = JSON.parse(xhr.responseText);
        console.log('Search response keys: ' + Object.keys(resp).join(','));
        addLog('Response keys: ' + Object.keys(resp).join(','));
        var items = [];
        // OwnTone search returns nested objects: playlists.items, artists.items, albums.items, tracks.items
        if (resp.playlists && resp.playlists.items && Array.isArray(resp.playlists.items)) {
          items = resp.playlists.items;
          addLog('Using playlists.items: ' + items.length);
        } else if (resp.artists && resp.artists.items && Array.isArray(resp.artists.items)) {
          items = resp.artists.items;
          addLog('Using artists.items: ' + items.length);
        } else if (resp.albums && resp.albums.items && Array.isArray(resp.albums.items)) {
          items = resp.albums.items;
          addLog('Using albums.items: ' + items.length);
        } else if (resp.tracks && resp.tracks.items && Array.isArray(resp.tracks.items)) {
          items = resp.tracks.items;
          addLog('Using tracks.items: ' + items.length);
        } else if (resp.items && Array.isArray(resp.items)) {
          items = resp.items;
          addLog('Using resp.items: ' + items.length);
        } else if (Array.isArray(resp)) {
          items = resp;
          addLog('Using resp array: ' + items.length);
        } else {
          addLog('No items found in response!');
        }
        var dict = {};
        dict[KEY_RESULT_COUNT] = items.length;
        for (var i=0;i<items.length && i<MAX_RESULTS;i++) {
          var title = '';
          var uri = '';
          // CRITICAL: Prefer `uri` (library:...) over `path` (spotify:...)
          // The server expects library URIs for queue operations!
          title = items[i].name || items[i].title || items[i].display_name || '';
          uri = items[i].uri || items[i].path || items[i].id || items[i].permalink || '';
          dict[KEY_TITLE_BASE + i] = title;
          dict[KEY_URI_BASE + i] = uri;
        }
        sendDictionaryToPebble(dict);
        // indicate OK
        addLog('Search results: ' + items.length + ' items');
        sendDictionaryToPebble({[KEY_STATUS]:'OK'});
      } catch (e) {
        console.log('Search parse error: ' + e);
        addLog('Search parse error: ' + e);
        sendDictionaryToPebble({[KEY_RESULT_COUNT]:0});
        sendDictionaryToPebble({[KEY_STATUS]:'Error: parse'});
      }
    }
  };
  xhr.send(null);
}

function performAdd(uri, type) {
  addLog('Add requested: ' + (type||'') + ' -> ' + uri);
  try { sendDictionaryToPebble({[KEY_STATUS]:'ADD_STARTED'}); } catch (ex) {}
  if (!uri) return;
  var t = (type || '').toLowerCase();

  // default behavior for playlist/artist/album: clear and start
  var shuffle = 'true';
  if (t == 'album') shuffle = 'false';
  
  // OwnTone expects all parameters as QUERY PARAMETERS, not in POST body!
  // Build URL with query params for: uris, clear, playback, shuffle
  var url = OWNTONE_BASE + '/api/queue/items/add?uris=' + encodeURIComponent(uri) + 
            '&clear=true&playback=start&shuffle=' + encodeURIComponent(shuffle);
  
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10000;
  addLog('POST URL: ' + url);
  try { sendDictionaryToPebble({[KEY_STATUS]:'Sending request...'}); } catch (ex) {}
  xhr.open('POST', url, true);
  // No Content-Type header needed — no body being sent
  try { sendDictionaryToPebble({[KEY_STATUS]:'Sending to server'}); } catch (ex) {}
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      var respText = xhr.responseText || '';
      addLog('Add HTTP ' + xhr.status + ': ' + respText.substring(0, 200));
      try { sendDictionaryToPebble({[KEY_STATUS]:'Response: ' + xhr.status}); } catch (ex) {}
      if (xhr.status === 200 || xhr.status === 204) {
        sendDictionaryToPebble({[KEY_STATUS]:'OK'});
      } else {
        var errMsg = 'HTTP ' + xhr.status;
        if (respText.length > 0 && respText.length < 30) errMsg += ': ' + respText;
        sendDictionaryToPebble({[KEY_STATUS]:'Error: ' + errMsg});
        try { sendDictionaryToPebble({[KEY_DEBUG]: respText.substring(0, 500)}); } catch (ex2) {}
      }
    }
  };
  try { 
    xhr.send(null);  // Send with null body (all params in URL)
    addLog('POST sent to ' + url);
  } catch (e) { 
    addLog('Send exception: ' + e); 
    sendDictionaryToPebble({[KEY_STATUS]:'Error: send failed'}); 
  }
}

// --- Logging helpers (stored on the phone so the watch can request them) ---
function loadLogs() {
  try {
    var raw = localStorage.getItem('owntone_logs');
    if (!raw) return [];
    var arr = JSON.parse(raw);
    if (!Array.isArray(arr)) return [];
    return arr;
  } catch (e) {
    console.log('loadLogs error: ' + e);
    return [];
  }
}
function saveLogs(arr) {
  try {
    localStorage.setItem('owntone_logs', JSON.stringify(arr));
  } catch (e) {
    console.log('saveLogs error: ' + e);
  }
}
function addLog(msg) {
  try {
    var logs = loadLogs();
    var entry = {t: Date.now(), m: msg};
    logs.unshift(entry);
    if (logs.length > 32) logs.length = 32;
    saveLogs(logs);
    console.log('LOG: ' + msg);
  } catch (e) {
    console.log('addLog error: ' + e);
  }
}

Pebble.addEventListener('ready', function(e) {
  console.log('OwnTone JS ready');
  addLog('OwnTone JS ready');
  // Notify the watch that the JS companion is running
  try { sendDictionaryToPebble({[KEY_STATUS]:'JS_READY'}); } catch (ex) { console.log('JS_READY send failed: ' + ex); }
});

Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload;
  if (!payload) return;
  // Immediate minimal ACK so the watch can confirm the phone received something
  try { sendDictionaryToPebble({[KEY_STATUS]:'RECV'}); } catch (ex) { console.log('RECV send failed: ' + ex); }
  if (payload[KEY_LAST_ACTION]) {
    addLog('WATCH_LAST_ACTION: ' + payload[KEY_LAST_ACTION]);
  }
  if (payload[KEY_CMD] == 1) {
    var type = payload[KEY_TYPE] || 'playlist';
    var query = payload[KEY_QUERY] || '';
    // immediate ACK so watch knows the phone received the request
    sendDictionaryToPebble({[KEY_STATUS]:'RECV_SEARCH'});
    performSearch(type, query);
  } else if (payload[KEY_CMD] == 2) {
    // ACK the add command immediately so the watch knows the phone received it
    try { sendDictionaryToPebble({[KEY_STATUS]:'RECV_ADD'}); } catch (ex) {}
    // Log the full payload for debugging
    try { addLog('AppMessage ADD payload: ' + JSON.stringify(payload)); } catch (e) {}
    var uri = payload[KEY_URI_BASE] || payload[KEY_URI_BASE+0] || '';
    if (!uri) {
      addLog('Missing URI in add payload; aborting add');
      try { sendDictionaryToPebble({[KEY_STATUS]:'Error: missing_uri'}); } catch (ex) {}
      return;
    }
    performAdd(uri, payload[KEY_TYPE]);
  } else if (payload[KEY_CMD] == 2) {
    var uri = payload[KEY_URI_BASE] || payload[KEY_URI_BASE+0] || '';
    performAdd(uri, payload[KEY_TYPE]);
  } else if (payload[KEY_CMD] == 7) {
    // Random search
    var type = payload[KEY_TYPE] || 'playlist';
    sendDictionaryToPebble({[KEY_STATUS]:'RECV_RANDOM'});
    addLog('Random search for ' + type);
    var url;
    // Playlists don't support expression, use random letter query instead
    if (type.toLowerCase() === 'playlist') {
      var letters = 'abcdefghijklmnopqrstuvwxyz';
      var randomLetter = letters[Math.floor(Math.random() * letters.length)];
      var randomOffset = Math.floor(Math.random() * 20);
      url = OWNTONE_BASE + "/api/search?type=playlist&query=" + randomLetter + "&limit=8&offset=" + randomOffset;
    } else {
      // Artists and albums support expression with random ordering
      url = OWNTONE_BASE + "/api/search?type=" + encodeURIComponent(type.toLowerCase()) + 
            "&expression=" + encodeURIComponent("media_kind is music order by random desc") + "&limit=8";
    }
    var xhr = new XMLHttpRequest();
    xhr.timeout = 10000;
    xhr.open('GET', url, true);
    xhr.onreadystatechange = function() {
      if (xhr.readyState === 4) {
        sendDictionaryToPebble({[KEY_STATUS]:'HTTP_DONE:' + xhr.status});
        if (xhr.status !== 200) {
          console.log('Random search HTTP error: ' + xhr.status);
          addLog('Random search HTTP error: ' + xhr.status);
          sendDictionaryToPebble({[KEY_RESULT_COUNT]:0});
          sendDictionaryToPebble({[KEY_STATUS]:'Error: HTTP ' + xhr.status});
          return;
        }
        try {
          var resp = JSON.parse(xhr.responseText);
          var items = [];
          if (resp.playlists && resp.playlists.items && Array.isArray(resp.playlists.items)) {
            items = resp.playlists.items;
          } else if (resp.artists && resp.artists.items && Array.isArray(resp.artists.items)) {
            items = resp.artists.items;
          } else if (resp.albums && resp.albums.items && Array.isArray(resp.albums.items)) {
            items = resp.albums.items;
          }
          var dict = {};
          dict[KEY_RESULT_COUNT] = items.length;
          for (var i=0;i<items.length && i<8;i++) {
            dict[KEY_TITLE_BASE + i] = items[i].name || items[i].title || '';
            dict[KEY_URI_BASE + i] = items[i].uri || items[i].path || '';
          }
          sendDictionaryToPebble(dict);
          addLog('Random search results: ' + items.length);
          sendDictionaryToPebble({[KEY_STATUS]:'OK'});
        } catch (e) {
          console.log('Random search parse error: ' + e);
          addLog('Random search parse error: ' + e);
          sendDictionaryToPebble({[KEY_RESULT_COUNT]:0});
          sendDictionaryToPebble({[KEY_STATUS]:'Error: parse'});
        }
      }
    };
    xhr.send(null);
  } else if (payload[KEY_CMD] == 6) {
    addLog('Play/Pause requested');
    // immediate ACK so watch knows the phone received the request
    sendDictionaryToPebble({[KEY_STATUS]:'RECV_PLAYPAUSE'});
    // Basic toggle endpoint — may need adjustment for your Owntone setup
    var xhr = new XMLHttpRequest();
    xhr.timeout = 8000;
    var url = OWNTONE_BASE + '/api/player/toggle';
    xhr.open('PUT', url, true);
    xhr.onreadystatechange = function() {
      if (xhr.readyState === 4) {
        sendDictionaryToPebble({[KEY_STATUS]:'HTTP_DONE:' + xhr.status});
        addLog('PlayPause HTTP ' + xhr.status + ': ' + (xhr.responseText || ''));
        if (xhr.status === 200 || xhr.status === 204) {
          sendDictionaryToPebble({[KEY_STATUS]:'OK'});
        } else {
          sendDictionaryToPebble({[KEY_STATUS]:'Error: HTTP ' + xhr.status});
        }
      }
    };
    try { xhr.send(null); } catch (e) { console.log('PlayPause send error: ' + e); sendDictionaryToPebble({[KEY_STATUS]:'Error: send'}); }
  }
});
