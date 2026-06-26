// Single-chip (ESP32-C3 Mini): WiFi AP + WebSocket scoreboard + LDR goal sensors
// + DFRobot DF1201S MP3 player (same wiring and audio logic as MCodeFDMOld).
//
// Web UI (index.html, style.css, app.js) lives in LittleFS under /web/.
// Use the Arduino IDE "ESP32 LittleFS Data Upload" plugin to upload the data/ folder.
// If /web/ is empty the firmware serves a small embedded fallback page instead.

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include <DFRobot_DF1201S.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DF1201SSerial Serial1

LiquidCrystal_I2C lcd(0x20, 16, 2);

const char* AP_SSID = "Punhada";
const char* AP_PASS = "your_password_here";

// ── Sensor pins ───────────────────────────────────────────────────────────────
const int LDR1_PIN = 0;             // GPIO0 — Benfica goal
const int LDR2_PIN = 1;             // GPIO1 — Sporting goal
const int LDR_THRESHOLD   = 750;
const unsigned long DEBOUNCE_MS = 1000;

// ── Idle sound: play file 7 after ~3 minutes of no goals (same as old system) ─
const unsigned long IDLE_MS = 180000UL;  // 180 s ≈ 3000 × 60 ms loop
unsigned long lastEventMs   = 0;
bool idlePlayed             = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DFRobot_DF1201S DF1201S;

int goal1 = 0;   // Benfica
int goal2 = 0;   // Sporting
String currentState = "playing";

unsigned long lastGoal1Ms = 0;
unsigned long lastGoal2Ms = 0;

// ── Embedded fallback page ────────────────────────────────────────────────────
const char fallback_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="pt"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Setup - Punhada</title>
<style>
  body { font-family: Arial, sans-serif; background:#0d0d1a; color:#ddd; padding:30px; max-width:600px; margin:0 auto; }
  h1 { color:#DA291C; margin-bottom:16px; }
  code { background:#1a1a2e; padding:2px 6px; border-radius:3px; color:#00a651; }
  ol li { margin:8px 0; line-height:1.5; }
</style></head><body>
  <h1>Primeira configuração</h1>
  <p>O ESP32 está a funcionar, mas os ficheiros web ainda não foram carregados para o LittleFS.</p>
  <ol>
    <li>No Arduino IDE, instala o plugin <b>ESP32 LittleFS Data Upload</b>.</li>
    <li>Confirma que existe a pasta <code>data/web/</code> ao lado do <code>.ino</code>.</li>
    <li>Selecciona <b>Tools → ESP32 Sketch Data Upload</b> e espera pelo fim.</li>
    <li>Reinicia o ESP32 e recarrega esta página.</li>
  </ol>
  <p>AP: Punhada &nbsp;|&nbsp; IP: 192.168.4.1</p>
</body></html>
)rawliteral";

// ── Audio helpers (same file mapping as MCodeFDMOld) ─────────────────────────
//  1 = Sporting goal (main)   5 = Benfica wins
//  2 = Benfica goal (main)    6 = Sporting wins
//  3 = Startup jingle         7 = Idle ambient
//  4 = Shared goal alt        8 = Shared goal rare
//  9 = Negra (3-3)

void playCustomBenfica() {
  switch (random(0, 6)) {
    case 0:  DF1201S.playFileNum(4); break;
    case 5:  DF1201S.playFileNum(8); break;
    default: DF1201S.playFileNum(2); break;
  }
}

void playCustomSporting() {
  switch (random(0, 6)) {
    case 0:  DF1201S.playFileNum(4); break;
    case 5:  DF1201S.playFileNum(8); break;
    default: DF1201S.playFileNum(1); break;
  }
}

// ── WebSocket helpers ─────────────────────────────────────────────────────────

String buildPayload(const String& event = "") {
  String json = "{\"benfica\":" + String(goal1) +
                ",\"sporting\":" + String(goal2) +
                ",\"state\":\"" + currentState + "\"";
  if (event.length() > 0) json += ",\"event\":\"" + event + "\"";
  json += "}";
  return json;
}

void broadcastScore(const String& event = "") {
  ws.textAll(buildPayload(event));
}

// ── LCD helper ────────────────────────────────────────────────────────────────

void updateLcd(const char* line0 = nullptr, const char* line1 = nullptr) {
  lcd.clear();
  if (line0) { lcd.setCursor(0, 0); lcd.print(line0); }
  if (line1) { lcd.setCursor(0, 1); lcd.print(line1); }
}

void showScore() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Benfica  " + String(goal1));
  lcd.setCursor(0, 1); lcd.print("Sporting " + String(goal2));
}

// ── Game logic ────────────────────────────────────────────────────────────────

void handleGoal(int goalNum) {
  lastEventMs = millis();
  idlePlayed  = false;

  if (goalNum == 1) {
    goal1++;
    Serial.println("Benfica: " + String(goal1));
    if (goal1 >= 4) {
      currentState = "benfica_wins";
      broadcastScore("benfica_wins");
      updateLcd("Sorte", "");
      DF1201S.playFileNum(5);
      delay(3000);
      goal1 = 0; goal2 = 0;
      currentState = "playing";
      broadcastScore();
      showScore();
      return;
    }
    if (goal1 == 3 && goal2 == 3) {
      currentState = "negra";
      broadcastScore("negra");
      updateLcd("NEGRA!", "");
      DF1201S.playFileNum(9);
    } else {
      currentState = "playing";
      broadcastScore("goal_benfica");
      showScore();
      playCustomBenfica();
    }
  } else {
    goal2++;
    Serial.println("Sporting: " + String(goal2));
    if (goal2 >= 4) {
      currentState = "sporting_wins";
      broadcastScore("sporting_wins");
      updateLcd("SCP Grandioso!", "");
      DF1201S.playFileNum(6);
      delay(3000);
      goal1 = 0; goal2 = 0;
      currentState = "playing";
      broadcastScore();
      showScore();
      return;
    }
    if (goal1 == 3 && goal2 == 3) {
      currentState = "negra";
      broadcastScore("negra");
      updateLcd("NEGRA!", "");
      DF1201S.playFileNum(9);
    } else {
      currentState = "playing";
      broadcastScore("goal_sporting");
      showScore();
      playCustomSporting();
    }
  }
}

// ── WebSocket event ───────────────────────────────────────────────────────────
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text(buildPayload());
  } else if (type == WS_EVT_DATA) {
    String msg = String((char *)data).substring(0, len);
    if (msg == "reset") {
      goal1 = 0; goal2 = 0;
      currentState = "playing";
      broadcastScore();
      showScore();
    }
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  updateLcd("Punhada", "a iniciar...");

  DF1201SSerial.begin(115200, SERIAL_8N1, 20, 21);
  while (!DF1201S.begin(DF1201SSerial)) {
    Serial.println("DF1201S init failed — check wiring");
    delay(1000);
  }
  DF1201S.setVol(19);
  DF1201S.switchFunction(DF1201S.MUSIC);
  DF1201S.setPlayMode(DF1201S.SINGLE);
  DF1201S.setLED(false);
  DF1201S.setPrompt(false);
  Serial.print("DF1201S files: ");
  Serial.println(DF1201S.getTotalFile());
  delay(1000);
  DF1201S.playFileNum(3);   // startup jingle

  pinMode(LDR1_PIN, INPUT);
  pinMode(LDR2_PIN, INPUT);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.print("LittleFS: ");
    Serial.print(LittleFS.usedBytes() / 1024); Serial.print(" / ");
    Serial.print(LittleFS.totalBytes() / 1024); Serial.println(" KB");
    Serial.print("Web assets: ");
    Serial.println(LittleFS.exists("/web/index.html") ? "found" : "MISSING (using embedded fallback)");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    if (LittleFS.exists("/web/index.html"))
      req->send(LittleFS, "/web/index.html", "text/html");
    else
      req->send_P(200, "text/html", fallback_html);
  });

  server.serveStatic("/", LittleFS, "/web/");
  server.begin();

  lastEventMs = millis();
  showScore();
  Serial.println("Ready — WiFi: " + String(AP_SSID) + " / " + String(AP_PASS));
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  ws.cleanupClients();

  unsigned long now = millis();

  if (analogRead(LDR1_PIN) < LDR_THRESHOLD && now - lastGoal1Ms > DEBOUNCE_MS) {
    lastGoal1Ms = now;
    handleGoal(1);
  }

  if (analogRead(LDR2_PIN) < LDR_THRESHOLD && now - lastGoal2Ms > DEBOUNCE_MS) {
    lastGoal2Ms = now;
    handleGoal(2);
  }

  // Idle ambient sound — same cadence as old system (~3 min of inactivity)
  if (!idlePlayed && now - lastEventMs > IDLE_MS) {
    DF1201S.playFileNum(7);
    idlePlayed = true;
  }
}
