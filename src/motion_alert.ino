#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// =========================
// WLAN
// =========================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// =========================
// IFTTT
// =========================
const char* iftttKey = "YOUR_IFTTT_KEY";
const char* eventName = "pir_event";

// =========================
// PIR Sensor
// =========================
const int pirPin = D1; // PIR OUT an D1

// =========================
// Logik & Schutz
// =========================
bool eventLocked = false;
bool motionOngoing = false;

unsigned long motionStart = 0;
unsigned long lockStart = 0;
unsigned long lastQuietTime = 0;

// Zeiten (ms)
const unsigned long minMotionTime = 500;  // echte Bewegung (0.5s)
const unsigned long lockTime = 15000;     // 15s Mail-Sperre
const unsigned long quietTime = 3000;     // 3s Ruhe noetig

// =========================
// WLAN verbinden
// =========================
void connectWifi() {
  WiFi.begin(ssid, password);
  Serial.print("Verbinde WLAN");

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWLAN verbunden!");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWLAN NICHT verbunden (Timeout)");
  }
}

// =========================
// IFTTT ausloesen
// =========================
void triggerIFTTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Kein WLAN -> Mail nicht gesendet");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://maker.ifttt.com/trigger/";
  url += eventName;
  url += "/with/key/";
  url += iftttKey;

  http.begin(client, url);
  int httpCode = http.GET();
  http.end();

  Serial.print("IFTTT HTTP Code: ");
  Serial.println(httpCode);
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(9600);
  pinMode(pirPin, INPUT_PULLUP);

  Serial.println("=== PIR + IFTTT MAIL TEST START ===");
  connectWifi();
}

// =========================
// LOOP
// =========================
void loop() {
  int pirValue = digitalRead(pirPin); // LOW = Bewegung
  unsigned long now = millis();

  // Bewegung startet
  if (pirValue == LOW && !motionOngoing && !eventLocked) {
    motionOngoing = true;
    motionStart = now;
  }

  // Bewegung lange genug -> Mail
  if (motionOngoing && !eventLocked &&
      (now - motionStart >= minMotionTime)) {

    Serial.print(now);
    Serial.println(" ms | >>> BEWEGUNG ERKANNT -> MAIL");

    triggerIFTTT();

    eventLocked = true;
    lockStart = now;
  }

  // Ruhe erkannt
  if (pirValue == HIGH) {
    motionOngoing = false;
    lastQuietTime = now;
  }

  // System wieder freigeben
  if (eventLocked &&
      (now - lockStart >= lockTime) &&
      (now - lastQuietTime >= quietTime)) {

    eventLocked = false;

    Serial.print(now);
    Serial.println(" ms | <<< Wieder bereit");
  }

  delay(50);
}
