#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

namespace Config {

#define CONFIG_FILE_NAME "/config.json"
#define BRIGHNTESS_FILE_NAME "/bri"

int timeZone, onDurationMs, offDurationMs;

StaticJsonDocument<1024> configDoc;

String getJson() {
    File cf = LittleFS.open(CONFIG_FILE_NAME, "r");
    String res = cf.readString();
    Serial.print("CFG:");Serial.println(res);
    return res;
    cf.close();
}

DeserializationError deserialize(String json) {
    return deserializeJson(configDoc, json);
}

DeserializationError setup() {
    return deserialize(getJson());
}

JsonDocument& getJsonDoc() {
    return configDoc;
}

void saveJson(String json) {
    File cf = LittleFS.open(CONFIG_FILE_NAME, "w");
    cf.write(json.c_str());
    cf.close();
}

String serialize() {
    String res;
    serializeJson(configDoc,res);
    return res;
}

unsigned int getBrightness() {
    File cf = LittleFS.open(BRIGHNTESS_FILE_NAME, "r");
    unsigned int res = 0;
    cf.readBytes((char*)&res, sizeof(unsigned int));
    Serial.print("Read bri=");Serial.println(res);
    cf.close();
    return res;
}

void saveBrightness(unsigned int brigthness) {
    File cf = LittleFS.open(BRIGHNTESS_FILE_NAME, "w");
    cf.write((char*)&brigthness, sizeof(unsigned int));
    cf.close();
    Serial.print("Write bri=");Serial.println(brigthness);
}
}
