#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>

const char *ssid = "fisheye_AP";
WebServer server(80);

void setup() {
    Serial.begin(115200);

    // Camera pins for the XIAO ESP32S3 Sense expansion board.
    camera_config_t config = {};
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.pin_xclk = 10;
    config.pin_sccb_sda = 40;
    config.pin_sccb_scl = 39;
    config.pin_d0 = 15;
    config.pin_d1 = 17;
    config.pin_d2 = 18;
    config.pin_d3 = 16;
    config.pin_d4 = 14;
    config.pin_d5 = 12;
    config.pin_d6 = 11;
    config.pin_d7 = 48;
    config.pin_vsync = 38;
    config.pin_href = 47;
    config.pin_pclk = 13;
    config.xclk_freq_hz = 20000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA; // 640 x 480
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    esp_err_t error = esp_camera_init(&config);
    if (error != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", error);
        while (true) delay(1000);
    }

    IPAddress local_ip(10, 10, 10, 10);
    IPAddress subnet(255, 255, 255, 0);
    if (!WiFi.softAPConfig(local_ip, local_ip, subnet) || !WiFi.softAP(ssid)) {
        Serial.println("Access point setup failed.");
        while (true) delay(1000);
    }

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html",
            "<!doctype html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<title>Fisheye</title><img src=\"/stream\" style=\"max-width:100%\" alt=\"Live camera\">");
    });

    // Send JPEG frames continuously to one viewer at a time.
    server.on("/stream", HTTP_GET, []() {
        WiFiClient client = server.client();
        const char response[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n\r\n";
        if (client.print(response) != sizeof(response) - 1) {
            client.stop();
            return;
        }

        while (client.connected()) {
            camera_fb_t *frame = esp_camera_fb_get();
            if (!frame) {
                Serial.println("Camera capture failed.");
                break;
            }

            char header[96];
            int length = snprintf(header, sizeof(header),
                "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                (unsigned int)frame->len);
            bool sent = client.write((const uint8_t *)header, length) == (size_t)length
                && client.write(frame->buf, frame->len) == frame->len
                && client.print("\r\n") == 2;
            esp_camera_fb_return(frame);
            if (!sent) break;
            delay(1);
        }
        client.stop();
    });

    server.begin();
    Serial.println("Camera ready: http://10.10.10.10/");
}

void loop() {
    server.handleClient();
}
