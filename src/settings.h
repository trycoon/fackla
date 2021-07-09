
#ifndef settings_h
#define settings_h
// USER SETTINGS ----------------------------------------------------------------------
#define WDT_TIMEOUT_SEC 20          // main loop watchdog, if stalled longer than XX seconds we will reboot.

#define WIFI_SSID "Stargate"
#define WIFI_PASSWD "OEVkynQ!!"
#define OTA_PASSWD "OEVkynQ"

#define MQTT_SERVER "192.168.10.110"
#define MQTT_SERVER_PORT 1883
#define MQTT_TOPIC "home/esp-agriculture/status"
#define MQTT_COMMAND_TOPIC "home/esp-agriculture/command"
// END USER SETTINGS ----------------------------------------------------------------------
#endif