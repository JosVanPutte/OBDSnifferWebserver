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

#define BUILTIN 2
#define GREEN 26
#define BLUE 27

// non volatile ram
nvs_handle_t nvs;

String ssid = "";
String password = "";

void tryConnect() {
  String ap_mode = getNonVolatile(nvs, "ap_mode");
  ssid = getNonVolatile(nvs, "ssid");
  password = getNonVolatile(nvs, "password");
  if (ap_mode == "1" || ssid.isEmpty()) {
    digitalWrite(GREEN, HIGH);
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

struct CANConfig {
    int offset;
    int bits;
    float factor;
    float minValue;    // De "minimale waarde" voor bytes
    bool isByte;       // True als type "byte" is
    bool isInteger;    // True als type "integer" is
    bool isLittleEndian;
    int ms;
};
CANConfig currentCfg; // De actieve configuratie

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


void initConfig() {
    currentCfg.offset = getIntConfig("offset");
    currentCfg.bits = getIntConfig("bits");
    currentCfg.factor = getFloatConfig("factor");
    currentCfg.minValue = getFloatConfig("min"); // Zorg dat "min" in je brondata staat
   
    // Cache de string-vergelijkingen naar booleans
    const char* type = getConfig("datatype");
    currentCfg.isByte = (strcmp(type, "byte") == 0);
    currentCfg.isInteger = (strcmp(type, "integer") == 0);
   
    const char* endian = getConfig("endian");
    currentCfg.isLittleEndian = (strcmp(endian, "little") == 0);
    int ms = getIntConfig("ms");
    if (ms < 100) {
      ms = 500;
    }
    currentCfg.ms = ms;
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
    initConfig();
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
int16_t getInt16(const uint8_t *charPtr, bool isLittleEndian) {
    // We lezen expliciet byte 0 en byte 1
    uint16_t b0 = charPtr[0];
    uint16_t b1 = charPtr[1];
    uint16_t result;

    if (isLittleEndian) {
        // Little Endian: Byte 0 is de 'Least Significant Byte' (LSB)
        // Voorbeeld: [0xAA, 0xBB] wordt 0xBBAA
        result = (b1 << 8) | b0;
    } else {
        // Big Endian: Byte 0 is de 'Most Significant Byte' (MSB)
        // Voorbeeld: [0xAA, 0xBB] wordt 0xAABB
        result = (b0 << 8) | b1;
    }

    return (int16_t)result;
}
// Find the PID in the data of an OBD2 frame. OBD2 uses a 2-byte PID for Extended CAN frames, but 1 byte for Standard CAN frames
int16_t getPID(const CanFrame& frame)
{
  int8_t pidLengthInBytes = frame.extd ? 2 : 1;
  return (pidLengthInBytes == 1) ? frame.data[2] : getInt16(&frame.data[2], true);
}

void CANReader(void *par) {
  while (1) {
    CanFrame receivedOBD2Frame;
    while(ESP32Can.readFrame(receivedOBD2Frame)) {
      int16_t msgPID = getPID(receivedOBD2Frame);
      if (msgPID == PID) {
         digitalWrite(BLUE, HIGH);
         CANFrame = receivedOBD2Frame;
      }
    }
    delay(1);
    digitalWrite(BLUE, LOW);
  }
}
float getCANValue() {
    digitalWrite(BUILTIN, HIGH);
   
    float finalValue = 0;

    if (currentCfg.isByte) {
        // --- BYTE LOGICA (Unsigned 8-bit + Min Value) ---
        uint8_t rawByte = CANFrame.data[currentCfg.offset];
       
        if (currentCfg.bits < 8 && currentCfg.bits > 0) {
            rawByte &= (uint8_t)((1 << currentCfg.bits) - 1);
        }
       
        // Formule: (Raw + Min) * Factor
        finalValue = (float)(rawByte + currentCfg.minValue) * currentCfg.factor;

    } else {
        // --- WORD LOGICA (Signed 16-bit) ---
        // We gaan ervan uit dat getInt16 de endianness correct afhandelt
        int16_t rawWord = getInt16(&CANFrame.data[currentCfg.offset], currentCfg.isLittleEndian);
       
        if (currentCfg.bits < 16 && currentCfg.bits > 0) {
            uint16_t mask = (uint16_t)((1 << currentCfg.bits) - 1);
            uint16_t signBit = 1 << (currentCfg.bits - 1);
            uint16_t maskedValue = (uint16_t)rawWord & mask;
           
            // Pas Sign Extension toe als het getal negatief moet zijn
            if (maskedValue & signBit) {
                rawWord = (int16_t)(maskedValue - (1 << currentCfg.bits));
            } else {
                rawWord = (int16_t)maskedValue;
            }
        }
       
        finalValue = (float)rawWord * currentCfg.factor;
    }

    if (currentCfg.isInteger) {
        finalValue = round(finalValue);
    }

    digitalWrite(BUILTIN, LOW);
    return finalValue;
}

void setup() {
  Serial.begin(115200);
  nvs = initNvs();
  tryConnect();
  pinMode(BUILTIN, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
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
      initConfig();
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
      char buf[10];
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
      configPage.concat("&min=");
      configPage.concat(getConfig("min"));
      configPage.concat("&factor=");
      configPage.concat(getConfig("factor"));
      configPage.concat("&viz_type=");
      configPage.concat(getConfig("viz_type"));
      configPage.concat("&bits=");
      configPage.concat(getConfig("bits"));
      configPage.concat("&ms=");
      itoa(currentCfg.ms, buf, 10);
      configPage.concat(buf);
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
      testingPage.concat("&min=");
      testingPage.concat(getConfig("min"));
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
      testingPage.concat("&min=");
      testingPage.concat(getConfig("min"));
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
      testingPage.concat("&min=");
      testingPage.concat(getConfig("min"));
      testingPage.concat("&ms=");
      testingPage.concat(getConfig("ms"));
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
