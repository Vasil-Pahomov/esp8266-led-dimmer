#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "ntp.h"
#include "config.h"

namespace Web {
AsyncWebServer server(80);

DNSServer dnsServer;
AsyncWiFiManager wifiManager(&server, &dnsServer);

// Replaces placeholder with LED state value
String processor(const String& var){
  if (var == "CURRENT_TIME") {
    return String(Ntp::getEpochTime());
  } 
  else if (var == "CONFIG_JSON") {
    return Config::getJson();
  } 
  else if (var == "CONFIG_SERIALIZED") {
    return Config::serialize();
  }
  return String();
}

void setup(bool forceConfigPortal) {
  String apName = "LEDDimmer" + String(ESP.getChipId(), 16);

  wifiManager.setConfigPortalTimeout(60);
  if (forceConfigPortal) {
    wifiManager.resetSettings();
    wifiManager.startConfigPortal(apName.c_str());
  } else {
    wifiManager.autoConnect(apName.c_str());
  }

    // Initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  server.on("/savecfg",[](AsyncWebServerRequest *request){
    DeserializationError res = Config::deserialize(request->arg("cfg"));
    if (res != DeserializationError::Ok) {
      char buf[100] = "Invalid JSON, go back and fix errors. Error: ";
      strcat(buf, res.c_str());
      request->send(400, "text/html", buf);
    } else {
      Config::saveJson(request->arg("cfg"));
      request->redirect("/");
    }
  });

  // Start server
  server.begin();
}

void loop() {
  wifiManager.loop();
}
}