#include <Arduino.h>
#include <ArduinoJson.h>

namespace Config {

        String getJson();
        DeserializationError setup();
        DeserializationError deserialize(String json);
        JsonDocument& getJsonDoc();
        void saveJson(String json);
        String serialize();
        unsigned int getBrightness();
        void saveBrightness(unsigned int brigthness);
        int getTimeZone();
        unsigned int getOnDurationMs();
        unsigned int getOffDurationMs();
        unsigned int getDefaultBrightness();
};
