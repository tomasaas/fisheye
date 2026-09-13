#include <WiFi.h>
#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/viz/mjpeg.h>

using eloq::camera;
using eloq::viz::mjpeg;

const char *ssid = "fisheye_AP";

void setup() {
    Serial.begin(115200);

    camera.pinout.xiao();
    camera.resolution.uxga(); // 1600 x 1200
    camera.quality.set(12);

    if (!camera.begin().isOk()) {
        Serial.println(camera.exception.toString());
        while (true) delay(1000);
    }

    IPAddress local_ip(10, 10, 10, 10);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.softAPConfig(local_ip, local_ip, subnet)
        || !WiFi.softAP(ssid)) {
        Serial.println("Access point setup failed.");
        while (true) delay(1000);
    }

    mjpeg.server.setPort(80);

    if (!mjpeg.begin().isOk()) {
        Serial.println(mjpeg.exception.toString());
        while (true) delay(1000);
    }

    Serial.println("Camera ready: http://10.10.10.10/");
}

void loop() {
    delay(1000);
}