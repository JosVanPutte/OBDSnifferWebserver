#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32-TWAI-CAN.hpp>
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

uint16_t PID;

// non volatile ram
nvs_handle_t nvs;

String ssid = "";
String password = "";

void tryConnect() {
  String ap_mode = getNonVolatile(nvs, "ap_mode");
  ssid = getNonVolatile(nvs, "ssid");
  password = getNonVolatile(nvs, "password");
  if (ap_mode == "1" || ssid.isEmpty()) {
    Serial.println("AP mode WiFi 'OBDSniffer'");
    if (!WiFi.softAP("OBDSniffer", "")) {
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
boolean jsonValid = false;

const char *getConfig(const char *id) {
  if (jsonValid && jsonConfig.containsKey(id)) {
    return (const char *)jsonConfig[id];
  }
  return "";
}
int getIntConfig(const char *id) {
  String val = getConfig(id);
  if (val.isEmpty()) {
    return 0;
  }
  return atoi(val.c_str());
}

float getFloatConfig(const char *id) {
  String val = getConfig(id);
  if (val.isEmpty()) {
    return 0;
  }
  return atof(val.c_str());
}

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
    PID = std::stoi(getConfig("code"), NULL, 16);
    Serial.printf("label %s PID %s = %d\n", getConfig("name"),getConfig("code"), PID);
    jsonValid = true;
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

// Can Frame
CanFrame CANFrame;

// get the 16 bits value at the byte offset
const uint16_t getInt16(const uint8_t *charPtr, bool bigEndian) {
  if (!bigEndian) {
    return *(const uint16_t *) charPtr;
  }
  uint8_t little = *charPtr++;
  uint8_t big = *charPtr;
  return big << 8 + little;
}
// Find the PID in the data of an OBD2 frame. OBD2 uses a 2-byte PID for Extended CAN frames, but 1 byte for Standard CAN frames
uint16_t getPID(const CanFrame& frame)
{
  uint8_t pidLengthInBytes = frame.extd ? 2 : 1;
  return (pidLengthInBytes == 1) ? frame.data[2] : getInt16(&frame.data[2], false);
}

void CANReader(void *par) {
  while (1) {
    CanFrame receivedOBD2Frame;
    while(ESP32Can.readFrame(receivedOBD2Frame)) {
      uint16_t msgPID = getPID(receivedOBD2Frame);
      if (msgPID == PID) {
         CANFrame = receivedOBD2Frame;
      }
    }
    delay(1);
  }
}
float getCANValue() {
  int offset = getIntConfig("offset");
  uint16_t intVal;
  if (getIntConfig("bytes") == 2) {
    intVal = getInt16(&CANFrame.data[offset], strcmp("little", getConfig("endian")));
  } else {
    intVal = CANFrame.data[offset];
  }
  float value = intVal * getFloatConfig("factor");
  if (!strcmp("integer", getConfig("datatype"))) {
    value = round(value);
  }
  return value;
}

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
      jsonValid = true;
      Serial.println("deserialization OK");
      PID = std::stoi(getConfig("code"), NULL, 16);
      Serial.printf("label %s PID %s = %d\n", getConfig("name"),getConfig("code"), PID);
    }
  }

  // Web routes
  OBDServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)rootHtml, strlen(rootHtml));
  });
  OBDServer.on("/get-value", [](AsyncWebServerRequest *request) {
    float CANValue = getCANValue();
    int nrDecimals = strcmp("integer", getConfig("datatype")) ? 1 : 0;
    String json = "{\"waarde\": " + String(CANValue, nrDecimals) + "}";
    request->send(200, "application/json", json);
  });
    
  OBDServer.on("/reset-wifi.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("configuratie GOT params =");
    Serial.println(pars);
    String wifiPage = "/wifi.html";
    if (!ssid.isEmpty()) {
      wifiPage.concat( "?ssid=");
      wifiPage.concat(ssid);
      wifiPage.concat("&password=");
      wifiPage.concat(password);
      wifiPage.concat("&ap_mode=");
      wifiPage.concat(getNonVolatile(nvs, "ap_mode"));
      wifiPage.concat("&swap_mode=");
      wifiPage.concat(getNonVolatile(nvs, "swap_mode"));
       while(--pars >= 0) {
        const AsyncWebParameter *p = request->getParam(pars);
        Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
        wifiPage.concat("&");
        wifiPage.concat(p->name());
        wifiPage.concat("=");
        wifiPage.concat(p->value());
      }
    }
    Serial.println(wifiPage);
    request->redirect(wifiPage);
  });
  OBDServer.on("/wifi.html", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", (uint8_t *)resetWiFiHtml, strlen(resetWiFiHtml));
  });
  OBDServer.on("/configuratie.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("configuratie GOT params =");
    Serial.println(pars);
    if (!strlen(getConfig("viz_type"))) {
      request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
    } else {
      String configPage = "/config.html?name=";
      configPage.concat(getConfig("name"));
      configPage.concat("&code=");
      configPage.concat(getConfig("code"));
      configPage.concat("&offset=");
      configPage.concat(getConfig("offset"));
      configPage.concat("&number=");
      configPage.concat(getConfig("number"));
      configPage.concat("&datatype=");
      configPage.concat(getConfig("datatype"));
      configPage.concat("&endian=");
      configPage.concat(getConfig("endian"));
      configPage.concat("&max_value=");
      configPage.concat(getConfig("max_value"));
      configPage.concat("&factor=");
      configPage.concat(getConfig("factor"));
      configPage.concat("&viz_type=");
      configPage.concat(getConfig("viz_type"));
      configPage.concat("&bytes=");
      configPage.concat(getConfig("bytes"));
      while(--pars >= 0) {
        const AsyncWebParameter *p = request->getParam(pars);
        Serial.printf("%s = %s\n", p->name().c_str(), p->value().c_str());
        configPage.concat("&");
        configPage.concat(p->name());
        configPage.concat("=");
        configPage.concat(p->value());
      }
      request->redirect(configPage);
    }
  });
  OBDServer.on("/config.html", [](AsyncWebServerRequest *request) {
      int pars = request->params();
      Serial.print("config GOT params =");
      Serial.println(pars);
      request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
  });
  OBDServer.on("/visualisation.html", [](AsyncWebServerRequest *request) {
    if (!strlen(getConfig("viz_type"))) {
      request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
    } else {
      String testingPage = "/visualisation";
      testingPage.concat(getConfig("viz_type"));
      testingPage.concat(".html?label=");
      testingPage.concat(getConfig("name"));
      testingPage.concat("&max=");
      testingPage.concat(getConfig("max_value"));
      Serial.println(testingPage);
      request->redirect(testingPage);
    }
  });
  OBDServer.on("/test.html", [](AsyncWebServerRequest *request) {
    if (!strlen(getConfig("viz_type"))) {
      request->send(200, "text/html", (uint8_t *)configHtml, strlen(configHtml));
    } else {
      String testingPage = "/test";
      testingPage.concat(getConfig("viz_type"));
      testingPage.concat(".html?label=");
      testingPage.concat(getConfig("name"));
      testingPage.concat("&max=");
      testingPage.concat(getConfig("max_value"));
      Serial.println(testingPage);
      request->redirect(testingPage);
    }
  });
  OBDServer.on("/testnumber.html", [](AsyncWebServerRequest *request) {
    int pars = request->params();
    Serial.print("testNumber GOT params =");
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
    if (!strlen(getConfig("viz_type"))) {
      request->send(404, "text/plain", "No value configured");
    } else {
      String testingPage = "/monitor";
      testingPage.concat(getConfig("viz_type"));
      testingPage.concat(".html?label=");
      testingPage.concat(getConfig("name"));
      testingPage.concat("&max=");
      testingPage.concat(getConfig("max_value"));
      Serial.println(testingPage);
      request->redirect(testingPage);
    }
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
  OBDServer.on("/wifi", HTTP_POST, wifi);
  OBDServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  OBDServer.begin();
  Serial.println("Web server started. Open the IP in your browser.");
  // CAN frames with non-valid OBD2 data can be received on the CAN bus. We're not interested in those, therefore we want to filter them out.
  // Valid extended CAN IDs are in the range [0x18DAF100 .. 0x18DAF1FF], so we setup a filter to only receive CAN IDs in the range [0x18000000 .. 0x18FFFFFF]
  twai_filter_config_t canFilter;
  canFilter.acceptance_code = 0x18000000U << 3;
  canFilter.acceptance_mask = 0x00FFFFFFU << 3;
  canFilter.single_filter = true;
  int canL = 4;
  int canH = 5;
  if (!strcmp("1", getNonVolatile(nvs, "swap_mode").c_str())) {
    Serial.println("CAN H and L swapped");
    canH = 4;
    canL = 5;
  }
  while (!ESP32Can.begin(TWAI_SPEED_500KBPS, canL, canH, 16, 16, &canFilter)) {
    Serial.println("CAN bus init failed");
    delay(5000);
  }
  TaskHandle_t Task0;
  xTaskCreatePinnedToCore(CANReader, "CANReader",  10000, NULL, 0,  &Task0, 0); 
  Serial.println("CAN bus started!");
}


void loop() {
  delay(1);
}
