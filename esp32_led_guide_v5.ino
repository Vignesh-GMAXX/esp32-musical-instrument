/*
  ============================================================
  ESP32 LED Song Guide System - WiFi Version (v5)
  ============================================================

  Control is via a web page served by this ESP32.
  No physical buttons needed for NEXT/PLAY/PAUSE.
  Open the ESP32's IP address in any phone/laptop browser.

  ============================================================
  CONNECTIONS
  ============================================================

  ESP32 used: ESP32-U (standard 38-pin ESP32 DevKit)

  --- 14 LEDs (one per key) ---

  Each LED connects:
    Anode (+, longer leg) -> 220 ohm resistor -> ESP32 GPIO
    Cathode (-, shorter leg) -> GND

  LED  1  (Key 1 = C4)  -> GPIO 13
  LED  2  (Key 2 = D4)  -> GPIO 12
  LED  3  (Key 3 = E4)  -> GPIO 14
  LED  4  (Key 4 = F4)  -> GPIO 27
  LED  5  (Key 5 = G4)  -> GPIO 26
  LED  6  (Key 6 = A4)  -> GPIO 25
  LED  7  (Key 7 = B4)  -> GPIO 33
  LED  8  (Key 8 = C5)  -> GPIO 32
  LED  9  (Key 9 = D5)  -> GPIO 23
  LED 10  (Key 10 = E5) -> GPIO 22
  LED 11  (Key 11 = F5) -> GPIO 21
  LED 12  (Key 12 = G5) -> GPIO 19
  LED 13  (Key 13 = A5) -> GPIO 5
  LED 14  (Key 14 = C6) -> GPIO 18

  --- Power ---

  Power via USB cable (to PC or 5V adapter).
  All GND pins share a common rail.

  ============================================================
  HOW TO USE
  ============================================================

  1. Power on the ESP32.
  2. It connects to your iPhone hotspot automatically.
  3. Open Serial Monitor at 115200 baud.
  4. Wait for the IP address to print, e.g. "IP: 192.168.x.x"
  5. Open that IP in Safari or Chrome on your phone.
  6. Use the web page to pick a song and press Play.

  ============================================================
  WIFI CREDENTIALS
  ============================================================

  SSID     : iPhone
  Password : 31323132

  ============================================================
  WEB API ENDPOINTS (used internally by the web page)
  ============================================================

  GET /         -> serves the web app HTML page
  GET /play     -> start or resume current song
  GET /pause    -> pause current song
  GET /next     -> skip to next song and play it
  GET /song?i=N -> select song index N (0-4) and play it
  GET /status   -> returns JSON with current state

  ============================================================
  SONGS INCLUDED
  ============================================================

  0. Jingle Bells
  1. Happy Birthday
  2. Twinkle Twinkle Little Star
  3. Mary Had a Little Lamb
  4. Ode to Joy

  ============================================================
  NOTE MAPPING REFERENCE
  ============================================================

  Key Index | Note | Frequency
     0      |  C4  | 261.63 Hz  -> LED 1
     1      |  D4  | 293.66 Hz  -> LED 2
     2      |  E4  | 329.63 Hz  -> LED 3
     3      |  F4  | 349.23 Hz  -> LED 4
     4      |  G4  | 392.00 Hz  -> LED 5
     5      |  A4  | 440.00 Hz  -> LED 6
     6      |  B4  | 493.88 Hz  -> LED 7
     7      |  C5  | 523.25 Hz  -> LED 8
     8      |  D5  | 587.33 Hz  -> LED 9
     9      |  E5  | 659.25 Hz  -> LED 10
    10      |  F5  | 698.46 Hz  -> LED 11
    11      |  G5  | 783.99 Hz  -> LED 12
    12      |  A5  | 880.00 Hz  -> LED 13
    13      |  C6  |1046.50 Hz  -> LED 14

*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// WIFI CREDENTIALS
// ============================================================

const char* WIFI_SSID     = "iPhone";
const char* WIFI_PASSWORD = "31323132";

// ============================================================
// WEB SERVER
// ============================================================

WebServer server(80);

// ============================================================
// PIN DEFINITIONS
// ============================================================

const int ledPins[14] = {
  13, 12, 14, 27, 26, 25, 33, 32,
  23, 22, 21, 19, 5, 18
};

// ============================================================
// TIMING
// ============================================================

#define BEAT_MS     700
#define GAP_MS       60

// ============================================================
// NOTE INDEX DEFINITIONS
// ============================================================

#define C4   0
#define D4   1
#define E4   2
#define F4   3
#define G4   4
#define A4   5
#define B4   6
#define C5   7
#define D5   8
#define E5   9
#define F5  10
#define G5  11
#define A5  12
#define C6  13
#define REST 99

// ============================================================
// SONG DATA
// ============================================================

struct Note {
  int   key;
  float beats;
};

struct Song {
  const char  *name;
  const Note  *notes;
  int          length;
};

const Note jingleBells[] = {
  {E4,1},{E4,1},{E4,2},
  {E4,1},{E4,1},{E4,2},
  {E4,1},{G4,1},{C4,1},{D4,1},
  {E4,4},
  {F4,1},{F4,1},{F4,1},{F4,1},
  {F4,1},{E4,1},{E4,1},{E4,1},
  {E4,1},{D4,1},{D4,1},{E4,1},
  {D4,2},{G4,2},
  {E4,1},{E4,1},{E4,2},
  {E4,1},{E4,1},{E4,2},
  {E4,1},{G4,1},{C4,1},{D4,1},
  {E4,4},
  {F4,1},{F4,1},{F4,1},{F4,1},
  {F4,1},{E4,1},{E4,1},{E4,1},
  {G4,1},{G4,1},{F4,1},{D4,1},
  {C4,4},
};

const Note happyBirthday[] = {
  {C4,0.75f},{C4,0.25f},{D4,1},
  {C4,1},{F4,1},{E4,2},
  {C4,0.75f},{C4,0.25f},{D4,1},
  {C4,1},{G4,1},{F4,2},
  {C4,0.75f},{C4,0.25f},{C5,1},
  {A4,1},{F4,1},{E4,1},{D4,1},
  {B4,0.75f},{B4,0.25f},{A4,1},
  {F4,1},{G4,1},{F4,2},
};

const Note twinkle[] = {
  {C4,1},{C4,1},{G4,1},{G4,1},
  {A4,1},{A4,1},{G4,2},
  {F4,1},{F4,1},{E4,1},{E4,1},
  {D4,1},{D4,1},{C4,2},
  {G4,1},{G4,1},{F4,1},{F4,1},
  {E4,1},{E4,1},{D4,2},
  {G4,1},{G4,1},{F4,1},{F4,1},
  {E4,1},{E4,1},{D4,2},
  {C4,1},{C4,1},{G4,1},{G4,1},
  {A4,1},{A4,1},{G4,2},
  {F4,1},{F4,1},{E4,1},{E4,1},
  {D4,1},{D4,1},{C4,2},
};

const Note maryLamb[] = {
  {E4,1},{D4,1},{C4,1},{D4,1},
  {E4,1},{E4,1},{E4,2},
  {D4,1},{D4,1},{D4,2},
  {E4,1},{G4,1},{G4,2},
  {E4,1},{D4,1},{C4,1},{D4,1},
  {E4,1},{E4,1},{E4,1},{E4,1},
  {D4,1},{D4,1},{E4,1},{D4,1},
  {C4,4},
};

const Note odeToJoy[] = {
  {E4,1},{E4,1},{F4,1},{G4,1},
  {G4,1},{F4,1},{E4,1},{D4,1},
  {C4,1},{C4,1},{D4,1},{E4,1},
  {E4,1.5f},{D4,0.5f},{D4,2},
  {E4,1},{E4,1},{F4,1},{G4,1},
  {G4,1},{F4,1},{E4,1},{D4,1},
  {C4,1},{C4,1},{D4,1},{E4,1},
  {D4,1.5f},{C4,0.5f},{C4,2},
  {D4,1},{D4,1},{E4,1},{C4,1},
  {D4,1},{E4,0.5f},{F4,0.5f},{E4,1},{C4,1},
  {D4,1},{E4,0.5f},{F4,0.5f},{E4,1},{D4,1},
  {C4,1},{D4,1},{G4,2},
  {E4,1},{E4,1},{F4,1},{G4,1},
  {G4,1},{F4,1},{E4,1},{D4,1},
  {C4,1},{C4,1},{D4,1},{E4,1},
  {D4,1.5f},{C4,0.5f},{C4,2},
};

const Song songs[] = {
  { "Jingle Bells",           jingleBells,   sizeof(jingleBells)   / sizeof(Note) },
  { "Happy Birthday",         happyBirthday, sizeof(happyBirthday) / sizeof(Note) },
  { "Twinkle Twinkle",        twinkle,       sizeof(twinkle)       / sizeof(Note) },
  { "Mary Had a Little Lamb", maryLamb,      sizeof(maryLamb)      / sizeof(Note) },
  { "Ode to Joy",             odeToJoy,      sizeof(odeToJoy)      / sizeof(Note) },
};

const int SONG_COUNT = sizeof(songs) / sizeof(Song);

// ============================================================
// STATE
// ============================================================

int           currentSong    = 0;
int           noteIndex      = 0;
bool          playing        = false;
bool          paused         = false;
bool          inGap          = false;
unsigned long noteStartMs    = 0;
unsigned long noteDurationMs = 0;
unsigned long gapStartMs     = 0;

// ============================================================
// LED HELPERS
// ============================================================

void allLedsOn()
{
  for (int i = 0; i < 14; i++) digitalWrite(ledPins[i], HIGH);
}

void allLedsOff()
{
  for (int i = 0; i < 14; i++) digitalWrite(ledPins[i], LOW);
}

void showKey(int key)
{
  allLedsOff();
  if (key >= 0 && key < 14) digitalWrite(ledPins[key], HIGH);
}

// ============================================================
// SONG CONTROL
// ============================================================

void startSong()
{
  noteIndex        = 0;
  playing          = true;
  paused           = false;
  inGap            = false;
  Serial.print(">> PLAYING: ");
  Serial.println(songs[currentSong].name);
  allLedsOff();
  const Note &n    = songs[currentSong].notes[0];
  noteDurationMs   = (unsigned long)(n.beats * BEAT_MS) - GAP_MS;
  if (n.key != REST) showKey(n.key);
  noteStartMs      = millis();
}

void stopSong()
{
  playing   = false;
  paused    = false;
  inGap     = false;
  noteIndex = 0;
  allLedsOn();
}

void nextSong()
{
  playing     = false;
  paused      = false;
  inGap       = false;
  noteIndex   = 0;
  currentSong = (currentSong + 1) % SONG_COUNT;
  Serial.print(">> NEXT: ");
  Serial.println(songs[currentSong].name);
  startSong();
}

void selectSong(int idx)
{
  if (idx < 0 || idx >= SONG_COUNT) return;
  playing     = false;
  paused      = false;
  inGap       = false;
  noteIndex   = 0;
  currentSong = idx;
  Serial.print(">> SELECTED: ");
  Serial.println(songs[currentSong].name);
  startSong();
}

// ============================================================
// PLAYBACK UPDATE
// ============================================================

void updatePlayback()
{
  if (!playing || paused) return;

  unsigned long now = millis();

  if (!inGap)
  {
    if (now - noteStartMs >= noteDurationMs)
    {
      allLedsOff();
      inGap      = true;
      gapStartMs = now;
    }
  }
  else
  {
    if (now - gapStartMs >= GAP_MS)
    {
      noteIndex++;
      if (noteIndex >= songs[currentSong].length)
      {
        Serial.println(">> SONG DONE");
        currentSong = (currentSong + 1) % SONG_COUNT;
        Serial.print(">> AUTO NEXT: ");
        Serial.println(songs[currentSong].name);
        startSong();
        return;
      }
      const Note &n  = songs[currentSong].notes[noteIndex];
      noteDurationMs = (unsigned long)(n.beats * BEAT_MS) - GAP_MS;
      if (noteDurationMs < 20) noteDurationMs = 20;
      showKey(n.key);
      noteStartMs = millis();
      inGap       = false;
    }
  }
}

// ============================================================
// WEB SERVER HANDLERS
// ============================================================

// Add CORS headers so the page can call the API freely
void addCors()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

void handlePlay()
{
  addCors();
  if (!playing)       startSong();
  else if (paused)  { paused = false; Serial.println(">> RESUMED"); }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handlePause()
{
  addCors();
  if (playing && !paused)
  {
    paused = true;
    allLedsOff();
    Serial.println(">> PAUSED");
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleNext()
{
  addCors();
  nextSong();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleSong()
{
  addCors();
  if (server.hasArg("i"))
  {
    int idx = server.arg("i").toInt();
    selectSong(idx);
    server.send(200, "application/json", "{\"ok\":true}");
  }
  else
  {
    server.send(400, "application/json", "{\"error\":\"missing i param\"}");
  }
}

void handleStatus()
{
  addCors();
  // Build a small JSON with current state so the web page can update itself
  String json = "{";
  json += "\"song\":"    + String(currentSong)              + ",";
  json += "\"name\":\""  + String(songs[currentSong].name)  + "\",";
  json += "\"playing\":"  + String(playing  ? "true" : "false") + ",";
  json += "\"paused\":"   + String(paused   ? "true" : "false") + ",";
  json += "\"note\":"    + String(noteIndex)                + ",";
  json += "\"total\":"   + String(songs[currentSong].length) + ",";
  json += "\"songCount\":" + String(SONG_COUNT);
  json += "}";
  server.send(200, "application/json", json);
}

// The web app HTML is served directly from the ESP32.
// Open the ESP32's IP in your phone browser - no separate file needed.
void handleRoot()
{
  // NOTE: This serves a minimal redirect page.
  // The full web app (piano_guide_app.html) should be opened directly
  // by replacing the IP address shown in Serial Monitor.
  // Or use the full self-contained app HTML file provided separately.
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Piano Guide</title>";
  html += "<style>body{font-family:sans-serif;text-align:center;padding:40px;background:#111;color:#fff;}";
  html += "h2{color:#facc15;}p{color:#aaa;}a{color:#4ade80;font-size:1.2em;}</style></head><body>";
  html += "<h2>ESP32 Piano Guide</h2>";
  html += "<p>Connected! Open <b>piano_guide_app.html</b> in your browser</p>";
  html += "<p style='margin-top:30px;color:#666;font-size:0.85em'>API ready at this IP</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleNotFound()
{
  server.send(404, "text/plain", "Not found");
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);
  delay(500);

  // LED pins
  for (int i = 0; i < 14; i++)
  {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Blink all LEDs once to confirm power on
  allLedsOn();
  delay(400);
  allLedsOff();
  delay(200);
  allLedsOn();

  // Connect to WiFi
  Serial.println("\n==============================================");
  Serial.println("  ESP32 LED SONG GUIDE - WiFi Version");
  Serial.println("==============================================");
  Serial.print("  Connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n  WiFi connected!");
    Serial.print("  IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("  Open this IP in your phone browser.");
    Serial.println("==============================================");
  }
  else
  {
    Serial.println("\n  WiFi FAILED. Check SSID/password.");
    Serial.println("  Running without WiFi control.");
  }

  // Register web server routes
  server.on("/",       handleRoot);
  server.on("/play",   handlePlay);
  server.on("/pause",  handlePause);
  server.on("/next",   handleNext);
  server.on("/song",   handleSong);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("  Web server started.");
  Serial.print("  Selected: ");
  Serial.println(songs[currentSong].name);
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  server.handleClient();   // Handle any incoming web requests
  updatePlayback();        // Advance LED sequence
}
