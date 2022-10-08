#include <NTPClient.h>
#include <FS.h>
#include <ESPAsyncWiFiManager.h>

namespace Ntp {
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

void setup() {
    //update every hour
    ntpClient.setUpdateInterval(3600000);
    ntpClient.begin();
}

void loop() {
    ntpClient.update();
}

unsigned long getEpochTime() {
    return ntpClient.getEpochTime();
}
}