#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

namespace Config {

#define CONFIG_FILE_NAME "/config.json"
#define BRIGHNTESS_FILE_NAME "/bri"

int timeZone, onDurationMs, offDurationMs, storedBrighthess = -1;

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
    storedBrighthess = -1;
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
    if (storedBrighthess < 0) {
        File cf = LittleFS.open(BRIGHNTESS_FILE_NAME, "r");
        cf.readBytes((char*)&storedBrighthess, sizeof(unsigned int));
        Serial.print("Read bri=");Serial.println(storedBrighthess);
        cf.close();
    }   
    return storedBrighthess;
}

void saveBrightness(unsigned int brigthness) {
    File cf = LittleFS.open(BRIGHNTESS_FILE_NAME, "w");
    cf.write((char*)&brigthness, sizeof(unsigned int));
    cf.close();
    storedBrighthess = brigthness;
    Serial.print("Write bri=");Serial.println(brigthness);
}

int getTimeZone() {
    return getJsonDoc()["timeZone"];
}

unsigned int getOnDurationMs() {
    return getJsonDoc()["onDurationMs"];
}

unsigned int getOffDurationMs() {
    return getJsonDoc()["offDurationMs"];
}

unsigned int getDefaultBrightness() {
    return getJsonDoc()["defaultBrightness"];
}

}