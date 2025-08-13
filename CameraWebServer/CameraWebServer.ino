#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_sleep.h>

// ========== AI Thinker ESP32-CAM Pin Definitions ==========
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ====== Wi-Fi ======
const char* ssid = "el pepe";
const char* password = "3guys1cup";

// ========== Cloudinary Credentials ==========
const char* cloud_name = "dlr6drgoj";
const char* upload_preset = "ESP32-CAM";

// ========== ThingSpeak Credentials ==========
const char* thingspeak_api_key = "KOU513ED704T5O07"; // For uploading snapshots (field1) and heartbeat (field3)
const char* thingspeak_read_api_key = "N6598QTZTIAWCV0H"; // For reading arm/disarm
const char* thingspeak_channel_id = "3033231";
const int ARM_FIELD = 5; // Use field5 for arm/disarm
const int STATUS_FIELD = 3; // Use field3 for heartbeat/status

// ========== InfinityFree Endpoint ==========
const char* backend_url = "https://fiot-intruder-alarm.infinityfree.me/server/add_snapshot.php";

// ========== Arm/Disarm Logic ==========
// Default: DISARMED (no snapshots taken)

void startCameraServer();
void setupLedFlash();

bool pollArmedStatus() {
  HTTPClient http;
  String armUrl = String("http://api.thingspeak.com/channels/") + thingspeak_channel_id +
                  "/fields/" + String(ARM_FIELD) + "/last.json?api_key=" + thingspeak_read_api_key;
  http.begin(armUrl);
  int httpCode = http.GET();
  bool armed = false; // Default: disarmed
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("ThingSpeak payload: " + payload);
    int pos = payload.indexOf("\"field5\":\"");
    if (pos != -1) {
      int valStart = pos + 10;
      char val = payload[valStart];
      armed = (val == '1');
      Serial.printf("ThingSpeak: esp32Armed = %s\n", armed ? "true" : "false");
    } else {
      Serial.println("Could not parse field5 from ThingSpeak JSON");
    }
  } else {
    Serial.printf("ThingSpeak poll failed: HTTP %d\n", httpCode);
  }
  http.end();
  return armed;
}

// --- Heartbeat: Send ESP32 status to field3 every 5 seconds
unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 5000; // 5 seconds

void sendHeartbeat() {
  String url = "http://api.thingspeak.com/update?api_key=";
  url += thingspeak_api_key;
  url += "&field3=1"; // You could use a timestamp or just '1' to indicate alive
  HTTPClient http;
  if (http.begin(url)) {
    int code = http.GET();
    http.end();
    Serial.printf("Heartbeat sent to field3. HTTP %d\n", code);
  } else {
    Serial.println("Heartbeat HTTP begin failed!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera and WiFi setup
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 16000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  if (psramFound()) {
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.frame_size = FRAMESIZE_VGA;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

unsigned long lastUpload = 0;
const unsigned long uploadInterval = 20000; // 20 seconds

void loop() {
  // Heartbeat: update field3 every 5 seconds
  if (millis() - lastHeartbeat > heartbeatInterval) {
    lastHeartbeat = millis();
    sendHeartbeat();
  }

  // Check armed status from ThingSpeak field5
  bool esp32Armed = pollArmedStatus();

  if (!esp32Armed) {
    Serial.println("Disarmed! No snapshots will be taken.");
    delay(5000); // Poll again in 5 seconds for status changes
    return;
  }

  // Armed: perform regular logic (photo upload, etc.)
  if (millis() - lastUpload > uploadInterval) {
    lastUpload = millis();
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    Serial.printf("Photo captured: %zu bytes\n", fb->len);

    // ---- Cloudinary Upload (buffer-based) ----
    HTTPClient http;
    String url = "https://api.cloudinary.com/v1_1/" + String(cloud_name) + "/image/upload";
    if (!http.begin(url)) {
      Serial.println("Cloudinary HTTP begin failed!");
      esp_camera_fb_return(fb);
      return;
    }

    String boundary = "CloudinaryBoundary";
    String startRequest = "--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"\r\n"
      "Content-Type: image/jpeg\r\n\r\n";
    String midRequest = "\r\n--" + boundary + "\r\n"
      "Content-Disposition: form-data; name=\"upload_preset\"\r\n\r\n" + String(upload_preset) + "\r\n";
    String endRequest = "--" + boundary + "--\r\n";

    int totalLen = startRequest.length() + fb->len + midRequest.length() + endRequest.length();
    uint8_t* postBuffer = (uint8_t*)malloc(totalLen);
    if (!postBuffer) {
      Serial.println("Failed to allocate memory for POST buffer!");
      http.end();
      esp_camera_fb_return(fb);
      return;
    }

    int idx = 0;
    memcpy(postBuffer + idx, startRequest.c_str(), startRequest.length());
    idx += startRequest.length();
    memcpy(postBuffer + idx, fb->buf, fb->len);
    idx += fb->len;
    memcpy(postBuffer + idx, midRequest.c_str(), midRequest.length());
    idx += midRequest.length();
    memcpy(postBuffer + idx, endRequest.c_str(), endRequest.length());
    idx += endRequest.length();

    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    Serial.println("Uploading to Cloudinary...");
    int httpCode = http.POST(postBuffer, totalLen);
    String response = http.getString();

    free(postBuffer);
    http.end();
    esp_camera_fb_return(fb);

    String imageUrl = "";
    if (httpCode == 200) {
      Serial.println("Upload successful!");
      int urlStart = response.indexOf("\"secure_url\":\"");
      if (urlStart != -1) {
        urlStart += 14;
        int urlEnd = response.indexOf("\"", urlStart);
        imageUrl = response.substring(urlStart, urlEnd);
        Serial.println("Image URL: " + imageUrl);
      } else {
        Serial.println("Could not parse image URL");
        Serial.println(response);
      }
    } else {
      Serial.printf("Cloudinary upload failed: HTTP %d\n", httpCode);
      Serial.println(response);
    }

    // ---- ThingSpeak Update (for snapshots) ----
    if (imageUrl.length() > 0) {
      String tsUrl = "http://api.thingspeak.com/update?api_key=";
      tsUrl += thingspeak_api_key;
      tsUrl += "&field1=";
      tsUrl += imageUrl;

      HTTPClient tsHttp;
      if (!tsHttp.begin(tsUrl)) {
        Serial.println("ThingSpeak HTTP begin failed!");
        return;
      }
      int tsCode = tsHttp.GET();
      String tsResp = tsHttp.getString();
      if (tsCode > 0) {
        Serial.printf("ThingSpeak update: HTTP %d\n", tsCode);
        Serial.println(tsResp);
      } else {
        Serial.printf("ThingSpeak update failed: HTTP %d\n", tsCode);
      }
      tsHttp.end();

      // ---- POST to InfinityFree backend ----
      HTTPClient backendHttp;
      if (backendHttp.begin(backend_url)) {
        backendHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");

        unsigned long now = millis();
        String timestamp = String(now);

        String postData = "url=" + imageUrl + "&timestamp=" + timestamp;
        int backendCode = backendHttp.POST(postData);
        String backendResp = backendHttp.getString();
        Serial.printf("Snapshot POST: HTTP %d\n", backendCode);
        Serial.println(backendResp);
        backendHttp.end();
      } else {
        Serial.println("Failed to connect to backend for snapshot POST!");
      }
    }

    Serial.println("Waiting for next photo...");
  }
  delay(100); // Don't hammer loop
}