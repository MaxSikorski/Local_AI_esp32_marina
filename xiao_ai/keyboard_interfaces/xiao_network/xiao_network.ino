 
 
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <cstring>
#include <cstdint>
#include "src/display/TFT_eSPI.h"
#include "google_root_certificates.h"

static String MODEL;
static String ollamaURL;
static bool useOllama = false;
constexpr size_t PROD_INPUT_BYTES = 256;
constexpr size_t PROD_DECODE_BYTES = 2048;
TFT_eSPI tft;
static String apiKey, wifiName, serialLine;
static bool apiReady = false;
static unsigned configStage = 0;
static bool discardLine = false;

bool askGemini(const String& question, String& answer) {
  if (!apiReady) { answer = "Set up Wi-Fi and API in Serial Monitor."; return false; }
  if (WiFi.status() != WL_CONNECTED) { answer = "Wi-Fi disconnected. Try again or /setup."; return false; }
  if (time(nullptr) < 1700000000) { answer = "Clock not ready. Try again in a moment."; return false; }
  WiFiClientSecure client;
  client.setCACert(ROOT_CA);
  client.setHandshakeTimeout(25);
  HTTPClient http;
  String url = String("https://generativelanguage.googleapis.com/v1beta/models/") + MODEL + ":generateContent";
  if (!http.begin(client, url)) { answer = "HTTPS setup failed."; return false; }
  http.useHTTP10(true);  
  http.setConnectTimeout(15000);
  http.setTimeout(30000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-goog-api-key", apiKey);
  JsonDocument req;
  req["system_instruction"]["parts"][0]["text"] =
      "Reply in English using plain ASCII, no markdown. Keep the answer to one short sentence under 100 characters so it fits a small display.";
  req["contents"][0]["role"] = "user";
  req["contents"][0]["parts"][0]["text"] = question;
  req["generationConfig"]["maxOutputTokens"] = 512;
  String payload;
  serializeJson(req, payload);
  int status = http.POST(payload);
  if (status != 200) {
     
    answer = status > 0 ? String("API HTTP ") + status + ". Check key, quota and model."
                        : "HTTPS connection failed. Check Wi-Fi and clock.";
    Serial.printf("[API] HTTP status: %d\n", status);
    http.end();
    return false;
  }
   
   
  JsonDocument filter;
  filter["candidates"][0]["content"]["parts"][0]["text"] = true;
  filter["candidates"][0]["content"]["parts"][0]["thought"] = true;
  JsonDocument response;
  client.setTimeout(30000);
  auto error = deserializeJson(response, http.getStream(), DeserializationOption::Filter(filter));
  if (error) {
    answer = "Could not read API response. Try again.";
    http.end();
    return false;
  }
  for (JsonObject part : response["candidates"][0]["content"]["parts"].as<JsonArray>()) {
    if (part["thought"] | false) continue;
    answer += part["text"] | "";
  }
  http.end();
  answer.trim();
  if (answer.isEmpty()) { answer = "No text returned. Try another question."; return false; }
  return true;
}

bool askAI(const String& question, String& answer) {
  if (!useOllama) return askGemini(question, answer);
  if (!apiReady || WiFi.status() != WL_CONNECTED) { answer = "Configure Wi-Fi with /setup."; return false; }
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, ollamaURL + "/api/generate")) { answer = "Invalid Ollama address."; return false; }
  http.useHTTP10(true);
  http.setConnectTimeout(15000);
  http.setTimeout(60000);
  http.addHeader("Content-Type", "application/json");
  JsonDocument request;
  request["model"] = MODEL;
  request["prompt"] = question;
  request["system"] = "Reply in plain ASCII with one short sentence under 100 characters.";
  request["stream"] = false;
  request["think"] = false;
  request["options"]["num_predict"] = 256;
  String payload;
  serializeJson(request, payload);
  int status = http.POST(payload);
  if (status != 200) { answer = "Ollama request failed. Check host and model."; http.end(); return false; }
  JsonDocument filter, response;
  filter["response"] = true;
  client.setTimeout(60000);
  auto error = deserializeJson(response, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (error) { answer = "Could not read Ollama response."; return false; }
  answer = response["response"] | "";
  int endThink = answer.lastIndexOf("</think>");
  if (endThink >= 0) answer = answer.substring(endThink + 8);
  else if (answer.indexOf("<think>") >= 0) answer = "";
  answer.trim();
  if (answer.isEmpty()) { answer = "No final answer returned. Try again."; return false; }
  return true;
}

 
#include "touchscreen_keyboard.h"

void showStatus(const char* message) {
  lcdchat::last_user[0] = 0;
  strlcpy(lcdchat::last_reply, message, sizeof(lcdchat::last_reply));
  lcdchat::thinking = false;
  lcdchat::last_ok = true;
  lcdchat::drawConversation();
}

void beginConfiguration() {
  apiReady = false;
  configStage = 0;
  apiKey = ""; wifiName = ""; serialLine = ""; ollamaURL = ""; MODEL = "";
  Serial.println("\n115200 baud, Newline. Choose backend: gemini or ollama");
  showStatus("Choose gemini or ollama in Serial Monitor.");
}

void handleSerialLine(String value) {
  if (configStage == 0) {
    value.trim(); value.toLowerCase();
    if (value != "gemini" && value != "ollama") { Serial.println("Enter gemini or ollama:"); return; }
    useOllama = value == "ollama";
    configStage = 1; Serial.println("Wi-Fi SSID:");
  } else if (configStage == 1) {
    if (value.isEmpty()) return;
    wifiName = value; configStage = 2; Serial.println("Wi-Fi password:");
  } else if (configStage == 2) {
    WiFi.mode(WIFI_STA); WiFi.persistent(false); WiFi.setAutoReconnect(true); WiFi.setSleep(false);
    WiFi.begin(wifiName.c_str(), value.c_str()); value = "";
    showStatus("Connecting to Wi-Fi...");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) delay(50);
    if (WiFi.status() != WL_CONNECTED) { configStage = 1; Serial.println("Connection failed. Enter SSID again:"); return; }
    configStage = 3;
    Serial.println(useOllama ? "Ollama base URL: http://<computer-LAN-address>:11434" : "Gemini API key:");
  } else if (configStage == 3) {
    value.trim();
    if (value.isEmpty()) return;
    if (useOllama) {
      while (value.endsWith("/")) value.remove(value.length()-1);
      if (!value.startsWith("http://") || value.indexOf("localhost") >= 0 || value.indexOf("127.0.0.1") >= 0 || value.indexOf('@') >= 0) {
        Serial.println("Use an HTTP address reachable from XIAO on your local network."); return;
      }
      ollamaURL = value;
    } else { apiKey = value; configTime(0, 0, "time.google.com", "pool.ntp.org"); }
    configStage = 4; Serial.println("Model name (exact installed Ollama tag or Gemini model ID):");
  } else if (configStage == 4) {
    value.trim();
    if (value.isEmpty()) return;
    for (size_t i=0;i<value.length();++i) {
      char c=value[i];
      if (!(isalnum(c) || c=='-' || c=='_' || c=='.' || (useOllama && (c==':' || c=='/')))) { Serial.println("Invalid model name."); return; }
    }
    MODEL = value; apiReady = true; configStage = 5;
    showStatus("say hi :)");
    Serial.println("Ready. Use touchscreen GO or enter a question. /setup changes backend.");
  } else {
    value.trim();
    if (value == "/setup") { beginConfiguration(); return; }
    if (!value.isEmpty()) lcdchat::submit(value.c_str());
  }
}

void setup() {
  Serial.begin(115200);
   
  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  lcdchat::init();
  delay(1200);
  beginConfiguration();
}

void loop() {
  while (Serial.available()) {
    char c = char(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (discardLine) {
        Serial.println("Line too long; enter it again.");
        discardLine = false;
      } else {
        String line = serialLine;
        serialLine = "";
        handleSerialLine(line);
      }
      serialLine = "";
    } else if (!discardLine) {
      size_t limit = configStage == 5 ? PROD_INPUT_BYTES-1 : 512;
      if (serialLine.length() < limit) serialLine += c;
      else { serialLine = ""; discardLine = true; }
    }
  }
  if (configStage == 5) lcdchat::updateTouch();
  delay(5);
}
