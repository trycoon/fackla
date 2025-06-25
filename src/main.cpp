#include <Arduino.h>
#define FASTLED_INTERNAL        // Suppress build banner
#include <FastLED.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include "esp_log.h"
#include "settings.h"
#include "ledgfx.h"
#include "fire.h"

#define NUM_LEDS    11
#define LED_PIN     23

CRGB LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED
uint8_t brightness = 255;     // 0-255 LED brightness 

char buff[1024];
static const char* TAG = "Fackla";
String uniqueId = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
String appName = TAG + String("_") + uniqueId;

ClassicFireEffect fire(NUM_LEDS, 50, 300, 3, 2, true, false);
auto s=millis();

void OTA_setup() {
  ArduinoOTA.onStart([]() {
    ESP_LOGI(TAG, "ArduinoOTA start");
  });
  ArduinoOTA.onEnd([]() {
    ESP_LOGI(TAG, "ArduinoOTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    ESP_LOGI(TAG, "ArduinoOTA progress: %u%", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGE(TAG, "ArduinoOTA error[%u]: ", error);

    if (error == OTA_AUTH_ERROR) ESP_LOGE(TAG, "ArduinoOTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) ESP_LOGE(TAG, "ArduinoOTA Begin Failed");
    else if (error == OTA_CONNECT_ERROR) ESP_LOGE(TAG, "ArduinoOTA Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) ESP_LOGE(TAG, "ArduinoOTA Receive Failed");
    else if (error == OTA_END_ERROR) ESP_LOGE(TAG, "ArduinoOTA End Failed");
  });

  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(appName.c_str());
  ArduinoOTA.setPassword(OTA_PASSWD);
  ArduinoOTA.begin();
  ESP_LOGI(TAG, "ArduinoOTA initialized.");
}

void watchdogSetup() {
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);

  esp_err_t err = esp_task_wdt_add(NULL);
  switch (err) {
    case ESP_OK:
      Serial.printf("Watchdog activated OK (timeout is %d seconds)\n", WDT_TIMEOUT_SEC);
      break;
    case ESP_ERR_INVALID_ARG:
      Serial.println("Watchdog activation error: invalid argument");
      break;
    case ESP_ERR_NO_MEM:
      Serial.println("Watchdog activation error: insufficent memory");
      break;
    case ESP_ERR_INVALID_STATE:
      Serial.println("Watchdog activation error: not initialized yet");
      break;
    default:
      Serial.printf("Watchdog activation error: %d\n", err);
      break;
  }
}

void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
        {
          String quality = "";
          auto rssi = WiFi.RSSI();
          if (rssi > -40) {
              quality += String(rssi) + "dBm (Excellent)";
          } else if (rssi > -68) {
              quality +=String(rssi) + "dBm (Very good)";
          } else if (rssi > -71) {
              quality += String(rssi) + "dBm (Good)";
          } else if (rssi > -81) {
              quality += String(rssi) + "dBm (Poor)";
          } else {
              quality += String(rssi) + "dBm (Unusable)";
          }

          ESP_LOGI(TAG, "WiFi connected, IP: %s,  quality: %s", WiFi.localIP().toString().c_str(), quality.c_str());
          break;
        }
      case SYSTEM_EVENT_STA_LOST_IP:
      case SYSTEM_EVENT_STA_DISCONNECTED:
        {
          ESP_LOGI(TAG, "Lost WiFi connection, trying to reconnect...");
          WiFi.begin();
          break;
        }
      default:
        break;
    }
}

void setup() 
{
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);

  while (!Serial) { }
  snprintf(buff, sizeof(buff), "Build: %s %s", __DATE__, __TIME__);
  Serial.println(buff);

  ESP_LOGI(TAG, "Device name: %s", appName.c_str());

  watchdogSetup();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(LEDs, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear();  // Black out LEDs to be sure we start from a known state

  ESP_LOGI(TAG, "WiFi connecting to accesspoint...");
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  OTA_setup();

  ESP_LOGI(TAG, "Setup() done.");
}

void loop() 
{
  ArduinoOTA.handle();

  FastLED.clear();
  fire.DrawFire();
  FastLED.show(brightness);

  delay(30); // 30 eller 1ms
/*
  if (millis() > s + 10000) {
    fire.setBlueFire();
  }
  if (millis() > s + 20000) {
    fire.setGreenFire();
  }
  if (millis() > s + 30000) {
    fire.setRedFire();
  }
  if (millis() > s + 40000) {
    fire.setSolidBlue();
  }
  if (millis() > s + 50000) {
    fire.setSolidRed();
  }
  if (millis() > s + 60000) {
    fire.setSolidGreen();
  }
  if (millis() > s + 70000) {
    fire.setNormalFire();
    s = millis();
  }*/

  esp_task_wdt_reset(); // reset watchdog to show that we are still alive.

}
