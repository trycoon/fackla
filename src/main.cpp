#include <Arduino.h>
#define FASTLED_INTERNAL        // Suppress build banner

#if defined(ESP32) || defined(LIBRETINY)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(TARGET_RP2040) || defined(TARGET_RP2350) || defined(PICO_RP2040) || defined(PICO_RP2350)
#include <RPAsyncTCP.h>
#include <WiFi.h>
#endif
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include <esp_task_wdt.h>
#include "esp_log.h"
#include "settings.h"
#include "ledgfx.h"
#include "fire.h"


#define NUM_LEDS    11
#define LED_PIN     23

static AsyncWebServer server(80);

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
	<head>
			<title>Fackla</title>
			<meta http-equiv="Content-type" content="text/html; charset=utf-8"/>
    	<meta name="viewport" content="width=device-width,initial-scale=1">
      <style>
        body {
          font-family: Arial, sans-serif;
          margin: 0;
          padding: 20px;
          background-color: #f0f0f0;
        }
        h1 {
          color: #333;
        }
        .container {
          max-width: 600px;
          margin: 0 auto;
          background-color: white;
          padding: 20px;
          border-radius: 8px;
          box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .fire-modes {
          margin: 20px 0;
        }
        .radio-group {
          display: flex;
          flex-direction: column;
          gap: 10px;
        }
        .radio-option {
          display: flex;
          align-items: center;
          padding: 10px;
          border-radius: 4px;
          background-color: #f9f9f9;
          cursor: pointer;
          transition: background-color 0.2s;
        }
        .radio-option:hover {
          background-color: #e9e9e9;
        }
        .radio-option input {
          margin-right: 10px;
        }
        .radio-option.normal { border-left: 5px solid orange; }
        .radio-option.red { border-left: 5px solid red; }
        .radio-option.green { border-left: 5px solid green; }
        .radio-option.blue { border-left: 5px solid blue; }
        .radio-option.solid-red { border-left: 5px solid darkred; }
        .radio-option.solid-green { border-left: 5px solid darkgreen; }
        .radio-option.solid-blue { border-left: 5px solid darkblue; }
        .status {
          margin-top: 20px;
          padding: 10px;
          border-radius: 4px;
          background-color: #e0f7fa;
        }
        .brightness-control {
          margin-top: 20px;
          padding: 15px;
          background-color: #f5f5f5;
          border-radius: 8px;
        }
        .slider-container {
          display: flex;
          align-items: center;
          gap: 15px;
        }
        .slider {
          flex-grow: 1;
          height: 8px;
          -webkit-appearance: none;
          appearance: none;
          background: #ddd;
          outline: none;
          border-radius: 4px;
        }
        .slider::-webkit-slider-thumb {
          -webkit-appearance: none;
          appearance: none;
          width: 20px;
          height: 20px;
          background: #ff9800;
          cursor: pointer;
          border-radius: 50%;
          border: none;
        }
        .slider-value {
          min-width: 40px;
          text-align: center;
          font-weight: bold;
        }
        .system-info {
          margin-top: 20px;
          padding: 15px;
          background-color: #f5f5f5;
          border-radius: 8px;
        }
        .info-grid {
          display: grid;
          grid-template-columns: 120px 1fr;
          gap: 10px;
          margin-bottom: 20px;
        }
        .info-label {
          font-weight: bold;
          color: #555;
        }
        .restart-button {
          background-color: #f44336;
          color: white;
          border: none;
          padding: 10px 15px;
          border-radius: 4px;
          cursor: pointer;
          font-weight: bold;
          transition: background-color 0.2s;
        }
        .restart-button:hover {
          background-color: #d32f2f;
        }
      </style>
	</head>
  <body>
    <div class="container">
      <div class="fire-modes">
        <h2>Select Fire Mode</h2>
        <div class="radio-group">
          <label class="radio-option normal">
            <input type="radio" name="fireMode" value="normal" checked> Normal Fire
          </label>
          <label class="radio-option red">
            <input type="radio" name="fireMode" value="red"> Red Fire
          </label>
          <label class="radio-option green">
            <input type="radio" name="fireMode" value="green"> Green Fire
          </label>
          <label class="radio-option blue">
            <input type="radio" name="fireMode" value="blue"> Blue Fire
          </label>
          <label class="radio-option solid-red">
            <input type="radio" name="fireMode" value="solid-red"> Solid Red
          </label>
          <label class="radio-option solid-green">
            <input type="radio" name="fireMode" value="solid-green"> Solid Green
          </label>
          <label class="radio-option solid-blue">
            <input type="radio" name="fireMode" value="solid-blue"> Solid Blue
          </label>
        </div>
      </div>
      <div class="status" id="status">Status: %CURRENT_FIRE_MODE% fire mode active</div>
      
      <div class="brightness-control">
        <h2>Brightness</h2>
        <div class="slider-container">
          <input type="range" min="10" max="255" value="%CURRENT_BRIGHTNESS%" class="slider" id="brightnessSlider">
          <div class="slider-value" id="brightnessValue">%CURRENT_BRIGHTNESS%</div>
        </div>
      </div>
      
      <div class="system-info">
        <h2>System Information</h2>
        <div class="info-grid">
          <div class="info-label">Device:</div>
          <div class="info-value">%DEVICE_NAME%</div>
          
          <div class="info-label">Uptime:</div>
          <div class="info-value">%UPTIME%</div>
          
          <div class="info-label">WiFi:</div>
          <div class="info-value">%WIFI_QUALITY%</div>
          
          <div class="info-label">IP Address:</div>
          <div class="info-value">%IP_ADDRESS%</div>
        </div>
        
        <button id="restartBtn" class="restart-button">Restart Device</button>
      </div>
    </div>
    <script>
      // Set the initial radio button based on the current mode
      document.addEventListener('DOMContentLoaded', () => {
        const currentMode = '%CURRENT_FIRE_MODE%'.toLowerCase().replace(' ', '-');
        const radioButton = document.querySelector(`input[name="fireMode"][value="${currentMode}"]`);
        if (radioButton) {
          radioButton.checked = true;
        }
        
        // Auto-refresh system info every 10 seconds
        setInterval(() => {
          fetch('/system-info')
            .then(response => response.json())
            .then(data => {
              document.querySelector('.info-value:nth-of-type(2)').textContent = data.deviceName;
              document.querySelector('.info-value:nth-of-type(4)').textContent = data.uptime;
              document.querySelector('.info-value:nth-of-type(6)').textContent = data.wifiQuality;
              document.querySelector('.info-value:nth-of-type(8)').textContent = data.ipAddress;
            })
            .catch(error => console.error('Error fetching system info:', error));
        }, 10000);
        
        // Setup brightness slider
        const brightnessSlider = document.getElementById('brightnessSlider');
        const brightnessValue = document.getElementById('brightnessValue');
        
        brightnessSlider.addEventListener('input', () => {
          brightnessValue.textContent = brightnessSlider.value;
        });
        
        brightnessSlider.addEventListener('change', () => {
          const brightness = parseInt(brightnessSlider.value);
          fetch('/brightness', {
            method: 'PUT',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({
              brightness: brightness
            })
          })
          .then(response => {
            if (!response.ok) {
              throw new Error('Network response was not ok.');
            }
            return response.json();
          })
          .catch(error => {
            console.error('Error updating brightness:', error);
          });
        });
        
        // Restart button
        const restartBtn = document.getElementById('restartBtn');
        restartBtn.addEventListener('click', () => {
          if (confirm('Are you sure you want to restart the device?')) {
            fetch('/restart', {
              method: 'POST'
            })
            .then(() => {
              const statusEl = document.getElementById('status');
              statusEl.textContent = 'Restarting device...';
              statusEl.style.backgroundColor = '#ffeb3b';
              setTimeout(() => {
                window.location.reload();
              }, 5000);
            });
          }
        });
      });
      
      document.querySelectorAll('input[name="fireMode"]').forEach(radio => {
        radio.addEventListener('change', event => {
          const mode = event.target.value;
          const statusEl = document.getElementById('status');
          
          fetch('/state', {
            method: 'PUT',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({
              fireMode: mode
            })
          })
          .then(response => {
            if (response.ok) {
              // Format the mode for display by converting 'solid-red' to 'Solid Red'
              let displayMode = mode.replace('-', ' ').split(' ')
                .map(word => word.charAt(0).toUpperCase() + word.slice(1))
                .join(' ');
              statusEl.textContent = `Status: ${displayMode} fire mode activated`;
              return response.json();
            }
            throw new Error('Network response was not ok.');
          })
          .catch(error => {
            statusEl.textContent = `Error: ${error.message}`;
            console.error('Error:', error);
          });
        });
      });
    </script>
  </body>
</html>
)";

CRGB LEDs[NUM_LEDS] = {0};    // Frame buffer for FastLED
uint8_t brightness = 255;     // 0-255 LED brightness 

char buff[1024];
static const char* TAG = "Fackla";
String uniqueId = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
String appName = TAG + String("_") + uniqueId;

ClassicFireEffect fire(NUM_LEDS, 50, 300, 3, 2, true, false);
String currentFireMode = "normal"; // Track the current fire mode
auto s=millis();

void saveSettings() {
  DynamicJsonDocument doc(1024);
  doc["fireMode"] = currentFireMode;
  doc["brightness"] = brightness;
  
  File file = LittleFS.open("/settings.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    ESP_LOGI(TAG, "Settings saved");
  } else {
    ESP_LOGE(TAG, "Failed to open settings file for writing");
  }
}

// Load settings from LittleFS
void loadSettings() {
  File file = LittleFS.open("/settings.json", "r");
  if (file) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (!error) {
      if (doc.containsKey("fireMode")) {
        currentFireMode = doc["fireMode"].as<String>();
        ESP_LOGI(TAG, "Loaded fire mode: %s", currentFireMode.c_str());
        
        // Apply the loaded fire mode
        if (currentFireMode == "normal") {
          fire.setNormalFire();
        } else if (currentFireMode == "red") {
          fire.setRedFire();
        } else if (currentFireMode == "green") {
          fire.setGreenFire();
        } else if (currentFireMode == "blue") {
          fire.setBlueFire();
        } else if (currentFireMode == "solid-red") {
          fire.setSolidRed();
        } else if (currentFireMode == "solid-green") {
          fire.setSolidGreen();
        } else if (currentFireMode == "solid-blue") {
          fire.setSolidBlue();
        }
      }
      
      if (doc.containsKey("brightness")) {
        brightness = doc["brightness"].as<uint8_t>();
        brightness = constrain(brightness, 10, 255);
        ESP_LOGI(TAG, "Loaded brightness: %d", brightness);
      }
    } else {
      ESP_LOGE(TAG, "Failed to parse settings file");
    }
  } else {
    ESP_LOGI(TAG, "No settings file found, using defaults");
  }
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
      case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "WiFi station started");
        break;
      case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "WiFi connected to AP, waiting for IP");
        break;
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
        {
          ESP_LOGW(TAG, "Lost IP address at %lu ms", millis());
          break;
        }
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
  WiFi.setSleep(false); // Disable WiFi sleep mode for faster response
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  WiFi.setAutoReconnect(true);  

  #ifdef ESP32
  LittleFS.begin(true);
  #else
  LittleFS.begin();
  #endif

  FastLED.setBrightness(brightness);

  // Load settings from LittleFS
  loadSettings();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    ESP_LOGI(TAG, "GET request for / from IP: %s", clientIP.c_str());
    
    // Format the current fire mode for display (capitalize first letter)
    String displayMode = currentFireMode;
    if (displayMode.length() > 0) {
      displayMode.setCharAt(0, toupper(displayMode.charAt(0)));
      // Replace hyphens with spaces and capitalize
      int hyphenIndex = displayMode.indexOf('-');
      if (hyphenIndex != -1) {
        displayMode.setCharAt(hyphenIndex, ' ');
        if (hyphenIndex + 1 < displayMode.length()) {
          displayMode.setCharAt(hyphenIndex + 1, toupper(displayMode.charAt(hyphenIndex + 1)));
        }
      }
    }
    
    // Prepare uptime string
    unsigned long uptime = millis() / 1000;
    int seconds = uptime % 60;
    int minutes = (uptime / 60) % 60;
    int hours = (uptime / 3600) % 24;
    int days = uptime / 86400;
    
    char uptimeStr[50];
    if (days > 0) {
      sprintf(uptimeStr, "%d days, %02d:%02d:%02d", days, hours, minutes, seconds);
    } else {
      sprintf(uptimeStr, "%02d:%02d:%02d", hours, minutes, seconds);
    }
    
    // Prepare WiFi quality string
    auto rssi = WiFi.RSSI();
    String quality = String(rssi) + " dBm (";
    if (rssi > -40) {
      quality += "Excellent";
    } else if (rssi > -68) {
      quality += "Very good";
    } else if (rssi > -71) {
      quality += "Good";
    } else if (rssi > -81) {
      quality += "Poor";
    } else {
      quality += "Unusable";
    }
    quality += ")";
    
    // Build the complete HTML response without using template processor
    String html = R"(<!DOCTYPE html>
<html>
  <head>
    <title>Fackla</title>
    <meta http-equiv="Content-type" content="text/html; charset=utf-8"/>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 20px;
        background-color: #f0f0f0;
      }
      h1 {
        color: #333;
      }
      .container {
        max-width: 600px;
        margin: 0 auto;
        background-color: white;
        padding: 20px;
        border-radius: 8px;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      }
      .fire-modes {
        margin: 20px 0;
      }
      .radio-group {
        display: flex;
        flex-direction: column;
        gap: 10px;
      }
      .radio-option {
        display: flex;
        align-items: center;
        padding: 10px;
        border-radius: 4px;
        background-color: #f9f9f9;
        cursor: pointer;
        transition: background-color 0.2s;
      }
      .radio-option:hover {
        background-color: #e9e9e9;
      }
      .radio-option input {
        margin-right: 10px;
      }
      .radio-option.normal { border-left: 5px solid orange; }
      .radio-option.red { border-left: 5px solid red; }
      .radio-option.green { border-left: 5px solid green; }
      .radio-option.blue { border-left: 5px solid blue; }
      .radio-option.solid-red { border-left: 5px solid darkred; }
      .radio-option.solid-green { border-left: 5px solid darkgreen; }
      .radio-option.solid-blue { border-left: 5px solid darkblue; }
      .status {
        margin-top: 20px;
        padding: 10px;
        border-radius: 4px;
        background-color: #e0f7fa;
      }
      .brightness-control {
        margin-top: 20px;
        padding: 15px;
        background-color: #f5f5f5;
        border-radius: 8px;
      }
      .slider-container {
        display: flex;
        align-items: center;
        gap: 15px;
      }
      .slider {
        flex-grow: 1;
        height: 8px;
        -webkit-appearance: none;
        appearance: none;
        background: #ddd;
        outline: none;
        border-radius: 4px;
      }
      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 20px;
        height: 20px;
        background: #ff9800;
        cursor: pointer;
        border-radius: 50%;
        border: none;
      }
      .slider-value {
        min-width: 40px;
        text-align: center;
        font-weight: bold;
      }
      .system-info {
        margin-top: 20px;
        padding: 15px;
        background-color: #f5f5f5;
        border-radius: 8px;
      }
      .info-grid {
        display: grid;
        grid-template-columns: 120px 1fr;
        gap: 10px;
        margin-bottom: 20px;
      }
      .info-label {
        font-weight: bold;
        color: #555;
      }
      .restart-button {
        background-color: #f44336;
        color: white;
        border: none;
        padding: 10px 15px;
        border-radius: 4px;
        cursor: pointer;
        font-weight: bold;
        transition: background-color 0.2s;
      }
      .restart-button:hover {
        background-color: #d32f2f;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="fire-modes">
        <h2>Select Fire Mode</h2>
        <div class="radio-group">)";
        
    // Add radio options with the current one checked
    html += "<label class='radio-option normal'>";
    html += "<input type='radio' name='fireMode' value='normal'" + String(currentFireMode == "normal" ? " checked" : "") + "> Normal Fire</label>";
    
    html += "<label class='radio-option red'>";
    html += "<input type='radio' name='fireMode' value='red'" + String(currentFireMode == "red" ? " checked" : "") + "> Red Fire</label>";
    
    html += "<label class='radio-option green'>";
    html += "<input type='radio' name='fireMode' value='green'" + String(currentFireMode == "green" ? " checked" : "") + "> Green Fire</label>";
    
    html += "<label class='radio-option blue'>";
    html += "<input type='radio' name='fireMode' value='blue'" + String(currentFireMode == "blue" ? " checked" : "") + "> Blue Fire</label>";
    
    html += "<label class='radio-option solid-red'>";
    html += "<input type='radio' name='fireMode' value='solid-red'" + String(currentFireMode == "solid-red" ? " checked" : "") + "> Solid Red</label>";
    
    html += "<label class='radio-option solid-green'>";
    html += "<input type='radio' name='fireMode' value='solid-green'" + String(currentFireMode == "solid-green" ? " checked" : "") + "> Solid Green</label>";
    
    html += "<label class='radio-option solid-blue'>";
    html += "<input type='radio' name='fireMode' value='solid-blue'" + String(currentFireMode == "solid-blue" ? " checked" : "") + "> Solid Blue</label>";
    
    html += R"(
        </div>
      </div>
      <div class="status" id="status">Status: )" + displayMode + R"( fire mode active</div>
      
      <div class="brightness-control">
        <h2>Brightness</h2>
        <div class="slider-container">
          <input type="range" min="10" max="255" value=")" + String(brightness) + R"(" class="slider" id="brightnessSlider">
          <div class="slider-value" id="brightnessValue">)" + String(brightness) + R"(</div>
        </div>
      </div>
      
      <div class="system-info">
        <h2>System Information</h2>
        <div class="info-grid">
          <div class="info-label">Device:</div>
          <div class="info-value">)" + appName + R"(</div>
          
          <div class="info-label">Uptime:</div>
          <div class="info-value">)" + String(uptimeStr) + R"(</div>
          
          <div class="info-label">WiFi:</div>
          <div class="info-value">)" + quality + R"(</div>
          
          <div class="info-label">IP Address:</div>
          <div class="info-value">)" + WiFi.localIP().toString() + R"(</div>
        </div>
        
        <button id="restartBtn" class="restart-button">Restart Device</button>
      </div>
    </div>
    <script>
      // Set the initial radio button based on the current mode
      document.addEventListener('DOMContentLoaded', () => {
        // Auto-refresh system info every 10 seconds
        setInterval(() => {
          fetch('/system-info')
            .then(response => response.json())
            .then(data => {
              document.querySelector('.info-value:nth-of-type(2)').textContent = data.deviceName;
              document.querySelector('.info-value:nth-of-type(4)').textContent = data.uptime;
              document.querySelector('.info-value:nth-of-type(6)').textContent = data.wifiQuality;
              document.querySelector('.info-value:nth-of-type(8)').textContent = data.ipAddress;
            })
            .catch(error => console.error('Error fetching system info:', error));
        }, 10000);
        
        // Setup brightness slider
        const brightnessSlider = document.getElementById('brightnessSlider');
        const brightnessValue = document.getElementById('brightnessValue');
        
        brightnessSlider.addEventListener('input', () => {
          brightnessValue.textContent = brightnessSlider.value;
        });
        
        brightnessSlider.addEventListener('change', () => {
          const brightness = parseInt(brightnessSlider.value);
          fetch('/brightness', {
            method: 'PUT',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({
              brightness: brightness
            })
          })
          .then(response => {
            if (!response.ok) {
              throw new Error('Network response was not ok.');
            }
            return response.json();
          })
          .catch(error => {
            console.error('Error updating brightness:', error);
          });
        });
        
        // Restart button
        const restartBtn = document.getElementById('restartBtn');
        restartBtn.addEventListener('click', () => {
          if (confirm('Are you sure you want to restart the device?')) {
            fetch('/restart', {
              method: 'POST'
            })
            .then(() => {
              const statusEl = document.getElementById('status');
              statusEl.textContent = 'Restarting device...';
              statusEl.style.backgroundColor = '#ffeb3b';
              setTimeout(() => {
                window.location.reload();
              }, 5000);
            });
          }
        });
      });
      
      document.querySelectorAll('input[name="fireMode"]').forEach(radio => {
        radio.addEventListener('change', event => {
          const mode = event.target.value;
          const statusEl = document.getElementById('status');
          
          fetch('/state', {
            method: 'PUT',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({
              fireMode: mode
            })
          })
          .then(response => {
            if (response.ok) {
              // Format the mode for display by converting 'solid-red' to 'Solid Red'
              let displayMode = mode.replace('-', ' ').split(' ')
                .map(word => word.charAt(0).toUpperCase() + word.slice(1))
                .join(' ');
              statusEl.textContent = `Status: ${displayMode} fire mode activated`;
              return response.json();
            }
            throw new Error('Network response was not ok.');
          })
          .catch(error => {
            statusEl.textContent = `Error: ${error.message}`;
            console.error('Error:', error);
          });
        });
      });
    </script>
  </body>
</html>)";
    
    request->send(200, "text/html", html);
  });

  server.on("/state", HTTP_PUT, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    ESP_LOGI(TAG, "PUT request for /state from IP: %s", clientIP.c_str());
    
    // Empty response for now, we'll handle the body in onBody
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Handle the PUT request body
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, (const char*)data, len);
    
    String fireMode = doc["fireMode"].as<String>();
    ESP_LOGI(TAG, "Fire mode changed to: %s", fireMode.c_str());
    
    currentFireMode = fireMode; // Update the current fire mode
    
    if (fireMode == "normal") {
      fire.setNormalFire();
    } else if (fireMode == "red") {
      fire.setRedFire();
    } else if (fireMode == "green") {
      fire.setGreenFire();
    } else if (fireMode == "blue") {
      fire.setBlueFire();
    } else if (fireMode == "solid-red") {
      fire.setSolidRed();
    } else if (fireMode == "solid-green") {
      fire.setSolidGreen();
    } else if (fireMode == "solid-blue") {
      fire.setSolidBlue();
    }
    
    // Save settings whenever they change
    saveSettings();
  });

  // Add brightness endpoint
  server.on("/brightness", HTTP_PUT, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    ESP_LOGI(TAG, "PUT request for /brightness from IP: %s", clientIP.c_str());
    
    // Empty response for now, we'll handle the body in onBody
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Handle the PUT request body
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, (const char*)data, len);
    
    if (doc.containsKey("brightness")) {
      int newBrightness = doc["brightness"].as<int>();
      // Ensure brightness is within valid range
      newBrightness = constrain(newBrightness, 10, 255);
      brightness = newBrightness;
      
      ESP_LOGI(TAG, "Brightness changed to: %d", brightness);
      FastLED.setBrightness(brightness);
      
      // Save settings when brightness changes
      saveSettings();
    }
  });

  // Add restart endpoint
  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    ESP_LOGI(TAG, "Device restart requested from IP: %s", clientIP.c_str());
    
    request->send(200, "application/json", "{\"status\":\"restarting\"}");
    
    // Schedule a restart after sending the response
    // We use a small delay to allow the response to be sent
    static const int kRestartDelayMs = 500;
    static bool shouldRestart = false;
    
    shouldRestart = true;
    static uint32_t restartTime = millis() + kRestartDelayMs;
    
    // Setup a one-shot timer to restart the device
    // This is safer than restarting immediately
    static esp_timer_handle_t restart_timer;
    static esp_timer_create_args_t restart_timer_args = {
      .callback = [](void* arg) {
        ESP_LOGI(TAG, "Restarting now...");
        esp_restart();
      },
      .name = "restart-timer"
    };
    
    if (restart_timer == nullptr) {
      ESP_ERROR_CHECK(esp_timer_create(&restart_timer_args, &restart_timer));
    }
    
    ESP_ERROR_CHECK(esp_timer_start_once(restart_timer, kRestartDelayMs * 1000));
  });

  // Add system info endpoint for auto-refresh
  server.on("/system-info", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    
    // Device name
    doc["deviceName"] = appName;
    
    // Uptime
    unsigned long uptime = millis() / 1000;
    int seconds = uptime % 60;
    int minutes = (uptime / 60) % 60;
    int hours = (uptime / 3600) % 24;
    int days = uptime / 86400;
    
    char uptimeStr[50];
    if (days > 0) {
      sprintf(uptimeStr, "%d days, %02d:%02d:%02d", days, hours, minutes, seconds);
    } else {
      sprintf(uptimeStr, "%02d:%02d:%02d", hours, minutes, seconds);
    }
    doc["uptime"] = uptimeStr;
    
    // WiFi quality
    auto rssi = WiFi.RSSI();
    String quality = String(rssi) + " dBm (";
    if (rssi > -40) {
      quality += "Excellent";
    } else if (rssi > -68) {
      quality += "Very good";
    } else if (rssi > -71) {
      quality += "Good";
    } else if (rssi > -81) {
      quality += "Poor";
    } else {
      quality += "Unusable";
    }
    quality += ")";
    doc["wifiQuality"] = quality;
    
    // IP Address
    doc["ipAddress"] = WiFi.localIP().toString();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Handle requests for non-existent resources
  server.onNotFound([](AsyncWebServerRequest *request) {
    String clientIP = request->client()->remoteIP().toString();
    String method = (request->method() == HTTP_GET) ? "GET" : 
                   (request->method() == HTTP_POST) ? "POST" : 
                   (request->method() == HTTP_DELETE) ? "DELETE" : 
                   (request->method() == HTTP_PUT) ? "PUT" : 
                   (request->method() == HTTP_PATCH) ? "PATCH" : 
                   (request->method() == HTTP_HEAD) ? "HEAD" : 
                   (request->method() == HTTP_OPTIONS) ? "OPTIONS" : "UNKNOWN";
    
    ESP_LOGW(TAG, "404 Not Found: %s request for %s from IP: %s", 
             method.c_str(), 
             request->url().c_str(), 
             clientIP.c_str());
    
    request->send(404, "text/plain", "Not found");
  });

  server.begin();

  ESP_LOGI(TAG, "Web server started on http://%s", WiFi.localIP().toString().c_str());
  ESP_LOGI(TAG, "setup() done.");
}

void loop() 
{
  FastLED.clear();
  fire.DrawFire();
  FastLED.show(brightness);

  delay(30); // 30 eller 1ms

  esp_task_wdt_reset(); // reset watchdog to show that we are still alive.
}
