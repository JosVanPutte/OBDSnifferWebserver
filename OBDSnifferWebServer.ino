#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>
#include <time.h>
#include <ArduinoJson.h>
#include "storage.h"
#include "rootHtml.h"
#include "resetWiFiHtml.h"
#include "configHtml.h"
#include "testNumberHtml.h"
#include "testPieHtml.h"
#include "testBarHtml.h"
#include "testGraphHtml.h"
#include "monitorNumberHtml.h"
#include "monitorPieHtml.h"
#include "monitorBarHtml.h"
#include "monitorGraphHtml.h"


// non volatile ram
nvs_handle_t nvs;

const char *SSID = "OBDSniffer";
void tryConnect() {
  String ap_mode = getNonVolatile(nvs, "ap_mode");
  String ssid = getNonVolatile(nvs, "ssid");
  String password = getNonVolatile(nvs, "password");
  if (ap_mode == "1" || ssid.isEmpty()) {
    Serial.println("AP mode WiFi 'OBDSniffer'");
    if (!WiFi.softAP(SSID, "")) {
      Serial.println("Soft AP creation failed.");
    }
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("OBDSniffer IP address: ");
    Serial.println(myIP);
  } else {
    WiFi.mode(WIFI_STA);
    Serial.print("Connecting to WiFi ssid ");
    Serial.print(ssid);
    Serial.print(" password ");
    Serial.println(password);
    WiFi.begin(ssid, password);
    for (int i=0; i<60; i++) {
      delay(1000);
      if (WiFi.status() == WL_CONNECTED) {
        break;
      }
      if (i == 30) {
        Serial.println(";");
      } else {
        Serial.print(".");
      }
    }
    bool connected = WiFi.status() == WL_CONNECTED;
    Serial.printf(".\n%sconnected to the WiFi.\n", connected ? "" : "NOT ");
    if (!connected) {
      // out in the car we need AP mode
      setNonVolatile(nvs, "ap_mode", "on");
    } else {
      IPAddress myIP = WiFi.localIP();
      Serial.print("OBDSniffer IP address: ");
      Serial.println(myIP);
    }
  }
}
// Web server
AsyncWebServer OBDServer(80);

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
void wifi(AsyncWebServerRequest *request) {
  int pars = request->params();
  Serial.print("GOT HTTP_POST params =");
  Serial.println(pars);
  while(--pars >= 0) {
    const AsyncWebParameter *p = request->getParam(pars);
    Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    setNonVolatile(nvs, p->name().c_str(), p->value().c_str());
  }
  request->send(200, "text/html", (uint8_t *)rootHtml, strlen(rootHtml));
}
float CANValue = 0;

void setup() {
  Serial.begin(115200);
  nvs = initNvs();
  tryConnect();

  String saved = getNonVolatile(nvs, "config");
  if (!saved.isEmpty()) {
    Serial.println(saved);
    DeserializationError error = deserializeJson(jsonConfig, saved.c_str());
    if (error) {
      Serial.printf("failed to deserialize %s", saved.c_str());
    } else {
      Serial.println("deserialization OK");
      Serial.printf("label %s\n", (const char *)jsonConfig["name"]);
    }
  }

  // Web routes
  OBDServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ssid = getNonVolatile(nvs, "ssid");
    String password = getNonVolatile(nvs, "password");
    String homePage = "/index.html";
    if (!ssid.isEmpty()) {
      homePage.concat( "?ssid=");
      homePage.concat(ssid);
      homePage.concat("&password=");
      homePage.concat(password);
    }
    Serial.println(homePage);
    request->redirect(homePage);
  });
  OBDServer.on("/get-value", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"waarde\": " + String(CANValue, 2) + "}";
    request->send(200, "application/json", json);
  });
    
  OBDServer.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)rootHtml, strlen(rootHtml));
  });
  OBDServer.on("/reset-wifi.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)resetWiFiHtml, strlen(resetWiFiHtml));
  });
  OBDServer.on("/configuratie.html", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
  });
  OBDServer.on("/visualisation.html", [](AsyncWebServerRequest *request) {
  String testingPage = "/visualisation";
    testingPage.concat((const char *)jsonConfig["viz_type"]);
    testingPage.concat(".html?label=");
    testingPage.concat((const char *)jsonConfig["name"]);
    testingPage.concat("&max=");
    testingPage.concat((const char *)jsonConfig["max_value"]);
    Serial.println(testingPage);
    request->redirect(testingPage);
    });
  OBDServer.on("/test.html", [](AsyncWebServerRequest *request) {
    String testingPage = "/test";
    testingPage.concat((const char *)jsonConfig["viz_type"]);
    testingPage.concat(".html?label=");
    testingPage.concat((const char *)jsonConfig["name"]);
    testingPage.concat("&max=");
    testingPage.concat((const char *)jsonConfig["max_value"]);
    Serial.println(testingPage);
    request->redirect(testingPage);
  });
  OBDServer.on("/testnumber.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testNumberHtml, strlen(testNumberHtml));
  });
  OBDServer.on("/testpie.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testPieHtml, strlen(testPieHtml));
  });
  OBDServer.on("/testbar.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testBarHtml, strlen(testBarHtml));
  });
  OBDServer.on("/testgraph.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)testGraphHtml, strlen(testGraphHtml));
  });
  OBDServer.on("/monitor.html", [](AsyncWebServerRequest *request) {
    String testingPage = "/monitor";
    testingPage.concat((const char *)jsonConfig["viz_type"]);
    testingPage.concat(".html?label=");
    testingPage.concat((const char *)jsonConfig["name"]);
    testingPage.concat("&max=");
    testingPage.concat((const char *)jsonConfig["max_value"]);
    Serial.println(testingPage);
    request->redirect(testingPage);
  });
  OBDServer.on("/monitornumber.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)monitorNumberHtml, strlen(monitorNumberHtml));
  });
  OBDServer.on("/monitorpie.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)monitorPieHtml, strlen(monitorPieHtml));
  });
  OBDServer.on("/monitorbar.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)monitorBarHtml, strlen(monitorBarHtml));
  });
  OBDServer.on("/monitorgraph.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("GOT params =");
    Serial.println(pars);
    while(--pars >= 0) {
      const AsyncWebParameter *p = request->getParam(pars);
      Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
    }
    request->send(200, "text/html", (uint8_t *)monitorGraphHtml, strlen(monitorGraphHtml));
  });
  OBDServer.on("/config", HTTP_POST, config);
  OBDServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  OBDServer.on("/config", HTTP_POST, config);
  OBDServer.on("/wifi", HTTP_POST, wifi);
  OBDServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  OBDServer.begin();
  Serial.println("Web server started. Open the IP in your browser.");
}

void loop() {
  // 
}