#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <ArduinoJson.h>
#include "storage.h"
#include "rootHtml.h"
#include "configHtml.h"
#include "testNumberHtml.h"
#include "testPieHtml.h"

// ====== CHANGE THESE ======
const char* ssid     = "vanPutte";
const char* password = "vanputte";

// Time / NTP
const char* ntpServer = "pool.ntp.org";

// Timezone: Amsterdam (Europe/Amsterdam) is usually UTC+1, and UTC+2 in DST.
const long gmtOffset_sec = 3600;      // UTC+1
const int daylightOffset_sec = 0;     // Set 3600 if you want to force DST (not automatic)

// non volatile ram
nvs_handle_t nvs;

// Web server
AsyncWebServer server(80);

const size_t capacity = JSON_OBJECT_SIZE(8);
StaticJsonDocument<capacity> jsonConfig;

void config(AsyncWebServerRequest *request) {
  int pars = request->params();
  Serial.print("GOT HTTP_POST params =");
  Serial.println(pars);
  while(--pars >= 0) {
    const AsyncWebParameter *p = request->getParam(pars);
    Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    jsonConfig[p->name().c_str()] = p->value().c_str();
  }
  String configStr;
  if (!serializeJson(jsonConfig, configStr)) {
    Serial.println("Failed to serialize");
  } else {
    Serial.println(configStr) ;
    setNonVolatile(nvs, "config", configStr.c_str());
  }
  request->send(200, "text/html", (uint8_t *)rootHtml, strlen(rootHtml));
}


void setup() {
  Serial.begin(115200);
  nvs = initNvs();

  String saved = getNonVolatile(nvs, "config");
  if (!saved.isEmpty()) {
    Serial.println(saved);
    DeserializationError error = deserializeJson(jsonConfig, saved.c_str());
    if (error) {
      Serial.printf("failed to deserialize %s", saved.c_str());
    } else {
      Serial.printf("deserialization OK");
      Serial.println((const char *)jsonConfig["name"]);
    }
  }
  // Connect Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  // NTP time init
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Web routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)rootHtml, strlen(rootHtml));
  });
  server.on("/configuratie.html", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
  });
  server.on("/test.html", [](AsyncWebServerRequest *request) {
    String testingPage = "/testing";
    testingPage.concat((const char *)jsonConfig["viz_type"]);
    testingPage.concat(".html?label=");
    testingPage.concat((const char *)jsonConfig["name"]);
    testingPage.concat("&max=");
    testingPage.concat((const char *)jsonConfig["max_value"]);
    Serial.println(testingPage);
    request->redirect(testingPage);
  });
  server.on("/testingnumber.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testNumberHtml, strlen(testNumberHtml));
  });
  server.on("/testingpie.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testPieHtml, strlen(testPieHtml));
  });
  server.on("/config", HTTP_POST, config);
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started. Open the IP in your browser.");
}

void loop() {
  // 
}